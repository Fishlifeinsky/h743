cmake_minimum_required(VERSION 3.20)

set(TLSF_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_library(tlsf STATIC
    ${TLSF_DIR}/tlsf.c
)

target_include_directories(tlsf PUBLIC
    ${TLSF_DIR}
)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set_source_files_properties(${TLSF_DIR}/tlsf.c
        PROPERTIES COMPILE_FLAGS "-w"
    )
endif()
