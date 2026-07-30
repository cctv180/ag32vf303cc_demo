/**
 * @file freertos_cmd.c
 * @brief FreeRTOS 调试命令（letter-shell 导出）
 *
 * 命令：ps / os_meminfo / os_task_create / os_task_kill
 */

#include "FreeRTOS.h"
#include "task.h"
#include "shell.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

/**
 * @brief 打印封装（预留互斥锁扩展点）
 */
static void safe_printf(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

#if (configCHECK_FOR_STACK_OVERFLOW > 0)
/**
 * @brief FreeRTOS 栈溢出钩子
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    printf("Error: Stack Overflow, Task Name: %s\r\n", pcTaskName);
    for (;;)
    {
    }
}
#endif

/**
 * @brief 查看所有任务状态 (ps)
 */
static int task_list(int argc, char *argv[])
{
    char *p_buffer;

    (void)argc;
    (void)argv;

    p_buffer = pvPortMalloc(1024);
    if (p_buffer == NULL)
    {
        safe_printf("Error: Malloc failed for task list buffer.\r\n");
        return -1;
    }

    safe_printf("=======================================================\r\n");
    safe_printf("Name\t\tState\tPrio\tStack\tNum\r\n");
    safe_printf("-------------------------------------------------------\r\n");

    vTaskList(p_buffer);
    safe_printf("%s", p_buffer);

    safe_printf("=======================================================\r\n");
    safe_printf("State: X-Running, R-Ready, B-Blocked, S-Suspended, D-Deleted\r\n");

    vPortFree(p_buffer);
    return 0;
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN,
                 ps, task_list, list all tasks);

/**
 * @brief 查看堆内存信息 (os_meminfo)
 */
static int mem_info(int argc, char *argv[])
{
    size_t free_heap_size;
    size_t total_heap_size;
    size_t used_heap_size;

    (void)argc;
    (void)argv;

    free_heap_size = xPortGetFreeHeapSize();
    total_heap_size = configTOTAL_HEAP_SIZE;
    used_heap_size = total_heap_size - free_heap_size;

    safe_printf("====== Heap Memory Info ======\r\n");
    safe_printf("Total Heap: %u bytes\r\n", (unsigned)total_heap_size);
    safe_printf("Used Heap : %u bytes\r\n", (unsigned)used_heap_size);
    safe_printf("Free Heap : %u bytes\r\n", (unsigned)free_heap_size);
    safe_printf("Min Free  : %u bytes\r\n", (unsigned)xPortGetMinimumEverFreeHeapSize());
    safe_printf("==============================\r\n");
    return 0;
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN,
                 os_meminfo, mem_info, show heap memory info);

/**
 * @brief 示例任务入口
 */
static void test_task_entry(void *param)
{
    int count = (int)(intptr_t)param;

    for (;;)
    {
        safe_printf("Hello from test_task, count = %d\r\n", count++);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/**
 * @brief 创建测试任务 (os_task_create)
 * @note  Usage: os_task_create [priority] [stack size]
 */
static int create_task_cmd(int argc, char *argv[])
{
    UBaseType_t priority;
    configSTACK_DEPTH_TYPE stack_size;
    static int task_counter = 0;
    TaskHandle_t created_task = NULL;
    BaseType_t result;

    if (argc != 3)
    {
        safe_printf("Usage: os_task_create [priority] [stack size]\r\n");
        safe_printf("Example: os_task_create 2 512\r\n");
        return -1;
    }

    priority = (UBaseType_t)atoi(argv[1]);
    stack_size = (configSTACK_DEPTH_TYPE)atoi(argv[2]);

    if (priority >= configMAX_PRIORITIES)
    {
        safe_printf("Error: priority must be < %d\r\n", (int)configMAX_PRIORITIES);
        return -1;
    }

    if (stack_size < configMINIMAL_STACK_SIZE)
    {
        safe_printf("Error: stack size must be >= %d\r\n", (int)configMINIMAL_STACK_SIZE);
        return -1;
    }

    result = xTaskCreate(test_task_entry,
                         "testTask",
                         stack_size,
                         (void *)(intptr_t)task_counter++,
                         priority,
                         &created_task);

    if (result == pdPASS)
    {
        safe_printf("Task 'testTask' created successfully!\r\n");
    }
    else
    {
        safe_printf("Failed to create task. Error code: %d\r\n", (int)result);
    }
    return 0;
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN,
                 os_task_create, create_task_cmd, create a test task);

/**
 * @brief 按任务编号删除任务 (os_task_kill)
 * @note  Usage: os_task_kill <Task_Num>  （编号见 ps 输出最后一列）
 */
static int task_kill_cmd(int argc, char *argv[])
{
    UBaseType_t task_id_to_kill;
    TaskHandle_t task_handle_to_kill = NULL;
    UBaseType_t num_of_tasks;
    TaskStatus_t *task_status_array;
    TaskHandle_t current_task_handle;
    const char *idle_task_name = "IDLE";
    UBaseType_t i;

    if (argc != 2)
    {
        safe_printf("Usage: os_task_kill <Task_Num>\r\n");
        safe_printf("Use 'ps' to find the Task_Num.\r\n");
        return -1;
    }

    task_id_to_kill = (UBaseType_t)strtoul(argv[1], NULL, 10);
    num_of_tasks = uxTaskGetNumberOfTasks();
    task_status_array = pvPortMalloc(num_of_tasks * sizeof(TaskStatus_t));
    if (task_status_array == NULL)
    {
        safe_printf("Error: Failed to allocate memory for task status array.\r\n");
        return -1;
    }

    num_of_tasks = uxTaskGetSystemState(task_status_array, num_of_tasks, NULL);
    current_task_handle = xTaskGetCurrentTaskHandle();

    for (i = 0; i < num_of_tasks; i++)
    {
        if (task_status_array[i].xTaskNumber == task_id_to_kill)
        {
            task_handle_to_kill = task_status_array[i].xHandle;

            if (strcmp(task_status_array[i].pcTaskName, idle_task_name) == 0)
            {
                safe_printf("Error: Cannot delete the IDLE task.\r\n");
                goto exit;
            }

            if (task_handle_to_kill == current_task_handle)
            {
                safe_printf("Error: Cannot delete the shell task itself.\r\n");
                goto exit;
            }

            break;
        }
    }

    if (task_handle_to_kill == NULL)
    {
        safe_printf("Error: Task with ID %lu not found.\r\n", (unsigned long)task_id_to_kill);
    }
    else
    {
        vTaskDelete(task_handle_to_kill);
        safe_printf("Task with ID %lu has been deleted.\r\n", (unsigned long)task_id_to_kill);
    }

exit:
    vPortFree(task_status_array);
    return 0;
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN,
                 os_task_kill, task_kill_cmd, kill a task by its ID);
