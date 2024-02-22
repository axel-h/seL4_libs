/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
/*
 * This file provides routines that can be called by other libraries to access
 * platform-specific devices such as the serial port. Perhaps some day this may
 * be refactored into a more structured userspace driver model, but for now we
 * just provide the bare minimum for userspace to access basic devices such as
 * the serial port on any platform.
 */

#include <autoconf.h>
#include <sel4platsupport/gen_config.h>
#include <sel4muslcsys/gen_config.h>
#include <assert.h>
#include <sel4/sel4.h>
#include <sel4/bootinfo.h>
#include <sel4/invocation.h>
#include <sel4platsupport/device.h>
#include <sel4platsupport/platsupport.h>
#include <vspace/page.h>
#include <simple/simple_helpers.h>
#include <vspace/vspace.h>
#include "plat_internal.h"
#include <stddef.h>
#include <stdio.h>
#include <vka/capops.h>
#include <string.h>
#include <sel4platsupport/arch/io.h>
#include <simple-default/simple-default.h>
#include <utils/util.h>

#if defined(CONFIG_LIB_SEL4_PLAT_SUPPORT_USE_SEL4_DEBUG_PUTCHAR) && defined(CONFIG_DEBUG_BUILD)
#define USE_DEBUG_PUTCHAR
#endif

enum serial_setup_status {
    NOT_INITIALIZED = 0,
    START_REGULAR_SETUP,
    START_FAILSAFE_SETUP,
    SETUP_COMPLETE
};

typedef struct {
    enum serial_setup_status setup_status;
#ifndef USE_DEBUG_PUTCHAR
    seL4_CPtr device_cap;
    ps_io_ops_t io_ops;
    vspace_t *vspace;
    vka_t *vka;
    /* To keep failsafe setup we need actual memory for a simple and a vka */
    simple_t simple_mem;
    vka_t vka_mem;
#endif /* not USE_DEBUG_PUTCHAR */
} ctx_t;

/* Some globals for tracking initialisation variables. This is currently just to avoid
 * passing parameters down to the platform code for backwards compatibility reasons. This
 * is strictly to avoid refactoring all existing platform code */
static ctx_t ctx = {
    .setup_status = NOT_INITIALIZED
};

extern char __executable_start[];

#ifndef USE_DEBUG_PUTCHAR

static void *__map_device_page(void *cookie, uintptr_t paddr, size_t size,
                               int cached, ps_mem_flags_t flags)
{
    seL4_Error err;

    if (0 != ctx.device_cap) {
        /* we only support a single page for the serial device. */
        abort();
        UNREACHABLE();
    }

    vka_object_t dest;
    int bits = CTZ(size);
    err = sel4platsupport_alloc_frame_at(ctx.vka, paddr, bits, &dest);
    if (err) {
        ZF_LOGE("Failed to get cap for serial device frame");
        abort();
        UNREACHABLE();
    }

    ctx.device_cap = dest.cptr;

    if ((ctx.setup_status == START_REGULAR_SETUP) && ctx.vspace)  {
        /* map device page regularly */
        void *vaddr = vspace_map_pages(ctx.vspace, &dest.cptr, NULL, seL4_AllRights, 1, bits, 0);
        if (!vaddr) {
            ZF_LOGE("Failed to map serial device");
            abort();
            UNREACHABLE();
        }
        return vaddr;
    }

    /* Try a last ditch attempt to get serial device going, so we can print out
     * an error. Find a properly aligned virtual address and try to map the
     * device cap there.
     */
    if ((ctx.setup_status == START_FAILSAFE_SETUP) || !ctx.vspace) {
        seL4_Word header_start = (seL4_Word)__executable_start - PAGE_SIZE_4K;
        seL4_Word vaddr = (header_start - BIT(bits)) & ~(BIT(bits) - 1);
        err = seL4_ARCH_Page_Map(ctx.device_cap, seL4_CapInitThreadPD, vaddr, seL4_AllRights, 0);
        if (err) {
            ZF_LOGE("Failed to map serial device in failsafe mode");
            abort();
            UNREACHABLE();
        }
        return (void *)vaddr;
    }

    ZF_LOGE("invalid setup state %d", ctx.setup_status);
    abort();
}

static int setup_io_ops(
    simple_t *simple __attribute__((unused)),
    vka_t *vka __attribute__((unused)))
{
    ctx.io_ops.io_mapper.io_map_fn = &__map_device_page;

#ifdef CONFIG_ARCH_X86
    sel4platsupport_get_io_port_ops(&ctx.io_ops.io_port_ops, simple, vka);
#endif
    return platsupport_serial_setup_io_ops(&ctx.io_ops);
}

#endif /* not USE_DEBUG_PUTCHAR */

/*
 * This function is designed to be called when creating a new cspace/vspace,
 * and the serial port needs to be hooked in there too.
 */
void platsupport_undo_serial_setup(void)
{
    /* Re-initialise some structures. */
    ctx.setup_status = NOT_INITIALIZED;
#ifndef USE_DEBUG_PUTCHAR
    if (ctx.device_cap) {
        cspacepath_t path;
        seL4_ARCH_Page_Unmap(ctx.device_cap);
        vka_cspace_make_path(ctx.vka, ctx.device_cap, &path);
        vka_cnode_delete(&path);
        vka_cspace_free(ctx.vka, ctx.device_cap);
        ctx.device_cap = 0;
    }
    ctx.vka = NULL;
#endif /* not USE_DEBUG_PUTCHAR */
}

