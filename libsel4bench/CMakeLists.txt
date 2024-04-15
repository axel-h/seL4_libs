#
# Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.7.2)

project(libsel4bench C)

# Do a fail-safe check that all required configuration variables are set up
set(CHECK_CFG_VARS "KernelArch;KernelSel4Arch;KernelWordSize")
if(KernelArchARM)
    list(APPEND CHECK_CFG_VARS "KernelArmArmV;KernelArmCPU")
endif()
foreach(cfg_var IN LISTS CHECK_CFG_VARS)
    if((NOT DEFINED ${cfg_var}) OR (NOT ${cfg_var}))
        message(FATAL "${PROJECT_NAME} requires setting ${cfg_var}")
    endif()
    message("${PROJECT_NAME} ${cfg_var}: ${${cfg_var}}")
endforeach()

add_library(sel4bench EXCLUDE_FROM_ALL)

target_sources(
    sel4bench
    PRIVATE
    src/event_counters.c
    src/kernel_logging.c
    src/logging.c
    src/arch/${KernelArch}/event_counters.c
)

if(KernelArchX86)
    # empty
elseif(KernelArchARM)
    target_sources(
        sel4bench
        PRIVATE
        src/arch/${KernelArch}/armv/${KernelArmArmV}/event_counters.c
        src/arch/${KernelArch}/cpu/${KernelArmCPU}/event_counters.c
    )
    target_include_directories(
        sel4bench
        PUBLIC
            "arch_include/${KernelArch}/armv/${KernelArmArmV}"
            "arch_include/${KernelArch}/cpu/${KernelArmCPU}"
    )
    if(KernelArchArmV7ve)
        target_include_directories(sel4bench PUBLIC "arch_include/${KernelArch}/armv/armv7-a")
    endif()
elseif(KernelArchRiscV)
    # empty
else()
    message(FATAL_ERROR "Unsupported KernelArch '${KernelArch}'")
endif()

target_include_directories(
    sel4bench
    PUBLIC "sel4_arch_include/${KernelSel4Arch}" "arch_include/${KernelArch}" "include"
)

target_link_libraries(sel4bench muslc sel4 utils sel4_autoconf)
