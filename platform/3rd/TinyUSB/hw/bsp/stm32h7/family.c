/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019
 *    William D. Jones (thor0505@comcast.net),
 *    Ha Thach (tinyusb.org)
 *    Uwe Bonnes (bon@elektron.ikp.physik.tu-darmstadt.de
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * This file is part of the TinyUSB stack.
 */

/* metadata:
   manufacturer: STMicroelectronics
*/

/* Minimal port for STM32H743 — CubeMX handles hardware init (clocks, GPIO,
 * USB peripheral). This file provides only the 3 functions TinyUSB requires:
 *   1. OTG_FS_IRQHandler  — USB interrupt → tusb_int_handler
 *   2. board_millis       — timebase for TinyUSB timeouts
 *   3. board_get_unique_id — UID → USB serial number string
 */

#include "stm32h7xx_hal.h"
#include "bsp/board_api.h"

//--------------------------------------------------------------------+
// USB interrupt handler
//--------------------------------------------------------------------+

// OTG_FS is RHPort0 in TinyUSB
void OTG_FS_IRQHandler(void) {
  tusb_int_handler(0, true);
}

//--------------------------------------------------------------------+
// Board porting API
//--------------------------------------------------------------------+

size_t board_get_unique_id(uint8_t id[], size_t max_len) {
  (void) max_len;
  volatile uint32_t * stm32_uuid = (volatile uint32_t *) UID_BASE;
  uint32_t* id32 = (uint32_t*) (uintptr_t) id;
  uint8_t const len = 12;

  id32[0] = stm32_uuid[0];
  id32[1] = stm32_uuid[1];
  id32[2] = stm32_uuid[2];

  return len;
}

#if CFG_TUSB_OS == OPT_OS_NONE

uint32_t board_millis(void) {
  return HAL_GetTick();
}

uint32_t tusb_time_millis_api(void) {
  return board_millis();
}

#endif

//--------------------------------------------------------------------+
// ISR overrides (CubeMX provides these — see CMakeLists.txt defines)
//--------------------------------------------------------------------+

#if !defined(SYSTICK_HANDLER_PROVIDED)
volatile uint32_t system_ticks = 0;

void SysTick_Handler(void) {
  HAL_IncTick();
  system_ticks++;
}
#endif

#if !defined(HARDFAULT_HANDLER_PROVIDED)
void HardFault_Handler(void) {
  __asm("BKPT #0\n");
}
#endif

#if !defined(INIT_PROVIDED)
void _init(void) {
}
#endif
