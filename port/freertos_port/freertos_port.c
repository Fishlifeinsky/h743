#include "freertos_port.h"

#if BSP_FREERTOS_ENABLED

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    // 栈溢出，挂起
    taskDISABLE_INTERRUPTS();
    while (1) {}
}

void freertos_port_init(void)
{
    vTaskStartScheduler();

    // 永远不会到这里
    while (1) {}
}

#endif /* BSP_FREERTOS_ENABLED */
