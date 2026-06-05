#ifndef TUSB_CONFIG_H_
#define TUSB_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------+
// VBUS / ID pin
//--------------------------------------------------------------------+
// VBUS sense: 0 = disable (bus-powered device)
#define OTG_FS_VBUS_SENSE     0
#define OTG_HS_VBUS_SENSE     0

// PA10 is used as UART1_RX — skip USB ID pin
#define BOARD_NO_USB_ID_PIN

//--------------------------------------------------------------------+
// MCU & OS
//--------------------------------------------------------------------+
#define CFG_TUSB_MCU                OPT_MCU_STM32H7
#define CFG_TUSB_OS                 OPT_OS_NONE

//--------------------------------------------------------------------+
// Root Hub Port 0: OTG_FS (PA11/PA12, Full Speed, Device mode)
// CFG_TUSB_RHPORT0_MODE is REQUIRED — tusb_init() will static_assert without it.
// Bitmask: OPT_MODE_DEVICE(0x0001) | OPT_MODE_HOST(0x0002) | OPT_MODE_FULL_SPEED(0x0200) | OPT_MODE_HIGH_SPEED(0x0400)
//--------------------------------------------------------------------+
#define CFG_TUSB_RHPORT0_MODE       (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)

// Derived defaults (can override, but CFG_TUSB_RHPORT0_MODE is the source of truth)
// #define CFG_TUD_ENABLED             1   // auto: RHPORT0_MODE & OPT_MODE_DEVICE
// #define CFG_TUH_ENABLED             0   // auto: RHPORT0_MODE & OPT_MODE_HOST

// BSP board config (used by hw/bsp/stm32h7/family.c)
#define BOARD_TUD_RHPORT            0
#define BOARD_TUD_MAX_SPEED         OPT_MODE_FULL_SPEED
#define BOARD_TUH_RHPORT            0
#define BOARD_TUH_MAX_SPEED         OPT_MODE_FULL_SPEED

//--------------------------------------------------------------------+
// Device Classes (enable only what you need to save flash)
//--------------------------------------------------------------------+
#define CFG_TUD_CDC                 0
#define CFG_TUD_MSC                 1
#define CFG_TUD_HID                 0
#define CFG_TUD_MIDI                0
#define CFG_TUD_AUDIO               0
#define CFG_TUD_VIDEO               0
#define CFG_TUD_VENDOR              1
#define CFG_TUD_DFU                 0
#define CFG_TUD_DFU_RT              0
#define CFG_TUD_MTP                 0
#define CFG_TUD_ECM_RNDIS            0

//--------------------------------------------------------------------+
// MSC FIFO
#define CFG_TUD_MSC_EP_BUFSIZE      64

// VENDOR FIFO
//--------------------------------------------------------------------+
#define CFG_TUD_VENDOR_RX_BUFSIZE   64
#define CFG_TUD_VENDOR_TX_BUFSIZE   64

//--------------------------------------------------------------------+
// Memory
//--------------------------------------------------------------------+
#define CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_ALIGN          TU_ATTR_ALIGNED(4)

//--------------------------------------------------------------------+
// Debug level: 0=none, 1=error, 2=warning, 3=info, 4=debug
//--------------------------------------------------------------------+
#define CFG_TUSB_DEBUG              0

#ifdef __cplusplus
}
#endif

#endif
