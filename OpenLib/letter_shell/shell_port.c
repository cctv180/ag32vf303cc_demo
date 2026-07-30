/**
 * @file shell_port.c
 * @brief letter-shell 移植层（UART0 / MSG_UART）
 */

#include "shell.h"
#include "uart.h"
#include "system.h"

Shell shell;
char shellBuffer[512];

/**
 * @brief 系统信息
 */
int sys_info(int argc, char *agrv[])
{
    (void)argc;
    (void)agrv;

    printf("DeviceID : 0x%08lx\r\n", (unsigned long)SYS_GetDeviceID());
    printf("SysClk   : %.3f MHz\r\n", SYS_GetSysClkFreq() / (double)1e6);
    printf("Pclk     : %.3f MHz\r\n", SYS_GetPclkFreq() / (double)1e6);
    return 0;
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN,
                 sys_info, sys_info, sys info);

/**
 * @brief 测试命令：打印全部参数
 */
int testfunc(int argc, char *agrv[])
{
    printf("%d parameter(s)\r\n", argc);
    for (int i = 1; i < argc; i++)
    {
        printf("%s\r\n", agrv[i]);
    }
    return 0;
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN),
                 testfunc, testfunc, testfunc);

/**
 * @brief 用户 shell 写
 */
short userShellWrite(char *data, unsigned short len)
{
    UART_Send(MSG_UART, (const unsigned char *)data, len);
    return (short)len;
}

/**
 * @brief 用户 shell 读（非阻塞：FIFO 无数据立即返回）
 */
short userShellRead(char *data, unsigned short len)
{
    unsigned short n = 0;

    while (n < len)
    {
        if (UART_IsRxFifoEmpty(MSG_UART))
        {
            break;
        }
        data[n++] = (char)UART_ReceiveData(MSG_UART);
    }
    return (short)n;
}

/**
 * @brief 用户 shell 初始化
 */
void userShellInit(void)
{
    shell.write = userShellWrite;
    shell.read = userShellRead;
    shellInit(&shell, shellBuffer, 512);
}
