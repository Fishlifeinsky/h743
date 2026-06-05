#include "vendor.h"
#include "vendor_flash.h"
#include "tusb.h"
#include "mem.h"
#include "port/config.h"
#include "qspi_w25q64.h"
#include "quadspi.h"
#include <stdio.h>
#include <string.h>

//--------------------------------------------------------------------+
// 内部全局变量
//--------------------------------------------------------------------+

// 下载状态机
typedef enum {
    VENDOR_STATE_IDLE = 0,
    VENDOR_STATE_LOADED,    // LOAD 完成，等待 DATA
} vendor_state_t;

static vendor_state_t  g_state        = VENDOR_STATE_IDLE;
static uint32_t        g_load_addr    = 0;    // 目标写入地址
static uint32_t        g_load_size    = 0;    // 总下载字节数
static uint32_t        g_load_written = 0;    // 已写入字节数

// 命令行接收缓冲区
static uint8_t  g_rx_line_buf[VENDOR_RX_BUF_DEFAULT];
static uint32_t g_rx_line_len = 0;

// 扇区缓冲 (外部 Flash 扇区大小 4KB, 节省 RAM)
#define SECTOR_BUF_SIZE  VENDOR_FLASH_EXT_SECTOR_SIZE
static uint8_t  g_sector_buf[SECTOR_BUF_SIZE];
static uint32_t g_sector_fill = 0;

//--------------------------------------------------------------------+
// 静态函数 — hex 编解码 (ITCM)
//--------------------------------------------------------------------+

// hex 字符 → nibble (0-15), 失败返回 -1
static MEM_ITCM_SECTION int vendor_hex_nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

// hex 字符串 → 二进制，返回解码字节数，失败返回 -1
static MEM_ITCM_SECTION int vendor_hex_decode(const char *hex, uint8_t *out, int len) {
    int i, j = 0;
    for (i = 0; i < len; i += 2) {
        int hi = vendor_hex_nibble(hex[i]);
        int lo = vendor_hex_nibble(hex[i + 1]);
        if (hi < 0 || lo < 0) return -1;
        out[j++] = (uint8_t)((hi << 4) | lo);
    }
    return j;
}

//--------------------------------------------------------------------+
// 静态函数 — USB 响应发送
//--------------------------------------------------------------------+

static void vendor_respond(const char *cmd, const char *body) {
    char buf[VENDOR_TX_BUF_SIZE];
    int len;
    if (body && body[0]) {
        len = snprintf(buf, sizeof(buf), "%s%s=%s%s",
                       VENDOR_PROTO_PREFIX, cmd, body, VENDOR_PROTO_SUFFIX);
    } else {
        len = snprintf(buf, sizeof(buf), "%s%s%s",
                       VENDOR_PROTO_PREFIX, cmd, VENDOR_PROTO_SUFFIX);
    }
    if (len > 0 && len < (int)sizeof(buf)) {
        DEBUG_PRINT("[VENDOR TX] %s", buf);
        tud_vendor_n_write(0, buf, (uint32_t)len);
        tud_vendor_n_write_flush(0);
    }
}

//--------------------------------------------------------------------+
// 静态函数 — 扇区缓冲刷新
//--------------------------------------------------------------------+

static int vendor_sector_flush(void) {
    if (g_sector_fill == 0) return 0;

    int ret = vendor_flash_write(g_load_addr, g_sector_buf, g_sector_fill);
    if (ret == 0) {
        g_load_written += g_sector_fill;
        g_load_addr    += g_sector_fill;
        g_sector_fill   = 0;
    }
    return ret;
}

//--------------------------------------------------------------------+
// 静态函数 — 命令处理
//--------------------------------------------------------------------+

static void vendor_cmd_buff(const char *body) {
    if (body && body[0]) {
        vendor_respond(VENDOR_CMD_BUFF, body);
    } else {
        char buf[16];
        snprintf(buf, sizeof(buf), "%u", VENDOR_RX_BUF_DEFAULT);
        vendor_respond(VENDOR_CMD_BUFF, buf);
    }
}

