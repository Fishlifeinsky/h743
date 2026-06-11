# ctpb (控制传输 protobuf)

基于 mem 模块的 `--wrap=malloc` 机制，unpack 传 `NULL` 分配器即可使用 TLSF 内存池。

## 协议定义 (CTP.proto)

命令传输协议 (Command Transfer Protocol)，主机发送指令，设备端应答。

| 指令类型 | 枚举值 | 说明 |
|---------|--------|------|
| `CMD_EASY` | 0 | 简单指令，一次应答完成 |
| `CMD_COMP` | 1 | 复杂指令，设备端需创建上下文 (handle) |
| `CMD_SWIT` | 2 | 切换指令，一次应答后独占通信 |

当前已定义指令：

| 指令 | 类型 | 命令体 | 响应体 |
|------|------|--------|--------|
| Ping | EASY | `PingCmd` (无参) | `PingRes { bool alive }` |
| Version | EASY | `VersionCmd` (无参) | `VersionRes { string version }` |

顶层帧结构：

```
CTPCmd { id, handle, type, oneof{ PingCmd, VersionCmd, ... } }
CTPRes { id, oneof{ PingRes, VersionRes, ... } }
```

- `id` — 主机生成，设备原样带回
- `handle` — 仅 `CMD_COMP` 时有效

## 生成 protobuf-c 文件

1. 编辑 `.proto` 文件（`CTP.proto`）
2. 执行生成脚本：
   ```bat
   tools\gen_proto.bat CTP.proto
   ```
   或手动执行：
   ```bat
   set PATH=tools;%PATH%
   cd port\ctpb
   protoc --c_out=. CTP.proto
   ```
3. 重新编译项目，`module.cmake` 会通过 `file(GLOB *.pb-c.c)` 自动包含生成的 C 文件

## 使用示例

### 构造并发送命令

```c
#include "ctp.pb-c.h"

PingCmd body = PING_CMD__INIT;
CTPCmd  cmd  = CTPCMD__INIT;

cmd.id        = 1;
cmd.type      = CMD_TYPE__CMD_EASY;
cmd.body_case = CTPCMD__BODY_PING;
cmd.ping      = &body;

uint8_t *out = NULL;
size_t  len  = ctpb_frame_pack(&cmd.base, &out);
// 发送 out/len ...
free(out);
```

### 接收并应答

```c
uint8_t buf[256];
size_t len = ctpb_frame_recv(buf, sizeof(buf), portMAX_DELAY);
CTPCmd *recv = ctpcmd__unpack(NULL, len, buf);

PingRes body = PING_RES__INIT;
CTPRes  res  = CTPRES__INIT;
res.id        = recv->id;
res.body_case = CTPRES__BODY_PING;
res.ping      = &body;
body.alive    = true;

uint8_t *out = NULL;
size_t  len2 = ctpb_frame_pack(&res.base, &out);
// 发送 out/len2 ...

ctpcmd__free_unpacked(recv, NULL);
free(out);
```

### 添加新指令

1. 在 `CTP.proto` 中定义 `message XxxCmd` / `message XxxRes`
2. 在 `CTPCmd` / `CTPRes` 的 `oneof body` 中添加字段
3. 运行 `tools\gen_proto.bat` 重新生成
4. 在 `ctpb.c` 中添加 `static void handle_xxx(const CTPCmd *cmd)` 处理函数
5. 在 `ctpb_init()` 中注册: `g_handlers[CTPCMD__BODY_XXX] = handle_xxx;`

## 上下文管理 (CMD_COMP)

复杂指令需设备端创建上下文，上下文通过链表统一管理。`handle` 用于关联后续指令与已创建的上下文。上下文的生命周期由设备端控制，指令完成后释放。
