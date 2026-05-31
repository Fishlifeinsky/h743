cmake_minimum_required(VERSION 3.20)

#==============================================================================
# FatFs static library for STM32H743 (SD card via SDMMC1)
#
# 移植文件:
#   - ffconf.h     → BSP/fatfs_port/include/ (FatFs 配置)
#   - diskio.c     → 3rd/fatfs/ (胶水层, 调用 fatfs_port_sd_*)
#   - fatfs_port.c → BSP/fatfs_port/ (硬件实现, 由 BSP CMake 编译)
#==============================================================================

set(FATFS_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(FATFS_PORT_INC ${CMAKE_SOURCE_DIR}/platform/BSP/fatfs_port/include)

# 检查 BSP/fatfs_port/include 是否存在
if(NOT EXISTS "${FATFS_PORT_INC}")
    message(FATAL_ERROR "BSP/fatfs_port/include not found — fatfs_port module is required")
endif()

#--------------------------------------------------------------------
# FatFs core + diskio glue layer
#--------------------------------------------------------------------
set(FATFS_CORE_SRC
    ${FATFS_DIR}/ff.c
    ${FATFS_DIR}/ffsystem.c
    ${FATFS_DIR}/ffunicode.c
    ${FATFS_DIR}/diskio.c          # 胶水层: disk_* → fatfs_port_sd_*
)

#--------------------------------------------------------------------
# Static library target
#--------------------------------------------------------------------
add_library(fatfs STATIC
    ${FATFS_CORE_SRC}
)

#--------------------------------------------------------------------
# Include directories
#--------------------------------------------------------------------
target_include_directories(fatfs PUBLIC
    ${FATFS_DIR}                    # ff.h
    ${FATFS_PORT_INC}               # ffconf.h + fatfs_port.h
    ${CMAKE_SOURCE_DIR}/platform    # BSP/config.h
)

#--------------------------------------------------------------------
# Suppress warnings on 3rd-party code
#--------------------------------------------------------------------
if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set_source_files_properties(
        ${FATFS_CORE_SRC}
        PROPERTIES COMPILE_FLAGS "-w"
    )
endif()
