cmake_minimum_required(VERSION 3.20)

#==============================================================================
# platform — 单个静态库，聚合 3rd + BSP + LIB
#==============================================================================

set(PLATFORM_DIR ${CMAKE_CURRENT_SOURCE_DIR})

#==============================================================================
# 1. 收集源文件
#==============================================================================

# --- 3rd/TinyUSB ---
set(TINYUSB_DIR ${PLATFORM_DIR}/3rd/TinyUSB)
set(TINYUSB_SOURCES
    ${TINYUSB_DIR}/src/tusb.c
    ${TINYUSB_DIR}/src/common/tusb_fifo.c
    ${TINYUSB_DIR}/src/device/usbd.c
    ${TINYUSB_DIR}/src/device/usbd_control.c
    ${TINYUSB_DIR}/src/class/vendor/vendor_device.c
    ${TINYUSB_DIR}/src/class/msc/msc_device.c
    ${TINYUSB_DIR}/src/portable/synopsys/dwc2/dcd_dwc2.c
    ${TINYUSB_DIR}/src/portable/synopsys/dwc2/dwc2_common.c
    ${TINYUSB_DIR}/hw/bsp/stm32h7/family.c
)

# --- 3rd/tlsf ---
set(TLSF_DIR ${PLATFORM_DIR}/3rd/tlsf)
set(TLSF_SOURCES ${TLSF_DIR}/tlsf.c)

# --- 3rd/fatfs ---
set(FATFS_DIR ${PLATFORM_DIR}/3rd/fatfs)
set(FATFS_SOURCES
    ${FATFS_DIR}/ff.c
    ${FATFS_DIR}/ffsystem.c
    ${FATFS_DIR}/ffunicode.c
    ${FATFS_DIR}/diskio.c
)

# --- 3rd/FreeRTOS ---
set(FREERTOS_DIR ${PLATFORM_DIR}/3rd/FreeRtos)
set(FREERTOS_SOURCES
    ${FREERTOS_DIR}/tasks.c
    ${FREERTOS_DIR}/queue.c
    ${FREERTOS_DIR}/list.c
    ${FREERTOS_DIR}/timers.c
    ${FREERTOS_DIR}/event_groups.c
    ${FREERTOS_DIR}/stream_buffer.c
    ${FREERTOS_DIR}/portable/MemMang/heap_3.c
)

# --- 3rd/lvgl ---
set(LVGL_DIR ${PLATFORM_DIR}/3rd/lvgl)

# LVGL 核心 + 全部子系统 (GLOB_RECURSE, gc-sections 自动裁掉未用文件)
file(GLOB_RECURSE LVGL_SOURCES "${LVGL_DIR}/src/*.c")

# --- BSP 模块 ---
file(GLOB BSP_MODULES LIST_DIRECTORIES true ${PLATFORM_DIR}/BSP/*)

set(BSP_SOURCES "")
set(BSP_INCLUDES "")

foreach(MOD ${BSP_MODULES})
    if(NOT IS_DIRECTORY ${MOD})
        continue()
    endif()

    get_filename_component(MOD_NAME ${MOD} NAME)

    file(GLOB MOD_SOURCES "${MOD}/*.c" "${MOD}/*.cpp" "${MOD}/*.S" "${MOD}/*.s")

    if(EXISTS "${MOD}/include")
        set(MOD_INCLUDE "${MOD}/include")
    else()
        set(MOD_INCLUDE "${MOD}")
    endif()

    list(APPEND BSP_SOURCES ${MOD_SOURCES})
    list(APPEND BSP_INCLUDES ${MOD_INCLUDE})

    message(STATUS "BSP: ${MOD_NAME} -> ${MOD_SOURCES}")
endforeach()

# 添加 platform 目录以支持 #include "BSP/config.h"
list(APPEND BSP_INCLUDES ${PLATFORM_DIR})

# --- LIB ---
set(ELIB_DIR ${PLATFORM_DIR}/LIB/elib)
set(VFS_DIR ${PLATFORM_DIR}/LIB/vfs)

#==============================================================================
# 2. 单个 platform 静态库
#==============================================================================

add_library(platform STATIC
    ${TINYUSB_SOURCES}
    ${TLSF_SOURCES}
    ${FATFS_SOURCES}
    ${FREERTOS_SOURCES}
    ${LVGL_SOURCES}
    ${BSP_SOURCES}
    ${ELIB_DIR}/list.c
    ${VFS_DIR}/vfs.c
)

#==============================================================================
# 3. Include 目录 (全部 PUBLIC)
#==============================================================================

target_include_directories(platform PUBLIC
    # TinyUSB
    ${TINYUSB_DIR}/src
    ${TINYUSB_DIR}/hw
    ${PLATFORM_DIR}/BSP/tinyusb_port/include
    # tlsf
    ${TLSF_DIR}
    # fatfs
    ${FATFS_DIR}
    ${PLATFORM_DIR}/BSP/fatfs_port/include
    # lvgl
    ${LVGL_DIR}
    # BSP
    ${BSP_INCLUDES}
    # elib ( "elib/list.h" )
    ${ELIB_DIR}/..
    # vfs
    ${VFS_DIR}
    ${VFS_DIR}/..
    # FreeRTOS
    ${FREERTOS_DIR}/include
)

#==============================================================================
# 4. 编译定义 / 选项
#==============================================================================

target_compile_definitions(platform PUBLIC
    CFG_TUSB_MCU=OPT_MCU_STM32H7
    CFG_TUSB_CONFIG_FILE="tusb_config.h"
    __STM32_VFS__
)

#==============================================================================
# 5. 全局 malloc/free 替换为 TLSF (via linker --wrap)
#==============================================================================

target_link_options(platform PUBLIC
    -Wl,--wrap=malloc
    -Wl,--wrap=free
    -Wl,--wrap=realloc
    -Wl,--wrap=calloc
)

#==============================================================================
# 6. 特殊源文件属性
#==============================================================================

# family.c: suppress missing-prototypes, disable handlers provided by CubeMX
set_source_files_properties(${TINYUSB_DIR}/hw/bsp/stm32h7/family.c
    PROPERTIES COMPILE_FLAGS "-Wno-missing-prototypes -DSYSTICK_HANDLER_PROVIDED -DHARDFAULT_HANDLER_PROVIDED -DINIT_PROVIDED"
)

# port.c: suppress warnings, indicate handlers are provided by FreeRTOS
set_source_files_properties(${PLATFORM_DIR}/BSP/freertos_port/port.c
    PROPERTIES COMPILE_FLAGS "-Wno-missing-prototypes -DSYSTICK_HANDLER_PROVIDED -DPENDSV_HANDLER_PROVIDED -DSVC_HANDLER_PROVIDED"
)

# Suppress warnings on 3rd-party code
if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set_source_files_properties(
        ${TINYUSB_SOURCES}
        ${TLSF_SOURCES}
        ${FATFS_SOURCES}
        ${FREERTOS_SOURCES}
        ${LVGL_SOURCES}
        PROPERTIES COMPILE_FLAGS "-w"
    )
endif()
