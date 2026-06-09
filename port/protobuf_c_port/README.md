# protobuf_c_port

protobuf-c 移植模块。基于 mem 模块的 `--wrap=malloc` 机制，unpack 传 `NULL` 分配器即可使用 TLSF 内存池。

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
   cd port\protobuf_c_port
   protoc --c_out=. CTP.proto
   ```
3. 重新编译项目，`module.cmake` 会通过 `file(GLOB *.pb-c.c)` 自动包含生成的 C 文件

## 使用示例

```c
#include "protobuf_c_port.h"
#include "ctp.pb-c.h"

// ===== 主机侧：打包命令 =====
CTPCmd cmd = CTP_CMD__INIT;
cmd.id = 1;
cmd.type = CMD__EASY;
PingCmd ping = PING_CMD__INIT;
cmd.body_case = CTP_CMD__BODY_PING;
cmd.ping = &ping;

uint8_t *out = NULL;
size_t  len  = protobuf_c_port_pack(&cmd.base, &out);
// 发送 out/len ...

// ===== 设备侧：解包并应答 =====
CTPCmd *recv = ctp_cmd__unpack(NULL, len, buf);
// 匹配 id, 执行指令
PingRes res = PING_RES__INIT;
res.alive = true;
CTPRes resp = CTP_RES__INIT;
resp.id = recv->id;
resp.body_case = CTP_RES__BODY_PING;
resp.ping = &res;
ctp_cmd__free_unpacked(recv, NULL);

uint8_t *out2 = NULL;
size_t  len2  = protobuf_c_port_pack(&resp.base, &out2);
free(out2);
free(out);
```

## 上下文管理 (CMD_COMP)

复杂指令需设备端创建上下文，上下文通过链表统一管理。`handle` 用于关联后续指令与已创建的上下文。上下文的生命周期由设备端控制，指令完成后释放。
