OUTPUT_ARCH( "riscv" )
ENTRY(_start)

SECTIONS
{
  PROVIDE(__stack_pointer$ = SRAM_BASE + SRAM_SIZE);
  . = __boot_addr;

  .text.init __boot_addr : ALIGN(4) {
    *(.text.entry)
    *(.text.init)
    *(.text.init.*)
    . = ALIGN(4);
  } >REGION_TEXT

  .text.itim : ALIGN(4) {
    *(.text.itim);
    *(.text.itim.*);
    . = ALIGN(4);
  } >ITIM AT >REGION_TEXT
  __itim_text_lma  = LOADADDR(.text.itim);
  __itim_text_vma  = ADDR(.text.itim);
  __itim_text_size = __itim_text_lma == __itim_text_vma  ? 0 : SIZEOF(.text.itim);

  .text.sram : ALIGN(4) {
    *(.text.sram);
    *(.text.sram.*);
    . = ALIGN(4);
  } >SRAM AT >REGION_TEXT
  __sram_text_lma  = LOADADDR(.text.sram);
  __sram_text_vma  = ADDR(.text.sram);
  __sram_text_size = __sram_text_lma == __sram_text_vma  ? 0 : SIZEOF(.text.sram);
}
