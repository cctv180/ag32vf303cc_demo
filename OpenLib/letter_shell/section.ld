/* 基于 framework-agrv_sdk/misc/section.ld，在 .rodata 内嵌入 letter-shell 命令导出段 */
SECTIONS
{
  .flash : ALIGN(4) {
    *(.flash);
    *(.flash.*);
    . = ALIGN(4);
  } >FLASH

  .text : ALIGN(4) {
    *(.text*)
    . = ALIGN(4);
  } >REGION_EXEC AT >REGION_TEXT
  __exec_text_lma  = LOADADDR(.text);
  __exec_text_vma  = ADDR(.text);
  __exec_text_size = __exec_text_lma == __exec_text_vma ? 0 : SIZEOF(.text);

  .preinit_array : ALIGN(4) {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >REGION_EXEC AT >REGION_TEXT

  .init_array : ALIGN(4) {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >REGION_EXEC AT >REGION_TEXT

  .fini_array : ALIGN(4) {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >REGION_EXEC AT >REGION_TEXT

  .data : ALIGN(4) {
    *(.data*);
    *(.got*);
    . = ALIGN(4);
  } >REGION_DATA AT >REGION_TEXT
  __data_lma  = LOADADDR(.data);
  __data_vma  = ADDR(.data);
  __data_size = __data_lma == __data_vma  ? 0 : SIZEOF(.data);

  .rodata : ALIGN(4) {
    *(.rodata*);
    . = ALIGN(4);
    /* letter-shell SHELL_USING_CMD_EXPORT：随 .rodata 启动拷贝到 SRAM */
    _shell_command_start = .;
    KEEP (*(shellCommand))
    _shell_command_end = .;
    . = ALIGN(4);
  } >REGION_RODATA AT >REGION_TEXT
  __rodata_lma  = LOADADDR(.rodata);
  __rodata_vma  = ADDR(.rodata);
  __rodata_size = __rodata_lma == __rodata_vma  ? 0 : SIZEOF(.rodata);

  .sdata : ALIGN(4) {
    __global_pointer$ = . + 0x800;
    *(.sdata* .srodata*)
    . = ALIGN(4);
  } >REGION_DATA AT >REGION_TEXT
  __sdata_lma  = LOADADDR(.sdata);
  __sdata_vma  = ADDR(.sdata);
  __sdata_size = __sdata_lma == __sdata_vma  ? 0 : SIZEOF(.sdata);

  /* bss segment */
  .bss : ALIGN(4) {
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
  } >REGION_DATA
  __bss_vma  = ADDR(.bss);
  __bss_size = SIZEOF(.bss);

  .sbss : ALIGN(4) {
    *(.sbss*)
    . = ALIGN(4);
  } >REGION_DATA
  __sbss_vma = ADDR(.sbss);
  __sbss_size = SIZEOF(.sbss);

  /* For uninitalized data in sram, use NOLOAD to reduce .bin size and avoid putting them in bss, which saves init time */
  .noinit (NOLOAD) : ALIGN(4) {
    *(.noinit*)
    . = ALIGN(4);
  } >REGION_DATA

  /* thread-local data segment */
  .tdata : ALIGN(4) {
    _tls_data = .;
    *(.tdata*)
    . = ALIGN(4);
  } >REGION_DATA AT >REGION_TEXT
  .tbss : ALIGN(4) {
    *(.tbss*)
    . = ALIGN(4);
  } >REGION_DATA

  /* End of uninitalized data segement */
  . = ALIGN(4);
  end = .;
  __malloc_start = .;

  PROVIDE(__stack_size = 0x1000);
  .stack : {
    ASSERT((__malloc_start + __stack_size < SRAM_BASE + SRAM_SIZE), "Error: Not enough SRAM for stack");
  }
}
