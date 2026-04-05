#ifndef OSFX_STORAGE_H
#define OSFX_STORAGE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct osfx_storage_writer {
    void* handle;
    int (*write)(void* handle, const char* text);
    int (*close)(void* handle);
} osfx_storage_writer;

typedef struct osfx_storage_reader {
    void* handle;
    /*
     * Return value:
     *   1: one line is read
     *   0: EOF
     *  <0: read error
     */
    int (*read_line)(void* handle, char* out, size_t out_cap);
    int (*close)(void* handle);
} osfx_storage_reader;

typedef struct osfx_storage_backend {
    void* user_ctx;
    int (*open_writer)(void* user_ctx, const char* key, osfx_storage_writer* out_writer);
    int (*open_reader)(void* user_ctx, const char* key, osfx_storage_reader* out_reader);
} osfx_storage_backend;

/* Register a custom backend. Pass NULL to reset to built-in default backend. */
void osfx_storage_set_backend(const osfx_storage_backend* backend);

/* Returns custom backend if registered, otherwise built-in backend when available. */
const osfx_storage_backend* osfx_storage_get_backend(void);

int osfx_storage_open_writer(const char* key, osfx_storage_writer* out_writer);
int osfx_storage_open_reader(const char* key, osfx_storage_reader* out_reader);

#ifdef __cplusplus
}
#endif

#endif