/* Initialise serial input interrupt. */
void platsupport_serial_input_init_IRQ(void)
{
}

int platsupport_serial_setup_io_ops(ps_io_ops_t *io_ops)
{
    int err = 0;
    if (ctx.setup_status == SETUP_COMPLETE) {
        return 0;
    }
    err = __plat_serial_init(io_ops);
    if (!err) {
        ctx.setup_status = SETUP_COMPLETE;
    }
    return err;
}

int platsupport_serial_setup_bootinfo_failsafe(void)
{
    int err = 0;
    if (ctx.setup_status == SETUP_COMPLETE) {
        return 0;
    }

#ifdef USE_DEBUG_PUTCHAR
    /* only support putchar on a debug kernel */
    ctx.setup_status = SETUP_COMPLETE;
#else /* not USE_DEBUG_PUTCHAR */
    ctx.setup_status = START_FAILSAFE_SETUP;

    simple_t *simple = &(ctx.simple_mem);
    memset(simple, 0, sizeof(*simple));

    simple_default_init_bootinfo(simple, platsupport_get_bootinfo());

    vka_t *vka = &(ctx.vka_mem);
    memset(vka, 0, sizeof(*vka));
    simple_make_vka(simple, vka);

    ctx.vspace = NULL;
    ctx.vka = vka;

    err = setup_io_ops(simple, vka);
#endif /* [not] USE_DEBUG_PUTCHAR */
    return err;
}

int platsupport_serial_setup_simple(
    vspace_t *vspace __attribute__((unused)),
    simple_t *simple __attribute__((unused)),
    vka_t *vka __attribute__((unused)))
{
    int err = 0;
    switch (ctx.setup_status) {
    case SETUP_COMPLETE:
        return 0;
    case NOT_INITIALIZED:
        break; /* continue below */
    default:
        ZF_LOGE("Trying to initialise a partially initialised serial. Current setup status is %d\n", ctx.setup_status);
        assert(!"You cannot recover");
        return -1;
    }

#ifdef USE_DEBUG_PUTCHAR
    /* only support putchar on a debug kernel */
    ctx.setup_status = SETUP_COMPLETE;
#else /* not USE_DEBUG_PUTCHAR */
    /* start setup */
    ctx.setup_status = START_REGULAR_SETUP;
    ctx.vspace = vspace;
    ctx.vka = vka;
    err = setup_io_ops(simple, vka); /* uses ctx.vka */
#endif /* [not] USE_DEBUG_PUTCHAR */
    return err;
}

/* This function is called if an attempt for serial I/O is done before it has
 * been set up. Try to do some best-guess seetup.
 */
static int fallback_serial_setup()
{
    switch (ctx.setup_status) {

    case SETUP_COMPLETE:
        return 0; /* we don't except to be called in thi state */

    case NOT_INITIALIZED:
    case START_REGULAR_SETUP:
        break; /* continue below fir failsafe setup */

    default: /* includes START_FAILSAFE_SETUP */
        return -1;
    }

#ifdef CONFIG_LIB_SEL4_PLAT_SUPPORT_USE_SEL4_DEBUG_PUTCHAR
    ctx.setup_status = SETUP_COMPLETE;
    ZF_LOGI("using kernel syscalls for char I/O");
#else /* not CONFIG_LIB_SEL4_PLAT_SUPPORT_USE_SEL4_DEBUG_PUTCHAR */
    /* Attempt failsafe initialization to be able to print something. */
    int err = platsupport_serial_setup_bootinfo_failsafe();
    if (err || (START_REGULAR_SETUP != ctx.setup_status)) {
        /* Setup failed, so printing an error may not output anything. */
        ZF_LOGE("You attempted to print before initialising the"
                " libsel4platsupport serial device!");
        return -1;
    }

    /* Setup worked, so this warning will show up. */
    ZF_LOGW("Regular serial setup failed.\n"
            "This message coming to you courtesy of failsafe serial.\n"
            "Your vspace has been clobbered but we will keep running"
            " to get any more error output");
#endif /* [not] CONFIG_LIB_SEL4_PLAT_SUPPORT_USE_SEL4_DEBUG_PUTCHAR */
    return 0;
}

#ifdef CONFIG_LIB_SEL4_MUSLC_SYS_ARCH_PUTCHAR_WEAK
#define LIBSEL4_IO NO_INLINE WEAK
#else
#define LIBSEL4_IO NO_INLINE
#endif

void LIBSEL4_IO __arch_putchar(int c)
{
    if (ctx.setup_status != SETUP_COMPLETE) {
        if (0 != fallback_serial_setup()) {
            abort(); /* ToDo: hopefully this does not print anything */
            UNREACHABLE();
        }
    }
    __plat_putchar(c);
}

size_t LIBSEL4_IO __arch_write(char *data, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        __arch_putchar(data[i]);
    }
    return count;
}

int __arch_getchar(void)
{
    if (ctx.setup_status != SETUP_COMPLETE) {
        if (0 != fallback_serial_setup()) {
            abort(); /* ToDo: hopefully this does not print anything */
            UNREACHABLE();
        }
    }
    return __plat_getchar();
}
