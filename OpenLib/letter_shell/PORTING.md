# letter-shell 移植说明（AG32 / AgRV2K + PlatformIO + FreeRTOS）

本目录记录 letter-shell 在本工程的移植方式，供后续维护与移植到其他 AgRV 工程参考。

库版本：**letter-shell 3.2.4**（`OpenLib/letter_shell/Lib`，git submodule，`branch = master`）。

---

## 1. 目录结构

```
OpenLib/letter_shell/
├── Lib/                 # git submodule：https://github.com/NevermindZZT/letter-shell
├── shell_cfg_user.h     # 用户配置（overlay 覆盖 Lib/src/shell_cfg.h 默认值）
├── shell_port.c         # 移植层：read / write / shellInit + 工程自定义命令
├── shell_port.h         # 对外接口：extern shell、userShellInit()
├── section.ld           # 替换 SDK 的 section.ld，在 .rodata 内收集 shellCommand 段
├── link.ld              # 顶层链接脚本，串起 AgRV2K_FLASH.ld + 本目录 section.ld
└── PORTING.md           # 本文件
```

相关但不在本目录：

```
OpenLib/FreeRTOS/
├── FreeRTOSConfig.h     # FreeRTOS 配置（含 TRACE，供 ps 等命令）
└── freertos_cmd.c       # shell 导出的 FreeRTOS 调试命令
```

库以 submodule 形式引入，克隆工程后需要执行：

```powershell
git submodule update --init --recursive
```

---

## 2. 官方移植步骤 与 本工程对应实现

官方 README「移植说明」的五步 + 本工程配置落点如下：

| 步骤 | 本工程实现位置 |
| --- | --- |
| 定义 shell 对象 | `shell_port.c` 中 `Shell shell;` |
| 定义读写函数 | `shell_port.c` 中 `userShellRead()` / `userShellWrite()` |
| 申请缓冲区 | `shell_port.c` 中 `char shellBuffer[512];` |
| 调用 `shellInit` | `shell_port.c` 中 `userShellInit()` |
| 建立 shell 任务 | `src/main.c` 中 `prvShellTask()` |
| 配置宏 overlay | `shell_cfg_user.h` + 编译定义 `SHELL_CFG_USER` |

---

## 3. 串口读写实现

AgRV SDK 的 UART 是**纯轮询**驱动，`uart.c` 内没有中断服务函数，因此直接用寄存器级接口对接。
目标串口取 SDK 的全局日志串口 `MSG_UART`（由 `platformio.ini` 的 `logger_if = UART0` 决定，
在 `board_init()` 里赋值）。去掉 `logger_if` 或不调用 `board_init()` 时 `MSG_UART` 可能为空指针。

写：

```c
short userShellWrite(char *data, unsigned short len)
{
    UART_Send(MSG_UART, (const unsigned char *)data, len);
    return (short)len;
}
```

读**必须非阻塞**，否则会顶死 shell 任务。SDK 的 `UART_Receive()` 带超时会自旋等待，
因此这里直接判空 RX FIFO，取完即返回：

```c
short userShellRead(char *data, unsigned short len)
{
    unsigned short n = 0;
    while (n < len) {
        if (UART_IsRxFifoEmpty(MSG_UART)) break;
        data[n++] = (char)UART_ReceiveData(MSG_UART);
    }
    return (short)n;
}
```

> 若后续改为中断/DMA + 环形缓冲，只需替换这两个函数体，其余部分不用动。

波特率以固件侧 `BAUD_RATE` / `LOGGER_BAUD_RATE` 为准（可由平台根据 monitor 配置注入）；
串口监视器使用 `platformio.ini` 的 `monitor_speed`，须与固件一致。

---

## 4. 任务模型与输入吞吐

配置取 `SHELL_TASK_WHILE = 0`。此时 `shellTask()` **每次调用只 `read` 1 个字节**（不是处理一整行），
由外层 FreeRTOS 任务循环 + `vTaskDelay` 做轮询：

