/*
 * FreeRTOS Kernel V10.4.6
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

/*
 * The FreeRTOS kernel's RISC-V port is split between the the code that is
 * common across all currently supported RISC-V chips (implementations of the
 * RISC-V ISA), and code that tailors the port to a specific RISC-V chip:
 *
 * + FreeRTOS\Source\portable\GCC\RISC-V\portASM.S contains the code that
 *   is common to all currently supported RISC-V chips.  There is only one
 *   portASM.S file because the same file is built for all RISC-V target chips.
 *
 * + Header files called freertos_risc_v_chip_specific_extensions.h contain the
 *   code that tailors the FreeRTOS kernel's RISC-V port to a specific RISC-V
 *   chip.  There are multiple freertos_risc_v_chip_specific_extensions.h files
 *   as there are multiple RISC-V chip implementations.
 *
 * !!!NOTE!!!
 * TAKE CARE TO INCLUDE THE CORRECT freertos_risc_v_chip_specific_extensions.h
 * HEADER FILE FOR THE CHIP IN USE.  This is done using the assembler's (not the
 * compiler's!) include path.  For example, if the chip in use includes a core
 * local interrupter (CLINT) and does not include any chip specific register
 * extensions then add the path below to the assembler's include path:
 * FreeRTOS\Source\portable\GCC\RISC-V\chip_specific_extensions\RISCV_MTIME_CLINT_no_extensions
 *
 */


#ifndef __FREERTOS_RISC_V_EXTENSIONS_H__
#define __FREERTOS_RISC_V_EXTENSIONS_H__

#include "agrv.h"

#define portasmHAS_SIFIVE_CLINT         1
#define portasmHAS_MTIME                1
#if AGRV_FP_STACK == 1
#define portasmADDITIONAL_CONTEXT_SIZE  32
#else
#define portasmADDITIONAL_CONTEXT_SIZE  0
#endif

#define freertos_risc_v_trap_handler trap_entry
#define freertos_risc_v_application_interrupt_handler handle_trap

.macro portasmSAVE_ADDITIONAL_REGISTERS
#if AGRV_FP_STACK == 1
  addi    sp,  sp, -FSTKSIZE
  FSTORE  f0,  0 * FREGBYTES(sp)
  FSTORE  f1,  1 * FREGBYTES(sp)
  FSTORE  f2,  2 * FREGBYTES(sp)
  FSTORE  f3,  3 * FREGBYTES(sp)
  FSTORE  f4,  4 * FREGBYTES(sp)
  FSTORE  f5,  5 * FREGBYTES(sp)
  FSTORE  f6,  6 * FREGBYTES(sp)
  FSTORE  f7,  7 * FREGBYTES(sp)
  FSTORE  f8,  8 * FREGBYTES(sp)
  FSTORE  f9,  9 * FREGBYTES(sp)
  FSTORE  f10, 10 * FREGBYTES(sp)
  FSTORE  f11, 11 * FREGBYTES(sp)
  FSTORE  f12, 12 * FREGBYTES(sp)
  FSTORE  f13, 13 * FREGBYTES(sp)
  FSTORE  f14, 14 * FREGBYTES(sp)
  FSTORE  f15, 15 * FREGBYTES(sp)
  FSTORE  f16, 16 * FREGBYTES(sp)
  FSTORE  f17, 17 * FREGBYTES(sp)
  FSTORE  f18, 18 * FREGBYTES(sp)
  FSTORE  f19, 19 * FREGBYTES(sp)
  FSTORE  f20, 20 * FREGBYTES(sp)
  FSTORE  f21, 21 * FREGBYTES(sp)
  FSTORE  f22, 22 * FREGBYTES(sp)
  FSTORE  f23, 23 * FREGBYTES(sp)
  FSTORE  f24, 24 * FREGBYTES(sp)
  FSTORE  f25, 25 * FREGBYTES(sp)
  FSTORE  f26, 26 * FREGBYTES(sp)
  FSTORE  f27, 27 * FREGBYTES(sp)
  FSTORE  f28, 28 * FREGBYTES(sp)
  FSTORE  f29, 29 * FREGBYTES(sp)
  FSTORE  f30, 30 * FREGBYTES(sp)
  FSTORE  f31, 31 * FREGBYTES(sp)
#endif
    .endm

.macro portasmRESTORE_ADDITIONAL_REGISTERS
#if AGRV_FP_STACK == 1
  FLOAD   f0, 0 * FREGBYTES(sp)
  FLOAD   f1, 1 * FREGBYTES(sp)
  FLOAD   f2, 2 * FREGBYTES(sp)
  FLOAD   f3, 3 * FREGBYTES(sp)
  FLOAD   f4, 4 * FREGBYTES(sp)
  FLOAD   f5, 5 * FREGBYTES(sp)
  FLOAD   f6, 6 * FREGBYTES(sp)
  FLOAD   f7, 7 * FREGBYTES(sp)
  FLOAD   f8, 8 * FREGBYTES(sp)
  FLOAD   f9, 9 * FREGBYTES(sp)
  FLOAD   f10, 10 * FREGBYTES(sp)
  FLOAD   f11, 11 * FREGBYTES(sp)
  FLOAD   f12, 12 * FREGBYTES(sp)
  FLOAD   f13, 13 * FREGBYTES(sp)
  FLOAD   f14, 14 * FREGBYTES(sp)
  FLOAD   f15, 15 * FREGBYTES(sp)
  FLOAD   f16, 16 * FREGBYTES(sp)
  FLOAD   f17, 17 * FREGBYTES(sp)
  FLOAD   f18, 18 * FREGBYTES(sp)
  FLOAD   f19, 19 * FREGBYTES(sp)
  FLOAD   f20, 20 * FREGBYTES(sp)
  FLOAD   f21, 21 * FREGBYTES(sp)
  FLOAD   f22, 22 * FREGBYTES(sp)
  FLOAD   f23, 23 * FREGBYTES(sp)
  FLOAD   f24, 24 * FREGBYTES(sp)
  FLOAD   f25, 25 * FREGBYTES(sp)
  FLOAD   f26, 26 * FREGBYTES(sp)
  FLOAD   f27, 27 * FREGBYTES(sp)
  FLOAD   f28, 28 * FREGBYTES(sp)
  FLOAD   f29, 29 * FREGBYTES(sp)
  FLOAD   f30, 30 * FREGBYTES(sp)
  FLOAD   f31, 31 * FREGBYTES(sp)
  addi    sp, sp, FSTKSIZE
#endif
    .endm

#endif /* __FREERTOS_RISC_V_EXTENSIONS_H__ */
