# ag32vf303_demo

AG32（AgRV2K / `agrv2k_303`）示例工程：LED 闪烁 + UART `printf`。

基于 AGM PlatformIO 开发环境（`platform = AgRV`），应用源码在 `src/`，逻辑与管脚配置见 `demo_board.ve` / `demo_board.asf`。

## 环境要求

- 64 位 Windows 8.1 / 10 / 11（不支持 Win7）
- 安装路径避免中文
- VS Code + Python（安装时勾选加入 PATH）
- VS Code 扩展：**PlatformIO IDE**

官方说明可参考：[AG32 IDE 开发环境搭建](https://www.agmcn.com/doc/6720.html)、[AG32 使用入门](https://www.agmcn.com/doc/6793.html)。

## 安装 AgRV SDK（PlatformIO 平台包）

1. 从 AGM 提供的网盘/官网下载最新 **AgRV_pio** 安装包（如 `AgRV_pio-*-win64-*.exe`）。
2. 安装到无中文路径（例如 `D:\AgRV_pio`），其中会包含：
   - `platforms\AgRV` — 平台与板级支持
   - `packages\framework-agrv_sdk` 等 — 驱动与可选组件（FreeRTOS、lwIP、TinyUSB…）
3. 若 PlatformIO 核心目录落在含中文路径下，可设置环境变量 `PLATFORMIO_CORE_DIR` 指向无中文目录。
4. 用 VS Code 打开本工程目录，等待 PlatformIO 识别 `platform = AgRV`。

首次打开可先编译官方 example 验证环境：

`AgRV_pio\platforms\AgRV\examples\example`

## 本工程内嵌 SDK

为方便查看驱动源码与随工程版本管理，已将基础 SDK 拷入工程：

```text
frameworks/
└─ framework-agrv_sdk/    # 目录名不可更改
```

`platformio.ini` 中通过下列选项优先使用工程内 SDK：

```ini
framework = agrv_sdk
inline_framework_dir = ./frameworks
```

说明：

- 外层目录名可自定义（本工程用 `frameworks`），子目录必须叫 `framework-agrv_sdk`。
- 若后续启用 `agrv_freertos`、`agrv_lwip` 等，可将对应 `framework-agrv_*` 同样放入 `frameworks/`。
- 未放入工程的 framework 仍从本机 `packages` 安装目录加载。
- 合并方式参见：[将 SDK 合并到工程的办法](https://www.agmcn.com/doc/5768.html)。

## 本工程内置 board

同样按官方文档，将板级包拷入工程，避免平台升级导致 board 定义变化：

```text
boards/
├─ agrv2k_303.json
└─ agrv2k_303/
   ├─ board.c / board.h
   └─ board.ve / board.asf   # 板级默认 Logic；本工程实际用 demo_board.ve / .asf
```

`platformio.ini` 中启用：

```ini
boards_dir = boards
board = agrv2k_303
```

## 编译与烧录

AG32 要烧 **两份固件**：CPLD（Logic）和 MCU（Code）。

环境切换在 IDE **最下方状态栏**点 `env:debug` / `env:release`（本工程默认 `debug`）

在PlatformIO 左侧 **PROJECT TASKS** 操作即可（与终端命令对应）：

| 侧栏 | 本工程实际命令 | 作用 |
|---|---|---|
| **Build** | `pio run` | 编译 MCU |
| **Upload**       | `pio run -t upload`       | **只烧** MCU：`.pio\build\debug\agrv_demo.bin` → `0x80000000` |
| **Upload LOGIC** | `pio run -t logic` | **只烧** Logic：`.pio\logic\demo_board.bin` → `0x80027000` |
| **Create Batch** | `pio run -t batch`        | 生成工程根目录 `agrv_demo_debug_batch.bin`，**不烧芯片**     |
| **Upload Batch** | `pio run -t batch_upload` | 一次烧该 batch（含 Logic + MCU）                             |
| **Monitor**      | `pio device monitor`      | 串口监视（`UART0` / `COM*` / 115200）                        |

**推荐顺序：先 Upload LOGIC，再 Upload。**  
新芯片若未烧 Logic 就只点 Upload，运行会报错。只改了 `src/` 可再点 Upload；只改了 `.ve` / `.asf` 再点 Upload LOGIC。详见 [AG32的程序烧录](https://www.agmcn.com/doc/5766.html)。

PlatformIO 侧栏 其它 Custom 任务：**Unlock Flash** / **Wipe Flash**（慎用）、**Clean Garbage**、**Clean LOGIC**。

## `platformio.ini` 配置说明

约定：行首 `;` 表示注释掉的可选项（当前未启用）；无 `;` 为生效项。`#` 仅为说明注释。

### `[platformio]` — 工程全局

| 项 | 当前 | 含义 |
|---|---|---|
| `; boards_dir` | 注释 | 自定义 board 目录；启用后可把板级文件放进工程 |
| `src_dir` | `.` | 查找 `.c` 的基路径（相对工程根） |
| `include_dir` | `./src` | 查找 `.h` 的默认包含路径 |
| `default_envs` | `debug` | 默认环境；底部状态栏可切 `release` |

### `[setup]` — 板型、SDK、编译

| 项 | 当前 | 含义 |
|---|---|---|
| `board` | `agrv2k_303` | 板型/Flash：256K 用 103/303，1M 用 407（与脚数无关） |
| `framework` | `agrv_sdk` | 使用的库；可逗号多选，如再加 `agrv_freertos` |
| `inline_framework_dir` | `./frameworks` | 优先用工程内 `framework-agrv_*` |
| `program` | `agrv_demo` | 产物名：`agrv_demo.elf` / `.bin` / `*_batch.bin` |
| `; toolchain` | 注释 | `gnu`（默认）或 `clang` |
| `build_src_filter` | `-<*> +<./src/**/*.c>` | 排除全部后，只编译 `src` 下 `.c` |
| `; build_flags` | 注释 | 额外编译选项，如 `-Ixxx -DFOO` |
| `; debug_build_flags` | 注释 | debug 优化/调试信息（有平台默认值） |
| `; release_build_flags` | 注释 | release 优化（有平台默认值） |
| `; board_build.march` | 注释 | RISC-V ISA；默认 `rv32imafc`，改 `rv32imac` 可关 FPU |
| `; board_build.mabi` | 注释 | ABI；默认 `ilp32f`，改 `ilp32` 关浮点 ABI |
| `; check_device` | 注释 | 烧录前校验芯片 ID/Flash；`0` 关闭 |
| `; check_logic` | 注释 | 烧录前校验 Logic；`0` 关闭（新片未烧 Logic 常因此报错） |
| `; board_build.boot_addr` | 注释 | MCU 入口，默认 `0x80000000` |
| `; board_build.boot_mode` | 注释 | `flash` / `flash_sram` / `sram` |
| `; board_build.ldscript` | 注释 | 附加或覆盖链接脚本（`user.ld-` 为覆盖） |

### `[setup_logic]` — Logic / 管脚 / CPLD

| 项 | 当前 | 含义 |
|---|---|---|
| `logic_ve` | `demo_board.ve` | 时钟 + 信号线↔管脚 |
| `logic_device` | `AGRV2KL48` | 封装脚数（L100/L64/L48/Q32 等） |
| `; logic_compress` | 注释 | Logic 压缩（约 &lt;48KB，启动多 30–50 ms） |
| `; logic_embed` | 注释 | 把 Logic 嵌进 MCU 程序 |
| `; board_upload.logic_address` | 注释 | Logic 地址；未压缩默认约 `0x80027000` |
| `; ip_name` 等 | 注释 | 自定义 CPLD IP（`logic_dir` / `logic_ip` / `ips_dir`） |
| `design.asf` | `./demo_board.asf` | 上拉/下拉、驱动能力等 |
| `; design.pre_asf` / `; design.post_asf` | 注释 | 额外 asf，一般不用 |

### `[setup_upload]` — 调试 / 下载（当前启用）

| 项 | 当前 | 含义 |
|---|---|---|
| `protocol` | `cmsis-dap-openocd` | 调试器协议（也可 jlink / stlink / serial 等） |
| `debug_tool` | `${this.protocol}` | 调试工具 = `protocol` |
| `upload_protocol` | `${this.protocol}` | 烧录协议 = `protocol` |
| `; debug_speed` / `; upload_speed` | 注释 | SWD 速度（KHz） |
| `; upload_srst` | 注释 | 烧录前硬件复位 |

### `[setup_upload_serial]` — 串口烧录（未启用）

当前 `[env]` 继承的是 `setup_upload`。若要串口烧录，将 `extends` 中该项改为 `setup_upload_serial`。

| 项 | 含义 |
|---|---|
| `upload_protocol = serial` | 串口下载（无调试） |
| `upload_port = COM*` | 任意 COM |
| `upload_speed = 115200` | 波特率 |

### `[setup_monitor]` — 串口 printf（当前启用）

| 项 | 当前 | 含义 |
|---|---|---|
| `logger_if` | `UART0` | 日志 UART；TX 脚由 `.ve` 决定 |
| `monitor_port` | `COM*` | Monitor 串口 |
| `monitor_speed` | `115200` | 波特率 |

### `[setup_monitor_rtt]` — RTT 日志（未启用）

将 `extends` 中的 `setup_monitor` 换成 `setup_monitor_rtt` 即可用 Segger RTT。

### `[setup_batch]` — Create Batch / Upload Batch

| 项 | 当前 | 含义 |
|---|---|---|
| `; batch_user_bin` | 注释 | batch 中额外写入用户 bin 或常量到指定地址 |
| `; batch_user_logic` | 注释 | 用外部 logic.bin 替换本工程 Logic |
| `; batch_output` | 注释 | 改输出名；默认 `{program}_{env}_batch.bin` |
| `; lock_flash` | 注释 | batch 带读保护 / 锁 Flash |
| `; batch_arg` | 注释 | 生成 batch 的额外参数 |

### `[env]` / `[env:debug]` / `[env:release]`

| 项 | 含义 |
|---|---|
| `platform = AgRV` | AgRV 平台，不要改 |
| `extends = ...` | 组合上面各段；可改继承以切换串口烧录或 RTT |
| `[env:debug]` `build_type = debug` | 调试构建 |
| `[env:release]` `build_type = release` | 发布构建 |

## `demo_board.ve` 说明

AG32 的 **信号线**（如 `UART0_UARTTXD`、`GPIO4_1`）与芯片 **物理管脚**（`PIN_xx`）是分离的，需在 `.ve` 里一行一行绑定；系统初始化时会按该文件配置时钟与管脚。详见 [MCU驱动使用](https://www.agmcn.com/doc/4193.html)。

本工程 `demo_board.ve` 当前内容：

| 配置 | 含义 |
|---|---|
| `SYSCLK 200` / `BUSCLK 100` | 系统主频 200 MHz，总线 100 MHz（PLL 由系统自动配置） |
| `HSECLK 8` | 外部晶振 8 MHz（范围一般 4～16 MHz） |
| `UART0_UARTTXD PIN_30` / `UART0_UARTRXD PIN_31` | UART0 作 `printf` / 串口监视 |
| `GPIO4_1 PIN_28` / `GPIO4_2 PIN_29` | LED1 / LED2 |
| `GPIO6_2 PIN_35` | 按键 |

修改管脚时按原理图改右侧 `PIN_xx`；信号名以官方 Function Pin 列表为准。电源、复位、晶振、部分 ADC/USB 等固定脚不可随意重映射。`logic_device`（本工程为 `AGRV2KL48`）须与实际封装一致。

## `demo_board.asf` 说明

`.asf` 用于设定 **CPLD 侧**的管脚电气/附加配置（例如 GPIO 上拉/下拉、输出驱动能力等），与 `.ve` 里的「信号线 ↔ 管脚」映射配合使用。本工程由 `platformio.ini` 中 `design.asf = ./demo_board.asf` 指定。一般工程保留一个 `.asf` 即可；`.pre` / `.post` 多数情况用不到。新建工程可从官方 example 复制再改，参见 [建立自己的第一个工程](https://www.agmcn.com/doc/8141.html)。

## 目录结构

```text
.
├─ src/main.c                 # 应用
├─ demo_board.ve              # 时钟 + 信号线↔管脚
├─ demo_board.asf             # CPLD：上拉/下拉、驱动能力等
├─ frameworks/framework-agrv_sdk/
├─ platformio.ini
└─ README.md
```