```c
static void prvShellTask(void *pvParameters)
{
    (void)pvParameters;
    userShellInit();
    for (;;) {
        shellTask(&shell);                 /* 最多处理 1 字节 */
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
```

任务在 `main()` 中创建，栈 512 字（`SHELL_TASK_STACK`），优先级 `tskIDLE_PRIORITY + 2`。

`userShellInit()` 放在任务内部调用，保证 `shellInit()` 在调度器启动后执行。

**吞吐限制：** 当前约 1 字节 / 10 ms ≈ **100 B/s**。UART0 为 16 字节 FIFO，粘贴长命令或方向键
（多字节 ESC 序列）时可能丢字符。改进方向：

1. 任务内把 RX FIFO 抽干后再 `vTaskDelay`（改动小）；或
2. 改为 `SHELL_TASK_WHILE = 1` + 阻塞读（UART 中断 / 队列），并  
   `xTaskCreate(shellTask, "shell", 512, &shell, prio, NULL)` ——  
   读函数必须能阻塞让出 CPU，否则空转吃满一个优先级。

---

## 5. 关键配置（shell_cfg_user.h）

通过编译宏 `SHELL_CFG_USER="shell_cfg_user.h"` 注入，
`Lib/src/shell_cfg.h` 会 `#include SHELL_CFG_USER` 后再对未定义项补默认值。

注意：

- 头文件保护宏**不能**用 `__SHELL_CFG_H__`，否则会把库默认头挡住。
- overlay **不必**抄全 `shell_cfg.h`；本工程为便于对照，把可配置宏都显式写了。
- 当前库版本**没有** `SHELL_COMMAND_MAX_LENGTH`（README 文档残留）；命令行长度见下文缓冲区说明。

| 宏 | 取值 | 说明 |
| --- | --- | --- |
| `SHELL_TASK_WHILE` | `0` | 外层任务轮询，见第 4 节 |
| `SHELL_USING_CMD_EXPORT` | `1` | 命令导出方式，需链接脚本配合，见第 6 节 |
| `SHELL_USING_COMPANION` | `0` | 不用伴生对象，故 `SHELL_MALLOC/FREE` 置 0 |
| `SHELL_SUPPORT_END_LINE` | `1` | 尾行模式（覆盖库默认 0） |
| `SHELL_GET_TICK()` | `UTIL_GetTick()` | SDK 毫秒计时，供双击 Tab / 超时锁定使用 |
| `SHELL_ENTER_LF` / `SHELL_ENTER_CR` | `1` / `1` | 兼容不同终端换行；`SHELL_ENTER_CRLF` 必须为 0 |
| `SHELL_PRINT_BUFFER` / `SHELL_SCAN_BUFFER` | `0` / `0` | 关闭 shell 格式化 I/O，省 RAM；**不能**用 `shellPrint` |
| `SHELL_USING_LOCK` | `0` | 未实现 lock/unlock；多任务同写 shell 时需改为 1 并接互斥量 |
| `SHELL_DEFAULT_USER` | `"admin"` | 密码为 `""` 表示不校验 |
| `SHELL_CLS_WHEN_LOGIN` | `0` | 登录不清屏（覆盖库默认 1） |
| `SHELL_LOCK_TIMEOUT` | `0 * 60 * 1000` | 表达式求值为 0，关闭自动锁定 |
| `SHELL_HELP_SHOW_PERMISSION` | `0` | help 不显示权限（覆盖库默认 1） |

### 缓冲区实际容量

`shellInit(&shell, shellBuffer, 512)` 会把缓冲区在解析区与历史记录之间等分：

```text
parser.bufferSize = 512 / (SHELL_HISTORY_MAX_NUMBER + 1) = 512 / 6 ≈ 85 字节
```

单条命令有效长度约 **85 字节**，不是 512。需要更长命令时增大 `shellBuffer` 或减小 `SHELL_HISTORY_MAX_NUMBER`。

### 输出路径