static void vendor_cmd_flash(void) {
    vendor_flash_info_t info;
    vendor_flash_get_info(&info);

    char buf[64];
    if (info.present) {
        snprintf(buf, sizeof(buf), "%s,%lu,%02lX,%04lX",
                 VENDOR_STATUS_OK, info.size, info.mid, info.did);
    } else {
        snprintf(buf, sizeof(buf), "%s,0", VENDOR_STATUS_ERR);
    }
    vendor_respond(VENDOR_CMD_FLASH, buf);
}

static void vendor_cmd_load(const char *body) {
    if (g_state != VENDOR_STATE_IDLE) {
        vendor_respond(VENDOR_CMD_LOAD, VENDOR_STATUS_ERR);
        return;
    }

    uint32_t addr = 0, size = 0;
    const char *p = body;
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) p += 2;
    if (sscanf(p, "%lx,%lu", &addr, &size) != 2) {
        vendor_respond(VENDOR_CMD_LOAD, VENDOR_STATUS_ERR);
        return;
    }

    if (size == 0) {
        vendor_respond(VENDOR_CMD_LOAD, VENDOR_STATUS_ERR);
        return;
    }

    // 外部 Flash 需退出内存映射模式才能擦写
    if (addr >= VENDOR_FLASH_EXT_BASE &&
        addr <  VENDOR_FLASH_EXT_BASE + VENDOR_FLASH_EXT_SIZE) {
        HAL_QSPI_Abort(&hqspi);
    }

    // 擦除目标区域
    if (vendor_flash_erase(addr, size) != 0) {
        vendor_respond(VENDOR_CMD_LOAD, VENDOR_STATUS_ERR);
        return;
    }

    g_load_addr    = addr;
    g_load_size    = size;
    g_load_written = 0;
    g_sector_fill  = 0;
    g_state        = VENDOR_STATE_LOADED;

    vendor_respond(VENDOR_CMD_LOAD, VENDOR_STATUS_OK);
}

static void vendor_cmd_data(const char *body) {
    if (g_state != VENDOR_STATE_LOADED) {
        vendor_respond(VENDOR_CMD_DATA, VENDOR_STATUS_ERR);
        return;
    }

    if (!body || !body[0]) {
        vendor_respond(VENDOR_CMD_DATA, VENDOR_STATUS_ERR);
        return;
    }

    int hex_len = (int)strlen(body);

    // hex 字符串长度必须是偶数
    if (hex_len & 1) {
        vendor_respond(VENDOR_CMD_DATA, VENDOR_STATUS_ERR);
        return;
    }

    // 解码并写入扇区缓冲
    const char *src = body;
    int remaining   = hex_len;

    while (remaining > 0) {
        int space = (int)(SECTOR_BUF_SIZE - g_sector_fill);
        int chunk = (remaining / 2 < space) ? (remaining) : (space * 2);

        if (chunk == 0) {
            if (vendor_sector_flush() != 0) {
                vendor_respond(VENDOR_CMD_DATA, VENDOR_STATUS_ERR);
                g_state = VENDOR_STATE_IDLE;
                return;
            }
            continue;
        }

        int decoded = vendor_hex_decode(src, g_sector_buf + g_sector_fill, chunk);
        if (decoded < 0) {
            vendor_respond(VENDOR_CMD_DATA, VENDOR_STATUS_ERR);
            g_state = VENDOR_STATE_IDLE;
            return;
        }

        g_sector_fill += decoded;
        src           += chunk;
        remaining     -= chunk;

        if (g_sector_fill >= SECTOR_BUF_SIZE) {
            if (vendor_sector_flush() != 0) {
                vendor_respond(VENDOR_CMD_DATA, VENDOR_STATUS_ERR);
                g_state = VENDOR_STATE_IDLE;
                return;
            }
        }
    }

    // 校验是否超过 LOAD 声明的大小
    if (g_load_written + g_sector_fill > g_load_size) {
        vendor_respond(VENDOR_CMD_DATA, VENDOR_STATUS_ERR);
        g_state = VENDOR_STATE_IDLE;
        return;
    }

    vendor_respond(VENDOR_CMD_DATA, VENDOR_STATUS_OK);
}

