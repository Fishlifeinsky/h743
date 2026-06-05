#此文件从模板自动生成! 请勿更改!
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
cmake_minimum_required(VERSION 3.20)
# specify cross-compilers and tools
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER  arm-none-eabi-gcc)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(SIZE arm-none-eabi-size)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# project settings
project(h743 C CXX ASM)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

# Hardware floating point (Cortex-M7 with double-precision FPU)
add_compile_definitions(ARM_MATH_CM7;ARM_MATH_MATRIX_CHECK;ARM_MATH_ROUNDING)
add_compile_options(-mfloat-abi=hard -mfpu=fpv5-d16)
add_link_options(-mfloat-abi=hard -mfpu=fpv5-d16)

add_compile_options(-mcpu=cortex-m7 -mthumb -mthumb-interwork)
add_compile_options(-ffunction-sections -fdata-sections -fno-common -fmessage-length=0)

# Enable assembler files preprocessing
add_compile_options($<$<COMPILE_LANGUAGE:ASM>:-x$<SEMICOLON>assembler-with-cpp>)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    message(STATUS "Maximum optimization for speed")
    add_compile_options(-Ofast)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    message(STATUS "Maximum optimization for speed, debug info included")
    add_compile_options(-Ofast -g)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
    message(STATUS "Maximum optimization for size")
    add_compile_options(-Os)
else ()
    message(STATUS "Minimal optimization, debug info included")
    add_compile_options(-Og -g)
endif ()

include_directories(Core/Inc Drivers/STM32H7xx_HAL_Driver/Inc Drivers/STM32H7xx_HAL_Driver/Inc/Legacy Drivers/CMSIS/Device/ST/STM32H7xx/Include Drivers/CMSIS/Include)

add_definitions(-DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H743xx -DSYSTICK_HANDLER_PROVIDED -DHARDFAULT_HANDLER_PROVIDED -DINIT_PROVIDED)

#==============================================================================
# platform 静态库 = external + port + middleware
#==============================================================================

set(EXT_DIR ${CMAKE_SOURCE_DIR}/external)

