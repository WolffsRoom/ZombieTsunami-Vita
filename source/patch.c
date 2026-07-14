/*
 * Copyright (C) 2023 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

/**
 * @file  patch.c
 * @brief Patching some of the .so internal functions or bridging them to native
 *        for better compatibility.
 */

#include <kubridge.h>
#include <so_util/so_util.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/syslimits.h>

#include <psp2/io/fcntl.h>
#include <psp2/audioout.h>

#include "utils/logger.h"

extern so_module so_mod;
extern so_module so_fmodex_mod;
extern so_module so_fmodevent_mod;

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

typedef int (*cfile_open_fn)(void *self, const char *path, const char *mode);
typedef void (*cfile_get_directory_fn)(const char *path, void *directory);
typedef void (*cstring_fill_fn)(void *self, const char *text, uint32_t length);
typedef int (*fmod_set_software_format_fn)(void *system, int sample_rate,
                                           int format, int output_channels,
                                           int max_input_channels,
                                           int resampler);
typedef int (*fmod_set_dsp_buffer_size_fn)(void *system,
                                           uint32_t buffer_length,
                                           int num_buffers);

static cfile_open_fn cfile_open;
static cfile_get_directory_fn cfile_get_directory;
static cstring_fill_fn cstring_fill;
static fmod_set_software_format_fn fmod_set_software_format;
static fmod_set_dsp_buffer_size_fn fmod_set_dsp_buffer_size;
static uintptr_t cfile_vtable;
static so_hook android_file_mgr_open_hook;
static so_hook fmod_system_create_hook;
static so_hook fmod_set_output_hook;
static so_hook fmod_system_init_hook;
static so_hook fmod_event_init_hook;
static so_hook fmod_create_sound_hook;
static so_hook fmod_create_stream_hook;
static so_hook fmod_play_sound_hook;
static uint32_t fmod_create_sound_count;
static uint32_t fmod_play_sound_count;

void port_trace(const char *format, ...) {
    char line[768];
    va_list args;
    va_start(args, format);
    int length = vsnprintf(line, sizeof(line) - 2, format, args);
    va_end(args);

    if (length < 0) return;
    if (length > (int)sizeof(line) - 2) length = sizeof(line) - 2;
    line[length++] = '\n';

    SceUID fd = sceIoOpen(DATA_PATH "port.log",
                          SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0666);
    if (fd >= 0) {
        sceIoWrite(fd, line, length);
        sceIoClose(fd);
    }
}

// AndroidFileMgr normally asks Java's AssetManager to open bundle resources.
// The Vita data directory contains the APK's extracted assets instead, so
// bypass that Java-only path and feed the normal native FileMgr a Vita path.
static const char *asset_relative_path(const char *path) {
    static const char bundle_prefix[] = "bundle://";
    const char *relative = path;

    if (!path) return NULL;

    if (strncmp(relative, bundle_prefix, sizeof(bundle_prefix) - 1) == 0) {
        relative += sizeof(bundle_prefix) - 1;
    }

    // The sprite loader builds texture paths as ./<CFile directory>/<name>.
    while (strncmp(relative, "./", 2) == 0) relative += 2;
    while (*relative == '/') relative++;

    if (strncmp(relative, "res/", 4) != 0 &&
        strncmp(relative, "assets/", 7) != 0) {
        return NULL;
    }

    return relative;
}

static const char *asset_path(const char *path, char translated[PATH_MAX]) {
    const char *relative = asset_relative_path(path);
    if (!relative) return path;

    int length;
    if (strncmp(relative, "assets/", 7) == 0) {
        length = snprintf(translated, PATH_MAX, "%s%s", DATA_PATH, relative);
    } else {
        length = snprintf(translated, PATH_MAX, "%sassets/%s",
                          DATA_PATH, relative);
    }

    if (length < 0 || length >= PATH_MAX) {
        l_warn("Asset path is too long: %s", path);
        return path;
    }

    return translated;
}

static int hooked_bundle_check_file_exists(void *self, const char *path) {
    (void)self;

    char translated[PATH_MAX];
    const char *real_path = asset_path(path, translated);
    if (!real_path || real_path == path) {
        port_trace("BundleExists unsupported: %s", path ? path : "(null)");
        l_warn("BundleCheckFileExists: unsupported path %s",
               path ? path : "(null)");
        return 0;
    }

    FILE *file = fopen(real_path, "rb");
    if (!file) {
        port_trace("BundleExists missing: %s -> %s", path, real_path);
        l_warn("BundleCheckFileExists: missing %s (from %s)", real_path, path);
        return 0;
    }

    fclose(file);
    port_trace("BundleExists found: %s -> %s", path, real_path);
    l_debug("BundleCheckFileExists: found %s", real_path);
    return 1;
}