static void vendor_cmd_abort(void) {
    // 刷新扇区缓冲中剩余数据
    if (g_state == VENDOR_STATE_LOADED && g_sector_fill > 0) {
        vendor_sector_flush();
    }

    // 恢复内存映射模式
    qspi_w25q64_memory_mapped_mode();

    g_state        = VENDOR_STATE_IDLE;
    g_load_addr    = 0;
    g_load_size    = 0;
    g_load_written = 0;
    g_sector_fill  = 0;
    vendor_respond(VENDOR_CMD_ABORT, VENDOR_STATUS_OK);
}

//--------------------------------------------------------------------+
// 静态函数 — 命令行分发
//--------------------------------------------------------------------+

static void vendor_dispatch(const char *line) {
    DEBUG_PRINT("[VENDOR RX] %s\r\n", line);

    // 检查前缀
    if (strncmp(line, VENDOR_PROTO_PREFIX, VENDOR_PROTO_PREFIX_LEN) != 0) return;

    // 跳过 "VB+"
    const char *cmd = line + VENDOR_PROTO_PREFIX_LEN;

    // 查找 '=' 分隔符
    const char *eq = strchr(cmd, '=');
    const char *body = NULL;
    char cmd_name[16];
    int cmd_len;

    if (eq) {
        cmd_len = (int)(eq - cmd);
        body    = eq + 1;
    } else {
        cmd_len = (int)strlen(cmd);
    }

    if (cmd_len >= (int)sizeof(cmd_name)) return;
    memcpy(cmd_name, cmd, cmd_len);
    cmd_name[cmd_len] = '\0';

    // 分发
    if (strcmp(cmd_name, VENDOR_CMD_BUFF) == 0) {
        vendor_cmd_buff(body);
    } else if (strcmp(cmd_name, VENDOR_CMD_FLASH) == 0) {
        vendor_cmd_flash();
    } else if (strcmp(cmd_name, VENDOR_CMD_LOAD) == 0) {
        vendor_cmd_load(body);
    } else if (strcmp(cmd_name, VENDOR_CMD_DATA) == 0) {
        vendor_cmd_data(body);
    } else if (strcmp(cmd_name, VENDOR_CMD_ABORT) == 0) {
        vendor_cmd_abort();
    }
}

//--------------------------------------------------------------------+
// 外部接口
//--------------------------------------------------------------------+

void vendor_init(void) {
    g_state        = VENDOR_STATE_IDLE;
    g_rx_line_len  = 0;
    g_sector_fill  = 0;
    g_load_written = 0;
}

void vendor_poll(void) {
    if (!tud_vendor_n_mounted(0)) return;

    // 读取 USB 数据到命令行缓冲
    uint32_t avail = tud_vendor_n_available(0);
    if (avail > 0) {
        DEBUG_PRINT("[VENDOR RAW] avail=%lu\r\n", avail);
    }
    while (avail > 0 && g_rx_line_len < sizeof(g_rx_line_buf) - 1) {
        uint32_t n = tud_vendor_n_read(0, g_rx_line_buf + g_rx_line_len,
                                       sizeof(g_rx_line_buf) - 1 - g_rx_line_len);
        if (n == 0) break;
        DEBUG_PRINT("  read %lu bytes\r\n", n);
        g_rx_line_len += n;
        avail -= n;
    }

    // 查找完整命令行 (\r\n)
    uint32_t i = 0;
    while (i + 1 < g_rx_line_len) {
        if (g_rx_line_buf[i] == '\r' && g_rx_line_buf[i + 1] == '\n') {
            g_rx_line_buf[i] = '\0';
            vendor_dispatch((char *)g_rx_line_buf);

            i += 2;  // 跳过 \r\n

            // 将剩余数据移到缓冲区开头
            uint32_t remaining = g_rx_line_len - i;
            if (remaining > 0 && i > 0) {
                memmove(g_rx_line_buf, &g_rx_line_buf[i], remaining);
            }
            g_rx_line_len = remaining;
            i = 0;
        } else {
            i++;
        }
    }

    // 缓冲区溢出保护
    if (g_rx_line_len >= sizeof(g_rx_line_buf) - 1) {
        g_rx_line_len = 0;  // 丢弃，等待重新同步
    }
}
