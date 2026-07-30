/**
 * AG32 DEMO — FreeRTOS LED 闪烁 + letter-shell
 *
 * 板级初始化（时钟 / UART / LED GPIO）由 agrv_sdk 板级支持完成。
 */

#include "FreeRTOS.h"
#include "task.h"

#include "board.h"
#include "shell_port.h"

#define BLINK_GPIO GPIO4
#define BLINK_GPIO_BITS ((1 << 1) | (1 << 2)) /* LED1 + LED2 */
#define BLINK_MS 500
#define BLINK_TASK_PRIO (tskIDLE_PRIORITY + 1)
#define SHELL_TASK_PRIO (tskIDLE_PRIORITY + 2)
#define SHELL_TASK_STACK 512

static void prvBlinkTask(void *pvParameters)
{
    (void)pvParameters;

    uint32_t tick = 0;
    for (;;)
    {
        GPIO_Toggle(BLINK_GPIO, BLINK_GPIO_BITS);
        printf("[%lu] LED toggle\n", (unsigned long)tick++);
        vTaskDelay(pdMS_TO_TICKS(BLINK_MS));
    }
}

/**
 * @brief letter-shell 轮询任务（SHELL_TASK_WHILE=0）
 */
static void prvShellTask(void *pvParameters)
{
    (void)pvParameters;

    userShellInit();

    for (;;)
    {
        shellTask(&shell);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

int main(void)
{
    board_init();
    /* 调度器启动后由 FreeRTOS 管理中断 */
    INT_DisableIntGlobal();

    printf("\n========== AG32 DEMO (FreeRTOS + shell) ==========\n");
    printf("Chip: AgRV2K / board agrv2k_303\n");
    printf("SysClk: %.3f MHz\n", SYS_GetSysClkFreq() / (double)1e6);
    printf("Blink GPIO4[1:2], period %d ms\n", BLINK_MS);
    printf("Shell: UART0, user admin (no password)\n");
    printf("==================================================\n\n");

    /* board_init 已将 EXT_GPIO（GPIO4 bit1..3）配置为输出 */
    GPIO_SetOutput(BLINK_GPIO, BLINK_GPIO_BITS);

    xTaskCreate(prvBlinkTask,
                "blink",
                configMINIMAL_STACK_SIZE,
                NULL,
                BLINK_TASK_PRIO,
                NULL);

    xTaskCreate(prvShellTask,
                "shell",
                SHELL_TASK_STACK,
                NULL,
                SHELL_TASK_PRIO,
                NULL);

    vTaskStartScheduler();

    /* 堆足够创建空闲/定时器任务时，不应执行到此处 */
    for (;;)
    {
    }
}

/* configSUPPORT_STATIC_ALLOCATION == 1 时需要提供 */
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize)
{
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize)
{
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
