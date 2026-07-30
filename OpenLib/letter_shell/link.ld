/* 覆盖默认链接脚本：AgRV2K_FLASH + 含 letter-shell 命令段的 section.ld
 * platformio.ini: board_build.ldscript = OpenLib/letter_shell/link.ld-
 *
 * INCLUDE 按 -L / 当前目录搜索，不用“相对本脚本目录”的短名。
 */
INCLUDE AgRV2K_FLASH.ld
INCLUDE OpenLib/letter_shell/section.ld
