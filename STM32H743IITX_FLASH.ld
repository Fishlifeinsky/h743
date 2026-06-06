/*
******************************************************************************
**
**  File        : LinkerScript.ld
**
**  Author      : STM32CubeIDE
**
**  Abstract    : Linker script for STM32H7 series
**                2048Kbytes FLASH and 1056Kbytes RAM
**
**                Set heap size, stack size and stack location according
**                to application requirements.
**
**                Set memory bank area and size if external memory is used.
**
**  Target      : STMicroelectronics STM32
**
**  Distribution: The file is distributed as is, without any warranty
**                of any kind.
**
*****************************************************************************
** @attention
**
** Copyright (c) 2026 STMicroelectronics.
** All rights reserved.
**
** This software is licensed under terms that can be found in the LICENSE file
** in the root directory of this software component.
** If no LICENSE file comes with this software, it is provided AS-IS.
**
****************************************************************************
*/

/* Entry Point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack */
_estack = ORIGIN(RAM_D1) + LENGTH(RAM_D1);    /* end of RAM */
/* Generate a link error if heap and stack don't fit into RAM */
_Min_Heap_Size = 0x200;      /* required amount of heap  */
_Min_Stack_Size = 0x400; /* required amount of stack */

/* Specify the memory areas */
MEMORY
{
  FLASH (rx)     : ORIGIN = 0x08000000, LENGTH = 2048K
  ITCMRAM (xrw)  : ORIGIN = 0x00000000, LENGTH = 64K
  DTCMRAM (xrw)  : ORIGIN = 0x20000000, LENGTH = 128K
  RAM_D1 (xrw)   : ORIGIN = 0x24000000, LENGTH = 512K
  RAM_D2 (xrw)   : ORIGIN = 0x30000000, LENGTH = 288K
  RAM_D3 (xrw)   : ORIGIN = 0x38000000, LENGTH = 64K
  BKPRAM (xrw)   : ORIGIN = 0x38800000, LENGTH = 4K

  EX_FLASH(rx)   : ORIGIN = 0x90000000, LENGTH = 8M
  SDRAM (xrw)    : ORIGIN = 0xC0000000, LENGTH = 16M
}

