/**
 * AG32 DEMO — LED blink + UART printf
 *
 * Board init (clock / UART / LED GPIO) comes from agrv_sdk board support.
 * Pin map: demo_board.ve
 */

#include "board.h"

#define BLINK_GPIO      GPIO4
#define BLINK_GPIO_BITS ((1 << 1) | (1 << 2)) /* LED1 + LED2 */
#define BLINK_MS        500

int main(void)
{
    board_init();

    printf("\n========== AG32 DEMO ==========\n");
    printf("Chip: AgRV2K / board agrv2k_303\n");
    printf("SysClk: %.3f MHz\n", SYS_GetSysClkFreq() / (double)1e6);
    printf("Blink GPIO4[1:2], period %d ms\n", BLINK_MS);
    printf("================================\n\n");

    /* board_init already enables EXT_GPIO (GPIO4 bits 1..3) as output */
    GPIO_SetOutput(BLINK_GPIO, BLINK_GPIO_BITS);

    uint32_t tick = 0;
    while (1)
    {
        GPIO_Toggle(BLINK_GPIO, BLINK_GPIO_BITS);
        printf("[%lu] LED toggle\n", (unsigned long)tick++);
        UTIL_IdleMs(BLINK_MS);
    }
}