# --- external/TinyUSB ---
set(TINYUSB_DIR ${EXT_DIR}/TinyUSB)
set(EXT_SOURCES
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

set(EXT_INCLUDES
    ${TINYUSB_DIR}/src
    ${TINYUSB_DIR}/hw
)

# --- external/tlsf ---
set(TLSF_DIR ${EXT_DIR}/tlsf)
list(APPEND EXT_SOURCES ${TLSF_DIR}/tlsf.c)
list(APPEND EXT_INCLUDES ${TLSF_DIR})

# --- external/fatfs ---
set(FATFS_DIR ${EXT_DIR}/fatfs)
list(APPEND EXT_SOURCES
    ${FATFS_DIR}/ff.c
    ${FATFS_DIR}/ffsystem.c
    ${FATFS_DIR}/ffunicode.c
    ${FATFS_DIR}/diskio.c
)
list(APPEND EXT_INCLUDES ${FATFS_DIR})

# --- external/FreeRTOS ---
set(FREERTOS_DIR ${EXT_DIR}/FreeRtos)
list(APPEND EXT_SOURCES
    ${FREERTOS_DIR}/tasks.c
    ${FREERTOS_DIR}/queue.c
    ${FREERTOS_DIR}/list.c
    ${FREERTOS_DIR}/timers.c
    ${FREERTOS_DIR}/event_groups.c
    ${FREERTOS_DIR}/stream_buffer.c
    ${FREERTOS_DIR}/portable/MemMang/heap_3.c
)
list(APPEND EXT_INCLUDES ${FREERTOS_DIR}/include)

# --- external/lvgl ---
set(LVGL_DIR ${EXT_DIR}/lvgl)
file(GLOB_RECURSE LVGL_SOURCES "${LVGL_DIR}/src/*.c")
list(APPEND EXT_SOURCES ${LVGL_SOURCES})
list(APPEND EXT_INCLUDES ${LVGL_DIR})

# --- port 模块 (遍历 port/*/module.cmake) ---
file(GLOB PORT_MODULE_FILES ${CMAKE_SOURCE_DIR}/port/*/module.cmake)
foreach(MOD_FILE ${PORT_MODULE_FILES})
    include(${MOD_FILE})
    list(APPEND PORT_SOURCES ${MODULE_SOURCES})
    list(APPEND PORT_INCLUDES ${MODULE_INCLUDES})
    get_filename_component(MOD_DIR ${MOD_FILE} DIRECTORY)
    get_filename_component(MOD_NAME ${MOD_DIR} NAME)
    message(STATUS "port/${MOD_NAME}: ${MODULE_SOURCES}")
endforeach()
list(APPEND PORT_INCLUDES ${CMAKE_SOURCE_DIR})  # 支持 #include "port/config.h"

# --- middleware 模块 (遍历 middleware/*/module.cmake) ---
file(GLOB MW_MODULE_FILES ${CMAKE_SOURCE_DIR}/middleware/*/module.cmake)
foreach(MOD_FILE ${MW_MODULE_FILES})
    include(${MOD_FILE})
    list(APPEND MW_SOURCES ${MODULE_SOURCES})
    list(APPEND MW_INCLUDES ${MODULE_INCLUDES})
endforeach()

#==============================================================================
# 构建 platform 库
#==============================================================================

add_library(platform STATIC
    ${EXT_SOURCES}
    ${PORT_SOURCES}
    ${MW_SOURCES}
)

target_include_directories(platform PUBLIC
    ${EXT_INCLUDES}
    ${CMAKE_SOURCE_DIR}/port/tinyusb_port/include
    ${CMAKE_SOURCE_DIR}/port/fatfs_port/include
    ${PORT_INCLUDES}
    ${MW_INCLUDES}
)

target_compile_definitions(platform PUBLIC
    CFG_TUSB_MCU=OPT_MCU_STM32H7
    CFG_TUSB_CONFIG_FILE="tusb_config.h"
    __STM32_VFS__
)

target_link_options(platform PUBLIC
    -Wl,--wrap=malloc
    -Wl,--wrap=free
    -Wl,--wrap=realloc
    -Wl,--wrap=calloc
)

# 特殊源文件属性
set_source_files_properties(${TINYUSB_DIR}/hw/bsp/stm32h7/family.c
    PROPERTIES COMPILE_FLAGS "-Wno-missing-prototypes -DSYSTICK_HANDLER_PROVIDED -DHARDFAULT_HANDLER_PROVIDED -DINIT_PROVIDED"
)
set_source_files_properties(${CMAKE_SOURCE_DIR}/port/freertos_port/port.c
    PROPERTIES COMPILE_FLAGS "-Wno-missing-prototypes -DSYSTICK_HANDLER_PROVIDED -DPENDSV_HANDLER_PROVIDED -DSVC_HANDLER_PROVIDED"
)

# 第三方库抑制警告
if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set_source_files_properties(${EXT_SOURCES} PROPERTIES COMPILE_FLAGS "-w")
endif()

#==============================================================================
# 主程序
#==============================================================================

file(GLOB_RECURSE SOURCES "Core/*.*" "Drivers/*.*")

set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/STM32H743IITX_FLASH.ld)

add_link_options(-Wl,-gc-sections,--print-memory-usage,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map)
add_link_options(-mcpu=cortex-m7 -mthumb -mthumb-interwork)
add_link_options(-T ${LINKER_SCRIPT})

add_executable(${PROJECT_NAME}.elf ${SOURCES} ${LINKER_SCRIPT})
target_link_libraries(${PROJECT_NAME}.elf -Wl,--whole-archive platform -Wl,--no-whole-archive)

set(HEX_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin)
set(EX_BIN_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}_ex.bin)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
        # 内部 FLASH: 排除 LMA 在 EX_FLASH 的段
        COMMAND ${CMAKE_OBJCOPY} -Obinary -R .sdram_code
        $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
        # 外部 FLASH: 只取 LMA 在 EX_FLASH 的段
        COMMAND ${CMAKE_OBJCOPY} -Obinary -j .sdram_code
        $<TARGET_FILE:${PROJECT_NAME}.elf> ${EX_BIN_FILE}
        COMMENT "Building ${HEX_FILE}\nBuilding ${BIN_FILE} (internal FLASH)\nBuilding ${EX_BIN_FILE} (external FLASH)"
)