static void *hooked_android_file_mgr_open(void *self, const char *path,
                                          const char *mode) {
    (void)self;

    char translated[PATH_MAX];
    const char *real_path = asset_path(path, translated);

    if (real_path && real_path != path) {
        port_trace("AndroidOpen enter: %s -> %s mode=%s", path, real_path,
                  mode ? mode : "(null)");
        l_debug("AndroidFileMgr::OpenFile: %s -> %s", path, real_path);

        // FileMgr::OpenFile would run GetFileFullPath once more and mangle an
        // already absolute Vita path. Construct its small CFile object exactly
        // as that routine does, then invoke CFile::OpenFile on the final path.
        uint8_t *file = calloc(1, 24);
        if (!file) {
            port_trace("AndroidOpen allocation failed");
            l_warn("AndroidFileMgr::OpenFile: CFile allocation failed");
            return NULL;
        }

        *(uintptr_t *)file = cfile_vtable + 8;
        *(uint32_t *)(file + 20) = 1;

        const char *opened_path = real_path;
        char jet_path[PATH_MAX];
        if (!cfile_open(file, opened_path, mode)) {
            // A few APK resources keep an extra .jet suffix (notably the
            // FMOD .fsb bank) while the game asks for the unsuffixed name.
            int jet_length = snprintf(jet_path, sizeof(jet_path),
                                      "%s.jet", real_path);
            if (jet_length < 0 || jet_length >= (int)sizeof(jet_path) ||
                !cfile_open(file, jet_path, mode)) {
                port_trace("AndroidOpen CFile failed: %s", real_path);
                l_warn("AndroidFileMgr::OpenFile: could not open %s", real_path);
                free(file);
                return NULL;
            }
            opened_path = jet_path;
            port_trace("AndroidOpen used .jet fallback: %s", opened_path);
        }

        const char *resource_path = asset_relative_path(path);
        cstring_fill(file + 12, resource_path,
                     (uint32_t)strlen(resource_path));
        cfile_get_directory(resource_path, file + 16);
        file[8] = 1;

        port_trace("AndroidOpen success: %s CFile=%p dir=%s",
                  opened_path, file, resource_path);
        return file;
    }

    port_trace("AndroidOpen passthrough: %s mode=%s",
              path ? path : "(null)", mode ? mode : "(null)");
    void *result = SO_CONTINUE(void *, android_file_mgr_open_hook,
                               self, path, mode);
    port_trace("AndroidOpen passthrough result=%p", result);
    return result;
}

static int hooked_fmod_system_create(void **system) {
    int result = SO_CONTINUE(int, fmod_system_create_hook, system);
    port_trace("FMOD_System_Create result=%d system=%p", result,
               system ? *system : NULL);
    return result;
}

static int hooked_fmod_set_output(void *self, int output_type) {
    /*
     * FMOD 4 selects AudioTrack on this Android build.  AudioTrack depends on
     * the Java android.media bridge, which FalsoJNI does not implement.  The
     * adjacent OpenSL ES backend is serviced by our opensles_soloader bridge.
     */
    enum {
        FMOD_OUTPUTTYPE_AUDIOTRACK_4X = 21,
        FMOD_OUTPUTTYPE_OPENSL_4X = 22,
    };
    int selected_output = output_type == FMOD_OUTPUTTYPE_AUDIOTRACK_4X
                              ? FMOD_OUTPUTTYPE_OPENSL_4X
                              : output_type;
    int result = SO_CONTINUE(int, fmod_set_output_hook, self, selected_output);
    port_trace("FMOD::System::setOutput requested=%d selected=%d result=%d",
               output_type, selected_output, result);
    return result;
}

static int hooked_fmod_system_init(void *self, int max_channels,
                                   uint32_t flags, void *extra) {
    enum {
        FMOD_SOUND_FORMAT_PCM16_4X = 2,
        FMOD_DSP_RESAMPLER_LINEAR_4X = 1,
    };
    int format_result = -1;
    int buffer_result = -1;
    if (fmod_set_software_format) {
        format_result = fmod_set_software_format(
            self, 48000, FMOD_SOUND_FORMAT_PCM16_4X, 0, 0,
            FMOD_DSP_RESAMPLER_LINEAR_4X);
    }
    if (fmod_set_dsp_buffer_size)
        buffer_result = fmod_set_dsp_buffer_size(self, 1024, 4);
    port_trace("FMOD output config rate=48000 PCM16 format_result=%d dsp_frames=1024 dsp_result=%d",
               format_result, buffer_result);

    int result = SO_CONTINUE(int, fmod_system_init_hook,
                             self, max_channels, flags, extra);
    int bgm_adopt = sceAudioOutGetAdopt(SCE_AUDIO_OUT_PORT_TYPE_BGM);
    port_trace("FMOD::System::init channels=%d flags=0x%x result=%d BGM_adopt=%d",
               max_channels, flags, result, bgm_adopt);
    return result;
}

