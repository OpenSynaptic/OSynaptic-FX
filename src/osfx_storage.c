#include "../include/osfx_storage.h"
#include "../include/osfx_build_config.h"

#include <string.h>

#if OSFX_ENABLE_FILE_IO
#include <stdio.h>
#endif

static const osfx_storage_backend* g_backend = NULL;

#if OSFX_ENABLE_FILE_IO
static int stdio_write(void* handle, const char* text) {
    FILE* fp = (FILE*)handle;
    if (!fp || !text) {
        return 0;
    }
    return (fputs(text, fp) >= 0) ? 1 : 0;
}

static int stdio_read_line(void* handle, char* out, size_t out_cap) {
    FILE* fp = (FILE*)handle;
    if (!fp || !out || out_cap == 0U) {
        return -1;
    }
    if (!fgets(out, (int)out_cap, fp)) {
        return feof(fp) ? 0 : -1;
    }
    return 1;
}

static int stdio_close(void* handle) {
    FILE* fp = (FILE*)handle;
    if (!fp) {
        return 0;
    }
    return (fclose(fp) == 0) ? 1 : 0;
}

static int stdio_open_writer(void* user_ctx, const char* key, osfx_storage_writer* out_writer) {
    FILE* fp;
    (void)user_ctx;
    if (!key || !out_writer) {
        return 0;
    }
    fp = fopen(key, "wb");
    if (!fp) {
        return 0;
    }
    out_writer->handle = fp;
    out_writer->write = stdio_write;
    out_writer->close = stdio_close;
    return 1;
}

static int stdio_open_reader(void* user_ctx, const char* key, osfx_storage_reader* out_reader) {
    FILE* fp;
    (void)user_ctx;
    if (!key || !out_reader) {
        return 0;
    }
    fp = fopen(key, "rb");
    if (!fp) {
        return 0;
    }
    out_reader->handle = fp;
    out_reader->read_line = stdio_read_line;
    out_reader->close = stdio_close;
    return 1;
}

static const osfx_storage_backend g_stdio_backend = {
    NULL,
    stdio_open_writer,
    stdio_open_reader
};
#endif

void osfx_storage_set_backend(const osfx_storage_backend* backend) {
    g_backend = backend;
}

const osfx_storage_backend* osfx_storage_get_backend(void) {
    if (g_backend) {
        return g_backend;
    }
#if OSFX_ENABLE_FILE_IO
    return &g_stdio_backend;
#else
    return NULL;
#endif
}

int osfx_storage_open_writer(const char* key, osfx_storage_writer* out_writer) {
    const osfx_storage_backend* backend = osfx_storage_get_backend();
    if (!out_writer) {
        return 0;
    }
    memset(out_writer, 0, sizeof(*out_writer));
    if (!backend || !backend->open_writer) {
        return 0;
    }
    return backend->open_writer(backend->user_ctx, key, out_writer);
}

int osfx_storage_open_reader(const char* key, osfx_storage_reader* out_reader) {
    const osfx_storage_backend* backend = osfx_storage_get_backend();
    if (!out_reader) {
        return 0;
    }
    memset(out_reader, 0, sizeof(*out_reader));
    if (!backend || !backend->open_reader) {
        return 0;
    }
    return backend->open_reader(backend->user_ctx, key, out_reader);
}