/* Define output sections */
SECTIONS
{
  /* The startup code goes first into FLASH */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector)) /* Startup code */
    . = ALIGN(4);
  } >FLASH

  /* The program code and other data goes into FLASH */
  .text :
  {
    . = ALIGN(4);
    *(.text)           /* .text sections (code) */
    *(.text*)          /* .text* sections (code) */
    *(.glue_7)         /* glue arm to thumb code */
    *(.glue_7t)        /* glue thumb to arm code */
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    . = ALIGN(4);
    _etext = .;        /* define a global symbols at end of code */
  } >FLASH

  /* Constant data goes into FLASH */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)         /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
    . = ALIGN(4);
  } >FLASH

  .ARM.extab (READONLY) : /* The READONLY keyword is only supported in GCC11 and later, remove it if using GCC10 or earlier. */
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } >FLASH
  .ARM (READONLY) : /* The READONLY keyword is only supported in GCC11 and later, remove it if using GCC10 or earlier. */
  {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >FLASH

  .preinit_array (READONLY) : /* The READONLY keyword is only supported in GCC11 and later, remove it if using GCC10 or earlier. */
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >FLASH

  .init_array (READONLY) : /* The READONLY keyword is only supported in GCC11 and later, remove it if using GCC10 or earlier. */
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >FLASH

  .fini_array (READONLY) : /* The READONLY keyword is only supported in GCC11 and later, remove it if using GCC10 or earlier. */
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >FLASH

  /* ---- ITCM: 代码段，FLASH 存放，启动时由 mem_itcm_load() 搬运 ---- */
  __itcm_load__ = LOADADDR(.itcm_ram_func);

  .itcm_ram_func :
  {
    . = ALIGN(4);
    __itcm_start__ = .;
    *(.itcm_ram_func)
    *(.itcm_ram_func*)
    . = ALIGN(4);
    __itcm_end__ = .;
  } >ITCMRAM AT> FLASH

  /* ---- SDRAM_CODE: 外部 SDRAM 执行代码，EX_FLASH 存放，启动时由 mem_sdram_load() 搬运 ---- */
  __sdram_code_load__ = LOADADDR(.sdram_code);

  .sdram_code :
  {
    . = ALIGN(4);
    __sdram_code_start__ = .;
    *(.sdram_code)
    *(.sdram_code*)
    . = ALIGN(4);
    __sdram_code_end__ = .;
  } >SDRAM AT> EX_FLASH

  /* used by the startup to initialize data */
  _sidata = LOADADDR(.data);

  /* Initialized data sections goes into RAM, load LMA copy after code */
  .data :
  {
    . = ALIGN(4);
    _sdata = .;        /* create a global symbol at data start */
    *(.data)           /* .data sections */
    *(.data*)          /* .data* sections */
    *(.RamFunc)        /* .RamFunc sections */
    *(.RamFunc*)       /* .RamFunc* sections */

    . = ALIGN(4);
    _edata = .;        /* define a global symbol at data end */
  } >RAM_D1 AT> FLASH

  /* Uninitialized data section */
  . = ALIGN(4);
  .bss :
  {
    /* This is used by the startup in order to initialize the .bss section */
    _sbss = .;         /* define a global symbol at bss start */
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)

    . = ALIGN(4);
    _ebss = .;         /* define a global symbol at bss end */
    __bss_end__ = _ebss;
  } >RAM_D1

  /* User_heap_stack section, used to check that there is enough RAM left */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM_D1

  /* ---- SRAM_D1: 剩余空闲内存 ---- */
  .sram_d1 (NOLOAD) :
  {
    . = ALIGN(4);
    __sram_d1_free_start__ = .;
    *(.sram_d1)
    . = ALIGN(4);
  } >RAM_D1

  /* ---- SRAM_D2: SRAM1+SRAM2 DMA 缓冲区 (256KB) ---- */
  .sram_d2_dma (NOLOAD) :
  {
    . = ALIGN(4);
    __sram_d2_dma_start__ = .;
    *(.sram_d2_dma)
    . = ALIGN(4);
    __sram_d2_dma_free_start__ = .;
  } >RAM_D2

  /* ---- SRAM_D2: SRAM3 USB/ETH 缓冲区 (32KB) ---- */
  .sram_d2_usb (NOLOAD) :
  {
    . = ALIGN(4);
    __sram_d2_usb_start__ = .;
    *(.sram_d2_usb)
    . = ALIGN(4);
    __sram_d2_usb_end__ = .;
  } >RAM_D2

  /* ---- SRAM_DTCM: 数据 TCM (128KB) ---- */
  .sram_dtcm (NOLOAD) :
  {
    . = ALIGN(4);
    __sram_dtcm_start__ = .;
    *(.sram_dtcm)
    . = ALIGN(4);
    __sram_dtcm_end__ = .;
  } >DTCMRAM

  /* ---- SRAM_D3: 低功耗保留，BDMA 可访问 (64KB) ---- */
  .sram_d3 (NOLOAD) :
  {
    . = ALIGN(4);
    __sram_d3_start__ = .;
    *(.sram_d3)
    . = ALIGN(4);
    __sram_d3_end__ = .;
  } >RAM_D3

  /* ---- Backup SRAM: VBAT 供电保持 (4KB) ---- */
  .sram_bkp (NOLOAD) :
  {
    . = ALIGN(4);
    __sram_bkp_start__ = .;
    *(.sram_bkp)
    . = ALIGN(4);
    __sram_bkp_end__ = .;
  } >BKPRAM

  /* ---- SDRAM_DATA: 外部 SDRAM 数据段 (零初始化，类似 BSS) ---- */
  .sdram_data (NOLOAD) :
  {
    . = ALIGN(4);
    __sdram_data_start__ = .;
    *(.sdram_data)
    *(.sdram_data*)
    . = ALIGN(4);
    __sdram_data_end__ = .;
  } >SDRAM

  /* ---- SDRAM_FREE: SDRAM 中 .sdram_code + .sdram_data 之后剩余空间，供 TLSF 管理 ---- */
  .sdram_free (NOLOAD) :
  {
    . = ALIGN(4);
    __sdram_free_start__ = .;
    *(.sdram_free)
    . = ALIGN(4);
  } >SDRAM

  /* Remove information from the standard libraries */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