static int hooked_fmod_event_init(void *self, int max_channels,
                                  uint32_t flags, void *extra,
                                  uint32_t event_flags) {
    int result = SO_CONTINUE(int, fmod_event_init_hook,
                             self, max_channels, flags, extra, event_flags);
    port_trace("FMOD::EventSystem::init channels=%d flags=0x%x event=0x%x result=%d",
               max_channels, flags, event_flags, result);
    return result;
}

static const char *fmod_trace_name(const char *name) {
    if (!name)
        return "(null)";
    if (strncmp(name, "res/", 4) == 0 ||
        strncmp(name, "bundle://", 9) == 0 ||
        strncmp(name, "ux0:", 4) == 0)
        return name;
    return "(memory/data)";
}

static int hooked_fmod_create_sound(void *self, const char *name,
                                    uint32_t mode, void *exinfo,
                                    void **sound) {
    int result = SO_CONTINUE(int, fmod_create_sound_hook,
                             self, name, mode, exinfo, sound);
    uint32_t call = fmod_create_sound_count++;
    if (call < 24 || result != 0)
        port_trace("FMOD::System::createSound #%u name=%s ptr=%p mode=0x%x result=%d sound=%p",
                   call + 1, fmod_trace_name(name), name, mode, result,
                   sound ? *sound : NULL);
    return result;
}

static int hooked_fmod_create_stream(void *self, const char *name,
                                     uint32_t mode, void *exinfo,
                                     void **sound) {
    /*
     * The Android build opens music with FMOD_NONBLOCKING.  Its background
     * file worker never finishes under the Vita loader, while a synchronous
     * createStream returns FMOD_ERR_INTERNAL.  Load MP3 music through
     * createSound as a compressed sample instead: this reads the local file
     * immediately but leaves MPEG decoding to the mixer, avoiding a large PCM
     * allocation.  Keep a decoded-sample attempt and finally the original
     * nonblocking stream as safe fallbacks.
     */
    const uint32_t fmod_nonblocking = 0x00010000u;
    const uint32_t fmod_create_compressed_sample = 0x00000200u;
    uint32_t selected_mode = mode & ~fmod_nonblocking;
    bool is_mp3 = name && strstr(name, ".mp3") != NULL;
    int result;

    if (is_mp3 && fmod_create_sound_hook.addr) {
        uint32_t compressed_mode = selected_mode |
                                   fmod_create_compressed_sample;
        result = SO_CONTINUE(int, fmod_create_sound_hook,
                             self, name, compressed_mode, exinfo, sound);
        port_trace("FMOD music createSound(compressed) name=%s requested=0x%x selected=0x%x result=%d sound=%p",
                   fmod_trace_name(name), mode, compressed_mode, result,
                   sound ? *sound : NULL);

        if (result != 0) {
            if (sound) *sound = NULL;
            result = SO_CONTINUE(int, fmod_create_sound_hook,
                                 self, name, selected_mode, exinfo, sound);
            port_trace("FMOD music createSound(decoded) name=%s selected=0x%x result=%d sound=%p",
                       fmod_trace_name(name), selected_mode, result,
                       sound ? *sound : NULL);
        }

        if (result == 0)
            return result;

        if (sound) *sound = NULL;
        port_trace("FMOD music sample routes failed; restoring original nonblocking stream");
    }

    result = SO_CONTINUE(int, fmod_create_stream_hook,
                         self, name, mode, exinfo, sound);
    port_trace("FMOD::System::createStream fallback name=%s ptr=%p mode=0x%x result=%d sound=%p",
               fmod_trace_name(name), name, mode, result,
               sound ? *sound : NULL);
    return result;
}

static int hooked_fmod_play_sound(void *self, int channel_index, void *sound,
                                  int paused, void **channel) {
    int result = SO_CONTINUE(int, fmod_play_sound_hook,
                             self, channel_index, sound, paused, channel);
    uint32_t call = fmod_play_sound_count++;
    if (call < 32 || result != 0)
        port_trace("FMOD::System::playSound #%u sound=%p paused=%d result=%d channel=%p",
                   call + 1, sound, paused, result,
                   channel ? *channel : NULL);
    return result;
}

