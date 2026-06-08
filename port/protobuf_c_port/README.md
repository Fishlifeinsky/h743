# protobuf_c_port

protobuf-c 移植模块。基于 mem 模块的 `--wrap=malloc` 机制，unpack 传 `NULL` 分配器即可使用 TLSF 内存池。

## 生成 protobuf-c 文件

1. 编写 `.proto` 文件（如 `sensor.proto`）
2. 执行生成脚本：
   ```bat
   tools\gen_proto.bat sensor.proto
   ```
   或手动执行：
   ```bat
   set PATH=tools;%PATH%
   cd port\protobuf_c_port
   protoc --c_out=. your_file.proto
   ```
3. 重新编译项目，`module.cmake` 会通过 `file(GLOB *.pb-c.c)` 自动包含生成的 C 文件

## 使用示例

```c
#include "protobuf_c_port.h"
#include "sensor.pb-c.h"

// unpack — NULL 分配器走 malloc (已被 mem 模块 wrap 到 TLSF)
SensorData *msg = sensor_data__unpack(NULL, data_len, data_buf);

// pack — 用 port 提供的辅助函数
uint8_t *out = NULL;
size_t  len  = protobuf_c_port_pack(&msg->base, &out);

// 释放
free(out);
sensor_data__free_unpacked(msg, NULL);
```
