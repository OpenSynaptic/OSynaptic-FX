/*
 * osfx_cli_main.c
 * ---------------
 * Native-host entry point for the OSynaptic-FX CLI lite.
 * Compile with: gcc -std=c99 -O2 -I include tools/osfx_cli_main.c build/libosfx_core.a -o osfx_cli
 * Usage:        osfx_cli <command> [args...]
 *               e.g.  osfx_cli plugin-list
 *                     osfx_cli plugin-load transport
 *                     osfx_cli transport-status
 *                     osfx_cli test-plugin run component
 */

#include "../include/osfx_core.h"
#include "../include/osfx_build_config.h"

#include <stdio.h>
#include <string.h>

static int matrix_emit_cb(const char* protocol, const uint8_t* frame, size_t frame_len, void* user_ctx) {
    (void)user_ctx;
    (void)frame;
    (void)protocol;
    (void)frame_len;
    return 1;
}

static int pf_emit_cb(const char* to_proto, uint16_t to_port, const uint8_t* payload, size_t payload_len, void* user_ctx) {
    (void)user_ctx;
    (void)to_proto;
    (void)to_port;
    (void)payload;
    (void)payload_len;
    return 1;
}

int main(int argc, char* argv[]) {
    osfx_protocol_matrix matrix;
    osfx_platform_runtime platform;
    char out[1024];
    int ok;

    osfx_protocol_matrix_init(&matrix, matrix_emit_cb, NULL);
    (void)osfx_protocol_matrix_register_defaults(&matrix);

    osfx_platform_runtime_init(&platform, &matrix, pf_emit_cb, NULL);

#if OSFX_CFG_AUTOLOAD_TRANSPORT
    (void)osfx_platform_plugin_load(&platform, "transport", "");
#endif
#if OSFX_CFG_AUTOLOAD_TEST_PLUGIN
    (void)osfx_platform_plugin_load(&platform, "test_plugin", "");
#endif
#if OSFX_CFG_AUTOLOAD_PORT_FORWARDER
    (void)osfx_platform_plugin_load(&platform, "port_forwarder", "");
#endif

    if (argc < 2) {
        printf("usage: osfx_cli <command> [args...]\n");
        return 1;
    }

    out[0] = '\0';
    ok = osfx_cli_lite_run(&platform, argc - 1, (const char**)&argv[1], out, sizeof(out));
    printf("%s\n", out);
    return ok ? 0 : 1;
}