void so_patch(void) {
    uintptr_t android_open = so_symbol(
        &so_mod, "_ZN4Mobi14AndroidFileMgr8OpenFileEPKcS2_");
    uintptr_t bundle_exists = so_symbol(
        &so_mod, "_ZN4Mobi14AndroidFileMgr21BundleCheckFileExistsEPKc");
    cfile_open = (cfile_open_fn)so_symbol(
        &so_mod, "_ZN4Mobi5CFile8OpenFileEPKcS2_");
    cfile_get_directory = (cfile_get_directory_fn)so_symbol(
        &so_mod, "_ZN4Mobi5CFile12GetDirectoryEPKcRNS_7CStringE");
    cstring_fill = (cstring_fill_fn)so_symbol(
        &so_mod, "_ZN4Mobi7CString10FillStringEPKcj");
    cfile_vtable = so_symbol(&so_mod, "_ZTVN4Mobi5CFileE");
    uintptr_t fmod_system_create = so_symbol(
        &so_fmodex_mod, "FMOD_System_Create");
    uintptr_t fmod_set_output = so_symbol(
        &so_fmodex_mod, "_ZN4FMOD6System9setOutputE15FMOD_OUTPUTTYPE");
    fmod_set_software_format = (fmod_set_software_format_fn)so_symbol(
        &so_fmodex_mod, "FMOD_System_SetSoftwareFormat");
    fmod_set_dsp_buffer_size = (fmod_set_dsp_buffer_size_fn)so_symbol(
        &so_fmodex_mod, "FMOD_System_SetDSPBufferSize");
    uintptr_t fmod_system_init = so_symbol(
        &so_fmodex_mod, "_ZN4FMOD6System4initEijPv");
    uintptr_t fmod_event_init = so_symbol(
        &so_fmodevent_mod, "_ZN4FMOD11EventSystem4initEijPvj");
    uintptr_t fmod_create_sound = so_symbol(
        &so_fmodex_mod,
        "_ZN4FMOD6System11createSoundEPKcjP22FMOD_CREATESOUNDEXINFOPPNS_5SoundE");
    uintptr_t fmod_create_stream = so_symbol(
        &so_fmodex_mod,
        "_ZN4FMOD6System12createStreamEPKcjP22FMOD_CREATESOUNDEXINFOPPNS_5SoundE");
    uintptr_t fmod_play_sound = so_symbol(
        &so_fmodex_mod,
        "_ZN4FMOD6System9playSoundE17FMOD_CHANNELINDEXPNS_5SoundEbPPNS_7ChannelE");

    if (!android_open || !bundle_exists || !cfile_open || !cfile_vtable ||
        !cfile_get_directory || !cstring_fill) {
        l_fatal("Could not resolve AndroidFileMgr hooks.");
        return;
    }

    sceIoRemove(DATA_PATH "port.log");
    port_trace("Zombie Tsunami Vita port trace v21 (native 48 kHz mixer)");

    android_file_mgr_open_hook = hook_addr(
        android_open, (uintptr_t)&hooked_android_file_mgr_open);
    hook_addr(bundle_exists, (uintptr_t)&hooked_bundle_check_file_exists);

    if (fmod_system_create)
        fmod_system_create_hook = hook_addr(
            fmod_system_create, (uintptr_t)&hooked_fmod_system_create);
    if (fmod_set_output)
        fmod_set_output_hook = hook_addr(
            fmod_set_output, (uintptr_t)&hooked_fmod_set_output);
    if (fmod_system_init)
        fmod_system_init_hook = hook_addr(
            fmod_system_init, (uintptr_t)&hooked_fmod_system_init);
    if (fmod_event_init)
        fmod_event_init_hook = hook_addr(
            fmod_event_init, (uintptr_t)&hooked_fmod_event_init);
    if (fmod_create_sound)
        fmod_create_sound_hook = hook_addr(
            fmod_create_sound, (uintptr_t)&hooked_fmod_create_sound);
    if (fmod_create_stream)
        fmod_create_stream_hook = hook_addr(
            fmod_create_stream, (uintptr_t)&hooked_fmod_create_stream);
    if (fmod_play_sound)
        fmod_play_sound_hook = hook_addr(
            fmod_play_sound, (uintptr_t)&hooked_fmod_play_sound);
}
