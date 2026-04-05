#ifndef OSFX_STORAGE_LITTLEFS_H
#define OSFX_STORAGE_LITTLEFS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Mount LittleFS and set osfx_storage backend to LittleFS. */
int osfx_storage_use_littlefs(void);

/*
 * Mount LittleFS and set osfx_storage backend with a base prefix.
 * Keys passed to osfx_storage_open_* will be resolved under this prefix
 * when they are not absolute paths.
 */
int osfx_storage_use_littlefs_at(const char* base_prefix);

/* Returns 1 when LittleFS backend is mounted and active, otherwise 0. */
int osfx_storage_littlefs_is_ready(void);

#ifdef __cplusplus
}
#endif

#endif
