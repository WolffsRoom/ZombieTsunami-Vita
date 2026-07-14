/*
 * Copyright (C) 2021      Andy Nguyen
 * Copyright (C) 2021-2022 Rinnegatamante
 * Copyright (C) 2022-2024 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "utils/init.h"

#include "utils/dialog.h"
#include "utils/glutil.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "utils/settings.h"

#include <reimpl/controls.h>

#include <string.h>

#include <psp2/appmgr.h>
#include <psp2/apputil.h>
#include <psp2/kernel/clib.h>
#include <psp2/power.h>

#include <falso_jni/FalsoJNI.h>
#include <so_util/so_util.h>
#include <fios/fios.h>

// Keep one 16 MiB arena per Android module. The actual PT_LOAD ranges are
// much smaller, but the spacing leaves room for so_util patch arenas.
#define FMODEX_LOAD_ADDRESS    0x98000000
#define FMODEVENT_LOAD_ADDRESS 0x99000000
#define CGAME_LOAD_ADDRESS     0x9A000000

extern so_module so_mod;
extern so_module so_fmodex_mod;
extern so_module so_fmodevent_mod;

void soloader_init_all() {
	// Launch `app0:configurator.bin` on `-config` init param
    sceAppUtilInit(&(SceAppUtilInitParam){}, &(SceAppUtilBootParam){});
    SceAppUtilAppEventParam eventParam;
    sceClibMemset(&eventParam, 0, sizeof(SceAppUtilAppEventParam));
    sceAppUtilReceiveAppEvent(&eventParam);
    if (eventParam.type == 0x05) {
        char buffer[2048];
        sceAppUtilAppEventParseLiveArea(&eventParam, buffer);
        if (strstr(buffer, "-config"))
            sceAppMgrLoadExec("app0:/configurator.bin", NULL, NULL);
    }

    // Set default overclock values
    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
    scePowerSetGpuXbarClockFrequency(166);

#ifdef USE_SCELIBC_IO
    if (fios_init(DATA_PATH) == 0)
        l_success("FIOS initialized.");
#endif

    if (!module_loaded("kubridge")) {
        l_fatal("kubridge is not loaded.");
        fatal_error("Error: kubridge.skprx is not installed.");
    }
    l_success("kubridge check passed.");

    if (!file_exists(CGAME_SO_PATH) ||
        !file_exists(FMODEX_SO_PATH) ||
        !file_exists(FMODEVENT_SO_PATH)) {
        fatal_error("Looks like you haven't installed the data files for this "
                    "port. Expected libcgame.so, libfmodex.so and "
                    "libfmodevent.so under %s.", DATA_PATH);
    }

    if (so_file_load(&so_fmodex_mod, FMODEX_SO_PATH, FMODEX_LOAD_ADDRESS) < 0) {
        fatal_error("Error: could not load %s.", FMODEX_SO_PATH);
    }
    if (so_file_load(&so_fmodevent_mod, FMODEVENT_SO_PATH, FMODEVENT_LOAD_ADDRESS) < 0) {
        fatal_error("Error: could not load %s.", FMODEVENT_SO_PATH);
    }
    if (so_file_load(&so_mod, CGAME_SO_PATH, CGAME_LOAD_ADDRESS) < 0) {
        fatal_error("Error: could not load %s.", CGAME_SO_PATH);
    }
    l_success("Android modules loaded.");

    settings_load();
    l_success("Settings loaded.");

    so_relocate(&so_fmodex_mod);
    so_relocate(&so_fmodevent_mod);
    so_relocate(&so_mod);
    l_success("Android modules relocated.");

    resolve_imports(&so_fmodex_mod);
    resolve_imports(&so_fmodevent_mod);
    resolve_imports(&so_mod);
    l_success("Android module imports resolved.");

    so_patch();
    l_success("SO patched.");

    so_flush_caches(&so_fmodex_mod);
    so_flush_caches(&so_fmodevent_mod);
    so_flush_caches(&so_mod);
    l_success("Android module caches flushed.");

    so_initialize(&so_fmodex_mod);
    so_initialize(&so_fmodevent_mod);
    so_initialize(&so_mod);
    l_success("Android modules initialized.");

    gl_preload();
    l_success("OpenGL preloaded.");

    jni_init();
    l_success("FalsoJNI initialized.");

    controls_init();
    l_success("Controls initialized.");
}