命令实现里的 `printf` 走 newlib → SDK `putchar` → UART；shell 提示符/回显走 `shell->write`。
两条路径独立且 `SHELL_USING_LOCK=0`，与其它任务的 `printf` 可能交错。规范化做法是打开
`SHELL_PRINT_BUFFER`（如 128）并在命令中用 `shellPrint(shellGetCurrent(), ...)`。

---

## 6. 链接脚本（GCC 命令导出的必要条件）

`SHELL_USING_CMD_EXPORT = 1` 时，`SHELL_EXPORT_CMD` 等宏把命令结构体放进名为
`shellCommand` 的自定义段，`shell.c` 在 GCC 下靠这两个符号定位命令表：

```c
extern const unsigned int _shell_command_start;
extern const unsigned int _shell_command_end;
```

官方 README 要求在 ld 文件的只读数据区添加：

```ld
_shell_command_start = .;
KEEP (*(shellCommand))
_shell_command_end = .;
```

### 为什么不能用 `INSERT AFTER` 追加

AgRV 平台把链接脚本拆成多个 `-Wl,-T` 传给 ld
（见 `platforms/AgRV/builder/frameworks/agrv_sdk.py` 中 `ldscripts` 的拼装）。
这种多脚本组合下用独立片段 `INSERT AFTER .text;` 会报
`.text not found for insert`，因此改为**整份替换** SDK 的 `section.ld`。

### 实际做法

`platformio.ini` 中末尾的 `-` 表示覆盖（而非追加）SDK 默认链接脚本：

```ini
board_build.ldscript = OpenLib/letter_shell/link.ld-
```

`link.ld` 只做两件事。`INCLUDE` 走 ld 的搜索路径（`-L` 与当前工作目录），
所以第二行必须写工程相对路径，不能写成 `section.ld`：

```ld
INCLUDE AgRV2K_FLASH.ld
INCLUDE OpenLib/letter_shell/section.ld
```

`section.ld` 复制自 `frameworks/framework-agrv_sdk/misc/section.ld`，
仅在 `.rodata` 段末尾插入命令段收集：

```ld
  .rodata : ALIGN(4) {
    *(.rodata*);
    . = ALIGN(4);
    _shell_command_start = .;
    KEEP (*(shellCommand))
    _shell_command_end = .;
    . = ALIGN(4);
  } >REGION_RODATA AT >REGION_TEXT
```

放在 `.rodata` **内部**而不是新建独立段，是因为 SDK 启动代码
（`frameworks/framework-agrv_sdk/misc/syscalls.c`）只按
`__rodata_lma / __rodata_vma / __rodata_size` 等固定符号做拷贝，
自定义段不会被搬运。flash 启动模式下 `REGION_RODATA` 是 SRAM，
命令表必须随 `.rodata` 一起拷贝才能访问。

> 若升级 SDK 且 `misc/section.ld` 有改动，需要把改动同步到本目录的 `section.ld`。

---

## 7. platformio.ini 配置

```ini
build_src_filter =
  -<*>
  +<./src/**/*.c>
  +<./OpenLib/letter_shell/shell_port.c>
  +<./OpenLib/letter_shell/Lib/src/shell.c>
  +<./OpenLib/letter_shell/Lib/src/shell_cmd_list.c>
  +<./OpenLib/letter_shell/Lib/src/shell_companion.c>
  +<./OpenLib/letter_shell/Lib/src/shell_ext.c>
  +<./OpenLib/FreeRTOS/freertos_cmd.c>

build_flags =
  -I./OpenLib/FreeRTOS
  -I./OpenLib/letter_shell
  -I./OpenLib/letter_shell/Lib/src
  -DSHELL_CFG_USER=\"shell_cfg_user.h\"

board_build.ldscript = OpenLib/letter_shell/link.ld-
```

说明：

