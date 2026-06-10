#ifndef CTP_HELPER_H
#define CTP_HELPER_H

#include "ctp.pb-c.h"
#include "ctpb.h"

#ifdef __cplusplus
extern "C" {
#endif

// =====================================================================
// 通用 protobuf-c 宏 (与具体 proto 无关)
// =====================================================================

// 调用 name__init(msg)
#define PB_INIT(name, msg)           name##__init(msg)

// 栈上声明并初始化消息变量
#define PB_DECL(Type, var)           Type var = Type##__INIT

// 解包 (NULL 分配器走 wrap -> TLSF)
#define PB_UNPACK(name, len, data)   name##__unpack(NULL, (len), (data))

// 释放解包消息
#define PB_FREE(name, msg)           name##__free_unpacked((msg), NULL)

// 打包到动态分配 buffer (失败返回 0)
#define PB_PACK(msg, out)            ctpb_pack(&(msg)->base, (out))

// =====================================================================
// CTP 帧构造宏 (通用, 不随 proto 新增指令而修改)
//
// 参数约定 — 以 PingCmd 为例:
//   Body  = PingCmd     (proto message 类型名)
//   field = ping         (oneof 联合体字段名, 小写)
//   BODY  = PING         (body_case 枚举后缀, 大写)
//
// 用法:
//   CTP_CMD(cmd, 1, CMD_TYPE__CMD_EASY, PingCmd, ping, PING);
//   cmd_body 即为 PingCmd 子消息
//
//   CTP_RES(res, cmd.id, PingRes, ping, PING);
//   res_body 即为 PingRes 子消息
// =====================================================================

#define CTP_CMD(var, _id, _type, Body, field, BODY)  \
    PB_DECL(Body, var##_body);                       \
    PB_DECL(CTPCmd, var);                            \
    var.id = (_id);                                  \
    var.type = (_type);                              \
    var.body_case = CTPCMD__BODY_##BODY;             \
    var.field = &var##_body

#define CTP_RES(var, _id, Body, field, BODY)         \
    PB_DECL(Body, var##_body);                       \
    PB_DECL(CTPRes, var);                            \
    var.id = (_id);                                  \
    var.body_case = CTPRES__BODY_##BODY;             \
    var.field = &var##_body

// 从命令复制 id, body 自行处理
#define CTP_RES_FROM_CMD(var, cmd_ptr)               \
    PB_DECL(CTPRes, var);                            \
    var.id = (cmd_ptr)->id

#ifdef __cplusplus
}
#endif

#endif
