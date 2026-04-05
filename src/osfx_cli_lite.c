#include "../include/osfx_cli_lite.h"
#include "../include/osfx_build_config.h"

#include <string.h>
#if OSFX_ENABLE_CLI
#include <stdio.h>
#endif

static int append_token(char* out, size_t out_cap, const char* token) {
    size_t used;
    size_t add;
    if (!out || out_cap == 0U || !token) {
        return 0;
    }
    used = strlen(out);
    add = strlen(token);
    if (used + add + 1U >= out_cap) {
        return 0;
    }
    memcpy(out + used, token, add + 1U);
    return 1;
}

static int join_args(int start, int argc, const char* argv[], char* out, size_t out_cap) {
    int i;
    if (!out || out_cap == 0U) {
        return 0;
    }
    out[0] = '\0';
    for (i = start; i < argc; ++i) {
        if (i > start) {
            if (!append_token(out, out_cap, " ")) {
                return 0;
            }
        }
        if (!append_token(out, out_cap, argv[i])) {
            return 0;
        }
    }
    return 1;
}

int osfx_cli_lite_run(osfx_platform_runtime* rt, int argc, const char* argv[], char* out, size_t out_cap) {
#if !OSFX_ENABLE_CLI
    (void)rt;
    (void)argc;
    (void)argv;
    if (!out || out_cap == 0U) {
        return 0;
    }
    {
        static const char msg[] = "error=cli_disabled";
        size_t len = sizeof(msg) - 1U;
        if (out_cap > len) {
            memcpy(out, msg, len + 1U);
        } else {
            out[0] = '\0';
        }
    }
    return 0;
#else
    if (!rt || !out || out_cap == 0U) {
        return 0;
    }
    out[0] = '\0';
    if (argc <= 0 || !argv || !argv[0]) {
        snprintf(out, out_cap, "error=empty_command");
        return 0;
    }

    if (strcmp(argv[0], "plugin-list") == 0) {
        size_t n = osfx_platform_plugin_count(rt);
        size_t i;
        size_t off = 0;
        int w = snprintf(out, out_cap, "plugins=");
        if (w < 0 || (size_t)w >= out_cap) {
            return 0;
        }
        off = (size_t)w;
        for (i = 0; i < n; ++i) {
            char name[32];
            if (!osfx_platform_plugin_name_at(rt, i, name, sizeof(name))) {
                continue;
            }
            w = snprintf(out + off, out_cap - off, "%s%s", (i ? "," : ""), name);
            if (w < 0 || (size_t)w >= (out_cap - off)) {
                return 0;
            }
            off += (size_t)w;
        }
        return 1;
    }

    if (strcmp(argv[0], "plugin-load") == 0) {
        const char* cfg = "";
        if (argc < 2) {
            snprintf(out, out_cap, "error=usage plugin-load <name> [config]");
            return 0;
        }
        if (argc >= 3) {
            cfg = argv[2];
        }
        if (!osfx_platform_plugin_load(rt, argv[1], cfg)) {
            snprintf(out, out_cap, "error=load_failed name=%s", argv[1]);
            return 0;
        }
        snprintf(out, out_cap, "ok=1 loaded=%s", argv[1]);
        return 1;
    }

    if (strcmp(argv[0], "plugin-cmd") == 0) {
        char args[512];
        if (argc < 3) {
            snprintf(out, out_cap, "error=usage plugin-cmd <name> <cmd> [args...]");
            return 0;
        }
        if (!join_args(3, argc, argv, args, sizeof(args))) {
            snprintf(out, out_cap, "error=args_too_long");
            return 0;
        }
        return osfx_platform_plugin_cmd(rt, argv[1], argv[2], args, out, out_cap);
    }

    if (strcmp(argv[0], "transport-status") == 0) {
        return osfx_platform_plugin_cmd(rt, "transport", "status", "", out, out_cap);
    }

    if (strcmp(argv[0], "test-plugin") == 0) {
        const char* cmd = (argc >= 2) ? argv[1] : "status";
        char args[512];
        if (argc >= 3) {
            if (!join_args(2, argc, argv, args, sizeof(args))) {
                return 0;
            }
            return osfx_platform_plugin_cmd(rt, "test_plugin", cmd, args, out, out_cap);
        }
        if (!join_args(2, argc, argv, args, sizeof(args))) {
            return 0;
        }
        return osfx_platform_plugin_cmd(rt, "test_plugin", cmd, "", out, out_cap);
    }

    if (strcmp(argv[0], "port-forwarder") == 0) {
        const char* cmd = (argc >= 2) ? argv[1] : "status";
        char args[512];
        if (argc >= 3) {
            if (!join_args(2, argc, argv, args, sizeof(args))) {
                return 0;
            }
            return osfx_platform_plugin_cmd(rt, "port_forwarder", cmd, args, out, out_cap);
        }
        if (!join_args(2, argc, argv, args, sizeof(args))) {
            return 0;
        }
        return osfx_platform_plugin_cmd(rt, "port_forwarder", cmd, "", out, out_cap);
    }

    snprintf(out, out_cap, "error=unknown_command");
    return 0;
#endif
}

