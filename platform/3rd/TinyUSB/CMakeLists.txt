cmake_minimum_required(VERSION 3.20)

#==============================================================================
# TinyUSB static library for STM32H743 (VENDOR + MSC Device, DWC2 USB OTG FS)
#==============================================================================

set(TINYUSB_DIR ${CMAKE_CURRENT_SOURCE_DIR})

#--------------------------------------------------------------------
# TinyUSB core — device only, CDC class only
#--------------------------------------------------------------------
set(TINYUSB_CORE_SRC
    ${TINYUSB_DIR}/src/tusb.c
    ${TINYUSB_DIR}/src/common/tusb_fifo.c
    ${TINYUSB_DIR}/src/device/usbd.c
    ${TINYUSB_DIR}/src/device/usbd_control.c
    ${TINYUSB_DIR}/src/class/vendor/vendor_device.c
    ${TINYUSB_DIR}/src/class/msc/msc_device.c
)

#--------------------------------------------------------------------
# Synopsys DWC2 — device driver only
#--------------------------------------------------------------------
set(TINYUSB_DWC2_SRC
    ${TINYUSB_DIR}/src/portable/synopsys/dwc2/dcd_dwc2.c
    ${TINYUSB_DIR}/src/portable/synopsys/dwc2/dwc2_common.c
)

#--------------------------------------------------------------------
# STM32H7 BSP — minimal port (family.c only)
#--------------------------------------------------------------------
set(TINYUSB_BSP_SRC
    ${TINYUSB_DIR}/hw/bsp/stm32h7/family.c
)

#--------------------------------------------------------------------
# Static library target
#--------------------------------------------------------------------
add_library(tinyusb STATIC
    ${TINYUSB_CORE_SRC}
    ${TINYUSB_DWC2_SRC}
    ${TINYUSB_BSP_SRC}
)

#--------------------------------------------------------------------
# Include directories
#--------------------------------------------------------------------
target_include_directories(tinyusb PUBLIC
    ${TINYUSB_DIR}/src
    ${TINYUSB_DIR}/hw              # for "bsp/board_api.h"
    ${CMAKE_SOURCE_DIR}/platform/BSP/tinyusb_port/include  # tusb_config.h + tusb_port.h
)

#--------------------------------------------------------------------
# Compile definitions
#--------------------------------------------------------------------
target_compile_definitions(tinyusb PUBLIC
    CFG_TUSB_MCU=OPT_MCU_STM32H7
    CFG_TUSB_CONFIG_FILE="tusb_config.h"
)

#--------------------------------------------------------------------
# family.c: suppress missing-prototypes, disable handlers provided by CubeMX
#--------------------------------------------------------------------
set_source_files_properties(${TINYUSB_DIR}/hw/bsp/stm32h7/family.c
    PROPERTIES COMPILE_FLAGS "-Wno-missing-prototypes -DSYSTICK_HANDLER_PROVIDED -DHARDFAULT_HANDLER_PROVIDED -DINIT_PROVIDED"
)

#--------------------------------------------------------------------
# Suppress warnings on 3rd-party code
#--------------------------------------------------------------------
if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set_source_files_properties(
        ${TINYUSB_CORE_SRC}
        ${TINYUSB_DWC2_SRC}
        ${TINYUSB_BSP_SRC}
        PROPERTIES COMPILE_FLAGS "-w"
    )
endif()