- 只编译 `Lib/src` 下 4 个源文件，`Lib/extensions`（log、fs、telnet、game 等）与 `Lib/demo` 不参与编译。
- `shell_cmd_list.c` 与 `shell_companion.c` 在当前配置下整体被条件编译掉，编译进来几乎不占空间，保留是为了切换配置时无需改 ini。
- `-I./OpenLib/FreeRTOS` 用于找到 `FreeRTOSConfig.h`（已从 `src/` 迁至此目录）。
- `freertos_cmd.c` 提供 `ps` 等命令；依赖 `FreeRTOSConfig.h` 中  
  `configUSE_TRACE_FACILITY=1`、`configUSE_STATS_FORMATTING_FUNCTIONS=1`。
- `-DSHELL_CFG_USER=\"shell_cfg_user.h\"` 中的转义引号是必须的。

---

## 8. 添加命令

在任意参与编译的 `.c` 文件中导出即可，无需登记到表里。

main 函数形式（自行解析字符串参数）：

```c
int testfunc(int argc, char *argv[]) { /* ... */ return 0; }
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN),
                 testfunc, testfunc, testfunc);
```

普通 C 函数形式（由 shell 完成整数/字符/字符串参数转换）：

```c
int add(int a, int b) { return a + b; }
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC),
                 add, add, add two int);
```

不希望命令执行后打印返回值时，附加 `SHELL_CMD_DISABLE_RETURN`。

### 当前工程命令一览

| 来源 | 命令 |
| --- | --- |
| `shell_port.c` | `sys_info`、`testfunc` |
| `freertos_cmd.c` | `ps`、`os_meminfo`、`os_task_create`、`os_task_kill` |
| 库内置 | `help`、`cmds`、`users`、`vars`、`keys`、`clear`、`setVar`、`sh` 等 |

---

## 9. 验证与排查

编译：

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e debug
```

确认命令段被正确收集（能看到 `_shell_command_start` / `_shell_command_end`
以及各 `shellCommandXxx` 符号，且地址落在 SRAM 区间 `0x2000xxxx` 内）：

```powershell
& "$env:USERPROFILE\.platformio\packages\toolchain-agrv\bin\riscv64-unknown-elf-nm.exe" `
  .pio\build\debug\agrv_demo.elf | Select-String "shell_command|shellCommand"
```

串口：监视器波特率与固件 `BAUD_RATE` 一致；上电后回车可见提示符，用户 `admin` 无密码。

常见问题：

| 现象 | 原因 |
| --- | --- |
| 输入无反应、提示符不出现 | `userShellRead` 变成阻塞实现，或串口号与 `logger_if` 不一致 / `MSG_UART` 未初始化 |
| 粘贴丢字、方向键乱码 | §4 吞吐瓶颈：每次只读 1 字节且 10 ms 延时 |
| `help` 只有少量命令 / 命令找不到 | 命令段未被链接脚本收集，检查 `board_build.ldscript` 是否带结尾 `-` |
| `ps` 链接失败或无输出 | 未编入 `freertos_cmd.c`，或未打开 `configUSE_TRACE_FACILITY` |
| 链接报 `.text not found for insert` | 用了 `INSERT AFTER` 方式，见第 6 节 |
| 链接报 `cannot open linker script file section.ld` | `link.ld` 里 `INCLUDE` 写了短名，需写工程相对路径 |
| 回车后换行异常或命令执行两次 | 终端换行设置与 `SHELL_ENTER_LF/CR/CRLF` 组合不匹配 |
| 输出与其它任务 printf 交错 | `SHELL_PRINT_BUFFER=0` 且无 shell 锁，见第 5 节 |

---

## 10. 简化替代方案（命令表方式）

若不希望维护自定义链接脚本，可改用官方的命令表方式：

1. `shell_cfg_user.h` 中置 `SHELL_USING_CMD_EXPORT 0`；
2. 命令统一登记到命令表：可改 `Lib/src/shell_cmd_list.c`（submodule，改动不便提交），  
   或在工程内自建一份命令表源文件并改编译列表；
3. 删除 `platformio.ini` 的 `board_build.ldscript` 一行，并删除本目录 `link.ld` / `section.ld`。

代价是失去分散 `SHELL_EXPORT_CMD` 的便利，故当前工程选择命令导出方式。
