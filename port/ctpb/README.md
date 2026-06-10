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

## 辅助宏 (include/ctp_helper.h)

### 通用宏 (与 proto 无关)

| 宏 | 说明 |
|---|---|
| `PB_DECL(Type, var)` | 栈上声明并初始化消息 |
| `PB_INIT(name, msg)` | 调用 `name__init(msg)` |
| `PB_UNPACK(name, len, data)` | 解包 |
| `PB_PACK(msg, out)` | 打包到动态 buffer |
| `PB_FREE(name, msg)` | 释放解包消息 |

### CTP 帧构造宏 (通用，新增指令无需改宏)

```
CTP_CMD(var, id, type, Body, field, BODY)
CTP_RES(var, id, Body, field, BODY)
```

参数约定以 `PingCmd` 为例：
- `Body` = `PingCmd` (message 类型)
- `field` = `ping` (oneof 字段名，小写)
- `BODY` = `PING` (body_case 枚举后缀，大写)

生成变量：`var` (顶层帧), `var_body` (子消息)。

## 使用示例

```c
#include "ctp_helper.h"

// ===== 主机侧：构造并发送命令 =====
CTP_CMD(cmd, 1, CMD_TYPE__CMD_EASY, PingCmd, ping, PING);

uint8_t *out = NULL;
size_t  len  = PB_PACK(&cmd, &out);
// 发送 out/len ...

// ===== 设备侧：接收并应答 =====
CTPCmd *recv = PB_UNPACK(ctpcmd, len, buf);

CTP_RES(resp, recv->id, PingRes, ping, PING);
resp_body.alive = true;

uint8_t *out2 = NULL;
size_t  len2  = PB_PACK(&resp, &out2);

PB_FREE(ctpcmd, recv);
free(out);
free(out2);

// ===== Version 指令同理 =====
CTP_CMD(vcmd, 2, CMD_TYPE__CMD_EASY, VersionCmd, version, VERSION);
// 发送...

CTP_RES(vres, vcmd.id, VersionRes, version, VERSION);
vres_body.version = "1.0.0";
```

## 上下文管理 (CMD_COMP)

复杂指令需设备端创建上下文，上下文通过链表统一管理。`handle` 用于关联后续指令与已创建的上下文。上下文的生命周期由设备端控制，指令完成后释放。
