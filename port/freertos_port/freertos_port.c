#include "freertos_port.h"
#include <stdlib.h>

#if BSP_FREERTOS_ENABLED

//--------------------------------------------------------------------+
// 动态内存分配 (对接标准库 malloc/free, 经 --wrap 链路走 TLSF)
//--------------------------------------------------------------------+

void * pvPortMalloc(size_t xWantedSize)
{
    void * pvReturn;

    vTaskSuspendAll();
    pvReturn = malloc(xWantedSize);
    (void) xTaskResumeAll();

#if (configUSE_MALLOC_FAILED_HOOK == 1)
    if (pvReturn == NULL) vApplicationMallocFailedHook();
#endif

    return pvReturn;
}

void vPortFree(void * pv)
{
    if (pv != NULL) {
        vTaskSuspendAll();
        free(pv);
        (void) xTaskResumeAll();
    }
}

void vPortHeapResetState(void)
{
    /* heap_3 无状态需要重置 */
}

//--------------------------------------------------------------------+
// 回调
//--------------------------------------------------------------------+

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
