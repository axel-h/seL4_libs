/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

//event definitions
//events common to all ARMV7A CPUs
#pragma once
#include <autoconf.h>

#define SEL4BENCH_EVENT_SOFTWARE_INCREMENT          0x00

/* generic events */
#define SEL4BENCH_EVENT_CACHE_L1I_MISS              0x01
#define SEL4BENCH_EVENT_CACHE_L1D_MISS              0x03
#define SEL4BENCH_EVENT_TLB_L1I_MISS                0x02
#define SEL4BENCH_EVENT_TLB_L1D_MISS                0x05

#ifndef CONFIG_ARM_CORTEX_A9
#define SEL4BENCH_EVENT_EXECUTE_INSTRUCTION         0x08
#endif /* CONFIG_ARM_CORTEX_A9 */

#define SEL4BENCH_EVENT_BRANCH_MISPREDICT           0x10

#define SEL4BENCH_EVENT_CACHE_L1D_HIT               0x04

#define SEL4BENCH_EVENT_MEMORY_READ                 0x06
#define SEL4BENCH_EVENT_MEMORY_WRITE                0x07
#define SEL4BENCH_EVENT_EXCEPTION                   0x09
#define SEL4BENCH_EVENT_EXCEPTION_RETURN            0x0A
#define SEL4BENCH_EVENT_CONTEXTIDR_WRITE            0x0B
#define SEL4BENCH_EVENT_SOFTWARE_PC_CHANGE          0x0C
#define SEL4BENCH_EVENT_EXECUTE_BRANCH_IMM          0x0D
#ifndef CONFIG_ARM_CORTEX_A9
#define SEL4BENCH_EVENT_FUNCTION_RETURN             0x0E
#endif /* CONFIG_ARM_CORTEX_A9 */
#define SEL4BENCH_EVENT_MEMORY_ACCESS_UNALIGNED     0x0F
#define SEL4BENCH_EVENT_CCNT                        0x11
#define SEL4BENCH_EVENT_EXECUTE_BRANCH_PREDICTABLE  0x12
/* Data memory access */
#define SEL4BENCH_EVENT_MEMORY_ACCESS                  0x13
/* Level 1 instruction cache access */
#define SEL4BENCH_EVENT_L1I_CACHE                   0x14
/* Level 1 data cache write-back */
#define SEL4BENCH_EVENT_L1D_CACHE_WB                0x15
/* Level 2 data cache access */
#define SEL4BENCH_EVENT_L2D_CACHE                   0x16
/* Level 2 data cache refill */
#define SEL4BENCH_EVENT_L2D_CACHE_REFILL            0x17
/* Level 2 data cache write-back */
#define SEL4BENCH_EVENT_L2D_CACHE_WB                0x18
/* Bus access */
#define SEL4BENCH_EVENT_BUS_ACCESS                  0x19
/* Local memory error */
#define SEL4BENCH_EVENT_MEMORY_ERROR                0x1A
/* Instruction speculatively executed */
#define SEL4BENCH_EVENT_INST_SPEC                   0x1B
/* Instruction architecturally executed, condition code check pass, write to TTBR */
#define SEL4BENCH_EVENT_TTBR_WRITE_RETIRED          0x1C
/* Bus cycle */
#define SEL4BENCH_EVENT_BUS_CYCLES                  0x1D

#include <sel4bench/cpu/events.h>
