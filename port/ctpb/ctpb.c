#include "ctpb.h"
#include "ctpb_frame.h"
#include "port/version.h"

#include <stdlib.h>

#include "ctp.pb-c.h"

//--------------------------------------------------------------------+
// 内部全局变量
//--------------------------------------------------------------------+

// 指令处理函数类型
typedef void (*cmd_handler_t)(const CTPCmd *cmd);

#define CTP_CMD_COUNT  (CTPCMD__BODY_FLASH_ABORT + 1)
static cmd_handler_t g_handlers[CTP_CMD_COUNT];

// 传输层发送回调
static ctpb_send_cb_t g_send_cb;

//--------------------------------------------------------------------+
// 辅助函数
//--------------------------------------------------------------------+

// 打包响应帧并发送
static void send_response(const CTPRes *res) {
    if (!g_send_cb) return;
    uint8_t *out = NULL;
    size_t len = ctpb_frame_pack(&res->base, &out);
    if (out && len) {
        g_send_cb(out, len);
        free(out);
    }
}

//--------------------------------------------------------------------+
// 简单指令处理函数
//--------------------------------------------------------------------+

static void handle_ping(const CTPCmd *cmd) {
    PingRes body = PING_RES__INIT;
    CTPRes  res  = CTPRES__INIT;

    res.id        = cmd->id;
    res.body_case = CTPRES__BODY_PING;
    res.ping      = &body;

    body.alive = true;
    send_response(&res);
}

static void handle_version(const CTPCmd *cmd) {
    VersionRes body = VERSION_RES__INIT;
    CTPRes     res  = CTPRES__INIT;

    res.id        = cmd->id;
    res.body_case = CTPRES__BODY_VERSION;
    res.version   = &body;

    body.version = (char *)PORT_VERSION_STR;
    send_response(&res);
}

static void handle_flash_info(const CTPCmd *cmd) {
    FlashInfoRes body = FLASH_INFO_RES__INIT;
    CTPRes       res  = CTPRES__INIT;

    res.id         = cmd->id;
    res.body_case  = CTPRES__BODY_FLASH_INFO;
    res.flash_info = &body;

    // TODO: 查询实际 Flash 信息
    body.present = false;
    body.size    = 0;
    body.mid     = 0;
    body.did     = 0;
    send_response(&res);
}

static void handle_buff_info(const CTPCmd *cmd) {
    BuffInfoRes body = BUFF_INFO_RES__INIT;
    CTPRes      res  = CTPRES__INIT;

    res.id        = cmd->id;
    res.body_case = CTPRES__BODY_BUFF_INFO;
    res.buff_info = &body;

    body.rx_size = CTPB_FRAME_BUF_SIZE;
    body.tx_size = CTPB_FRAME_BUF_SIZE;
    send_response(&res);
}

//--------------------------------------------------------------------+
// 复杂指令处理函数 (待填充)
//--------------------------------------------------------------------+

static void handle_flash_load(const CTPCmd *cmd)   { (void)cmd; }
static void handle_flash_data(const CTPCmd *cmd)   { (void)cmd; }
static void handle_flash_commit(const CTPCmd *cmd) { (void)cmd; }
static void handle_flash_abort(const CTPCmd *cmd)  { (void)cmd; }

//--------------------------------------------------------------------+
// 外部接口
//--------------------------------------------------------------------+

void ctpb_init(void) {
    ctpb_frame_init();

    g_handlers[CTPCMD__BODY_PING]       = handle_ping;
    g_handlers[CTPCMD__BODY_VERSION]    = handle_version;
    g_handlers[CTPCMD__BODY_FLASH_INFO] = handle_flash_info;
    g_handlers[CTPCMD__BODY_BUFF_INFO]  = handle_buff_info;

    g_handlers[CTPCMD__BODY_FLASH_LOAD]   = handle_flash_load;
    g_handlers[CTPCMD__BODY_FLASH_DATA]   = handle_flash_data;
    g_handlers[CTPCMD__BODY_FLASH_COMMIT] = handle_flash_commit;
    g_handlers[CTPCMD__BODY_FLASH_ABORT]  = handle_flash_abort;
}

void ctpb_rtos_init(void) {
    ctpb_frame_rtos_init();
}

void ctpb_set_send_cb(ctpb_send_cb_t cb) {
    g_send_cb = cb;
}

void ctpb_poll(void) {
    uint8_t buf[CTPB_FRAME_BUF_SIZE];

    size_t len = ctpb_frame_recv(buf, sizeof(buf), 0);
    if (len == 0) return;

    CTPCmd *cmd = ctpcmd__unpack(NULL, len, buf);
    if (!cmd) return;

    uint32_t idx = cmd->body_case;
    if (idx < CTP_CMD_COUNT && g_handlers[idx]) {
        g_handlers[idx](cmd);
    }

    ctpcmd__free_unpacked(cmd, NULL);
}
