#include "osfx_storage_littlefs.h"

#include "../include/osfx_storage.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#if (defined(ESP32) || defined(ARDUINO_ARCH_ESP32)) && defined(__has_include)
#if __has_include(<esp_littlefs.h>) && __has_include(<esp_err.h>)
#include <esp_littlefs.h>
#include <esp_err.h>
#define OSFX_HAVE_ESP32_LITTLEFS_CAPI 1
#else
#define OSFX_HAVE_ESP32_LITTLEFS_CAPI 0
#endif
#else
#define OSFX_HAVE_ESP32_LITTLEFS_CAPI 0
#endif

static int g_lfs_ready = 0;
static char g_base_prefix[32] = "/";

static int osfx_lfs_write(void* handle, const char* text) {
    FILE* fp = (FILE*)handle;
    if (!fp || !text) {
        return 0;
    }
    return (fputs(text, fp) >= 0) ? 1 : 0;
}

static int osfx_lfs_read_line(void* handle, char* out, size_t out_cap) {
    FILE* fp = (FILE*)handle;
    if (!fp || !out || out_cap == 0U) {
        return -1;
    }
    if (!fgets(out, (int)out_cap, fp)) {
        return feof(fp) ? 0 : -1;
    }
    return 1;
}

static int osfx_lfs_close(void* handle) {
    FILE* fp = (FILE*)handle;
    if (!fp) {
        return 0;
    }
    return (fclose(fp) == 0) ? 1 : 0;
}

static int osfx_path_join(const char* prefix, const char* suffix, char* out_path, size_t out_cap) {
    int n;
    if (!prefix || !suffix || !out_path || out_cap == 0U) {
        return 0;
    }

    if (prefix[0] == '\0' || strcmp(prefix, "/") == 0) {
        n = snprintf(out_path, out_cap, "%s", suffix);
    } else if (suffix[0] == '/') {
        n = snprintf(out_path, out_cap, "%s%s", prefix, suffix);
    } else {
        n = snprintf(out_path, out_cap, "%s/%s", prefix, suffix);
    }

    return (n > 0 && (size_t)n < out_cap) ? 1 : 0;
}

static int osfx_lfs_normalize_key(const char* key, char* out_path, size_t out_path_cap) {
    const char* resolved = key;

    if (!key || !out_path || out_path_cap == 0U) {
        return 0;
    }

    /* Backward compatibility: map old SPIFFS prefix to LittleFS-like path. */
    if (strncmp(resolved, "/spiffs/", 8) == 0) {
        resolved += 7;
    }

    if (resolved[0] != '/') {
        return osfx_path_join(g_base_prefix, resolved, out_path, out_path_cap);
    }

    if (strcmp(g_base_prefix, "/") == 0 || g_base_prefix[0] == '\0') {
        return osfx_path_join("/", resolved, out_path, out_path_cap);
    }

    if (strncmp(resolved, g_base_prefix, strlen(g_base_prefix)) == 0) {
        int is_exact = resolved[strlen(g_base_prefix)] == '\0';
        int has_sep = resolved[strlen(g_base_prefix)] == '/';
        if (is_exact || has_sep) {
            return osfx_path_join("/", resolved, out_path, out_path_cap);
        }
    }

    return osfx_path_join(g_base_prefix, resolved, out_path, out_path_cap);
}

static int osfx_lfs_open_writer(void* user_ctx, const char* key, osfx_storage_writer* out_writer) {
    FILE* fp;
    char path[128];
    (void)user_ctx;

    if (!out_writer || !osfx_lfs_normalize_key(key, path, sizeof(path))) {
        return 0;
    }

    fp = fopen(path, "wb");
    if (!fp) {
        return 0;
    }

    out_writer->handle = fp;
    out_writer->write = osfx_lfs_write;
    out_writer->close = osfx_lfs_close;
    return 1;
}

static int osfx_lfs_open_reader(void* user_ctx, const char* key, osfx_storage_reader* out_reader) {
    FILE* fp;
    char path[128];
    (void)user_ctx;

    if (!out_reader || !osfx_lfs_normalize_key(key, path, sizeof(path))) {
        return 0;
    }

    fp = fopen(path, "rb");
    if (!fp) {
        return 0;
    }

    out_reader->handle = fp;
    out_reader->read_line = osfx_lfs_read_line;
    out_reader->close = osfx_lfs_close;
    return 1;
}

static osfx_storage_backend g_littlefs_backend = {
    NULL,
    osfx_lfs_open_writer,
    osfx_lfs_open_reader
};

static int osfx_set_prefix(const char* base_prefix) {
    size_t n;

    if (base_prefix && base_prefix[0] != '\0') {
        n = strlen(base_prefix);
        if (n >= sizeof(g_base_prefix)) {
            return 0;
        }
        memcpy(g_base_prefix, base_prefix, n + 1U);
        return 1;
    }

    strcpy(g_base_prefix, "/");
    return 1;
}

#if OSFX_HAVE_ESP32_LITTLEFS_CAPI
static int osfx_mount_littlefs_if_needed(void) {
    static int mounted_once = 0;
    esp_vfs_littlefs_conf_t conf;
    esp_err_t err;

    if (mounted_once) {
        return 1;
    }

    memset(&conf, 0, sizeof(conf));
    conf.base_path = g_base_prefix;
    conf.partition_label = "spiffs";
    conf.format_if_mount_failed = true;
    conf.dont_mount = false;

    err = esp_vfs_littlefs_register(&conf);
    if (err == ESP_OK || err == ESP_ERR_INVALID_STATE) {
        mounted_once = 1;
        return 1;
    }
    return 0;
}
#endif

int osfx_storage_use_littlefs_at(const char* base_prefix) {
    if (!osfx_set_prefix(base_prefix)) {
        g_lfs_ready = 0;
        return 0;
    }

#if OSFX_HAVE_ESP32_LITTLEFS_CAPI
    if (!osfx_mount_littlefs_if_needed()) {
        g_lfs_ready = 0;
        return 0;
    }
    osfx_storage_set_backend(&g_littlefs_backend);
    g_lfs_ready = 1;
    return 1;
#else
    g_lfs_ready = 0;
    return 0;
#endif
}

int osfx_storage_use_littlefs(void) {
    return osfx_storage_use_littlefs_at("/littlefs");
}

int osfx_storage_littlefs_is_ready(void) {
    return g_lfs_ready;
}
