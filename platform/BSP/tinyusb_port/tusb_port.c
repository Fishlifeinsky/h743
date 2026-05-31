#include "tusb_port.h"
#include "tusb.h"
#include "bsp/board_api.h"
#include "stm32h7xx_hal.h"

//--------------------------------------------------------------------+
// 内部全局变量 — USB 描述符
//--------------------------------------------------------------------+

// 设备描述符
static const tusb_desc_device_t desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0,
    .bDeviceSubClass    = 0,
    .bDeviceProtocol    = 0,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = 0xCafe,
    .idProduct          = 0x4000,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01,
};

// 配置描述符: VENDOR + MSC
enum {
    ITF_NUM_VD  = 0,
    ITF_NUM_MSC = 1,
    ITF_COUNT
};

#define CONFIG_TOTAL_LEN    (TUD_CONFIG_DESC_LEN + TUD_VENDOR_DESC_LEN + TUD_MSC_DESC_LEN)
#define EPNUM_VD_OUT        0x02
#define EPNUM_VD_IN         0x82
#define EPNUM_MSC_OUT       0x03
#define EPNUM_MSC_IN        0x83

static const uint8_t desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_COUNT, 0, CONFIG_TOTAL_LEN,
        TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_VENDOR_DESCRIPTOR(ITF_NUM_VD, 0, EPNUM_VD_OUT, EPNUM_VD_IN, 64),
    TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 4, EPNUM_MSC_OUT, EPNUM_MSC_IN, 64)
};

//--------------------------------------------------------------------+
// TinyUSB 回调 (固定名称, 由 TinyUSB 库内部调用)
//--------------------------------------------------------------------+

const uint8_t *tud_descriptor_device_cb(void) {
    return (const uint8_t *)&desc_device;
}

const uint8_t *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return desc_configuration;
}

const uint16_t *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;
    static uint16_t desc_str[32];

    uint8_t count = 0;
    const char *ascii = NULL;

    switch (index) {
    case 0: // 语言 ID
        desc_str[1] = 0x0409; // US English
        count = 1;
        break;

    case 1: // 厂商
        ascii = "STM32H743";
        break;

    case 2: // 产品
        ascii = "TinyUSB VD";
        break;

    case 3: // 序列号 (从 UID 读取)
        count = board_usb_get_serial(&desc_str[1], 31);
        break;

    case 4: // MSC 接口
        ascii = "SD MSC";
        break;

    default:
        return NULL;
    }

    if (ascii) {
        count = (uint8_t)strlen(ascii);
        if (count > 31) count = 31;
        for (uint8_t i = 0; i < count; i++) {
            desc_str[1 + i] = ascii[i]; // char → UTF-16LE
        }
    }

    // 首字节: 长度(低字节) | 描述符类型(高字节)
    desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 + count * 2));

    return desc_str;
}

//--------------------------------------------------------------------+
// 外部接口
//--------------------------------------------------------------------+

int tusb_port_init(void) {
    // 使能 USB OTG_FS 中断 (DWC2 只用 OTG_FS_IRQn 一条中断线)
    // 优先级 5 = configMAX_SYSCALL_INTERRUPT_PRIORITY, 可安全调用 FreeRTOS API
    HAL_NVIC_SetPriority(OTG_FS_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(OTG_FS_IRQn);

    if (tusb_init()) {
        DEBUG_PRINT("TUSB: init ok\r\n");
        return 0;
    }
    DEBUG_PRINT("TUSB: init failed\r\n");
    return -1;
}

void tusb_port_task(void) {
    tud_task();
}
