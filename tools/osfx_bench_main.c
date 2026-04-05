/*
 * osfx_bench_main.c
 * -----------------
 * Benchmark runner for OSynaptic-FX core operations.
 *
 * Usage:  osfx_bench_main <output_dir> <memory_limit_kb>
 *   output_dir       — directory to write bench_report.md / bench_report.csv
 *   memory_limit_kb  — peak-RSS limit in KB (0 = skip check); non-zero exit if exceeded
 *
 * Compile:
 *   gcc  -std=c99 -O3 -I include tools/osfx_bench_main.c build/libosfx_core.a -lpsapi -o build/osfx_bench_main.exe
 *   cl   /nologo /TC /O2 /I include tools\osfx_bench_main.c build\osfx_core.lib Psapi.lib /Fe:build\osfx_bench_main.exe
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../include/osfx_core.h"
#include "../include/osfx_build_config.h"

/* ── Platform-specific peak RSS ─────────────────────────────────────────── */
#if defined(_WIN32) || defined(_WIN64)
#  ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#  include <psapi.h>
static size_t peak_rss_kb(void) {
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return (size_t)(pmc.PeakWorkingSetSize / 1024UL);
    }
    return 0;
}
#else
static size_t peak_rss_kb(void) { return 0; }
#endif

/* ── Benchmark definitions ──────────────────────────────────────────────── */
#define BENCH_ITERS 100000

static double run_b62_encode(void) {
    char buf[32];
    int i;
    clock_t t0 = clock(), t1;
    for (i = 0; i < BENCH_ITERS; i++) {
        osfx_b62_encode_i64((long long)i + 123456789LL, buf, sizeof(buf));
    }
    t1 = clock();
    return (double)BENCH_ITERS / ((double)(t1 - t0) / (double)CLOCKS_PER_SEC);
}

static double run_crc8(void) {
    static const uint8_t data[64];
    int i;
    clock_t t0 = clock(), t1;
    for (i = 0; i < BENCH_ITERS; i++) {
        volatile uint8_t r = osfx_crc8(data, sizeof(data), 0x07u, 0x00u);
        (void)r;
    }
    t1 = clock();
    return (double)BENCH_ITERS / ((double)(t1 - t0) / (double)CLOCKS_PER_SEC);
}

static double run_crc16(void) {
    static const uint8_t data[64];
    int i;
    clock_t t0 = clock(), t1;
    for (i = 0; i < BENCH_ITERS; i++) {
        volatile uint16_t r = osfx_crc16_ccitt(data, sizeof(data), 0x1021u, 0xFFFFu);
        (void)r;
    }
    t1 = clock();
    return (double)BENCH_ITERS / ((double)(t1 - t0) / (double)CLOCKS_PER_SEC);
}

static double run_packet_encode(void) {
    static const uint8_t body[32];
    uint8_t out[256];
    int i;
    clock_t t0 = clock(), t1;
    for (i = 0; i < BENCH_ITERS; i++) {
        osfx_packet_encode_full(0x3Fu, 0x00000001u, (uint8_t)(i & 0xFF),
                                (uint64_t)(i * 1000ULL),
                                body, sizeof(body),
                                out, sizeof(out));
    }
    t1 = clock();
    return (double)BENCH_ITERS / ((double)(t1 - t0) / (double)CLOCKS_PER_SEC);
}

static double run_sensor_encode(void) {
    uint8_t pkt[256];
    int pkt_len;
    int i;
    clock_t t0 = clock(), t1;
    for (i = 0; i < BENCH_ITERS; i++) {
        pkt_len = 0;
        osfx_core_encode_sensor_packet(
            0x00000001u, (uint8_t)(i & 0xFF), (uint64_t)(i * 1000ULL),
            "TEMP", 25.0 + (double)(i % 10), "degC",
            pkt, sizeof(pkt), &pkt_len);
    }
    t1 = clock();
    return (double)BENCH_ITERS / ((double)(t1 - t0) / (double)CLOCKS_PER_SEC);
}

/* ── Entry point ─────────────────────────────────────────────────────────── */
int main(int argc, char* argv[]) {
    const char* out_dir;
    int         mem_limit_kb;
    char        md_path[512];
    char        csv_path[512];
    size_t      peak_kb;
    int         mem_ok;
    FILE*       fmd;
    FILE*       fcsv;
    size_t      i;

    /* name, ops/sec */
    struct { const char* name; double ops; } results[5];

    out_dir      = (argc >= 2) ? argv[1] : ".";
    mem_limit_kb = (argc >= 3) ? atoi(argv[2]) : 0;

    /* ── Run all benchmarks ── */
    printf("[bench] running %d iterations each...\n", BENCH_ITERS);

    results[0].name = "b62_encode";      results[0].ops = run_b62_encode();
    results[1].name = "crc8";            results[1].ops = run_crc8();
    results[2].name = "crc16_ccitt";     results[2].ops = run_crc16();
    results[3].name = "packet_encode";   results[3].ops = run_packet_encode();
    results[4].name = "sensor_encode";   results[4].ops = run_sensor_encode();

    peak_kb = peak_rss_kb();
    mem_ok  = (mem_limit_kb <= 0) || (peak_kb == 0) ||
              (peak_kb <= (size_t)mem_limit_kb);

    /* ── Print summary ── */
    printf("=== Benchmark Results ===\n");
    for (i = 0; i < 5u; i++) {
        printf("  %-22s  %12.0f ops/sec\n", results[i].name, results[i].ops);
    }
    if (peak_kb > 0) {
        printf("  Peak RSS: %zu KB  (limit %d KB)  %s\n",
               peak_kb, mem_limit_kb, mem_ok ? "PASS" : "FAIL");
    }

    /* ── Write Markdown report ── */
    snprintf(md_path,  sizeof(md_path),  "%s/bench_report.md",  out_dir);
    fmd = fopen(md_path, "w");
    if (!fmd) { fprintf(stderr, "[bench] cannot open %s\n", md_path); return 1; }

    fprintf(fmd, "# OSynaptic-FX Benchmark Report\n\n");
    fprintf(fmd, "Iterations per operation: %d\n\n", BENCH_ITERS);
    fprintf(fmd, "| Operation | ops/sec |\n");
    fprintf(fmd, "|---|---|\n");
    for (i = 0; i < 5u; i++) {
        fprintf(fmd, "| `%s` | %.0f |\n", results[i].name, results[i].ops);
    }
    if (peak_kb > 0) {
        fprintf(fmd, "\n**Peak RSS**: %zu KB &mdash; limit %d KB &mdash; **%s**\n",
                peak_kb, mem_limit_kb, mem_ok ? "PASS" : "FAIL");
    }
    fclose(fmd);

    /* ── Write CSV report ── */
    snprintf(csv_path, sizeof(csv_path), "%s/bench_report.csv", out_dir);
    fcsv = fopen(csv_path, "w");
    if (!fcsv) { fprintf(stderr, "[bench] cannot open %s\n", csv_path); return 1; }

    fprintf(fcsv, "operation,ops_per_sec\n");
    for (i = 0; i < 5u; i++) {
        fprintf(fcsv, "%s,%.0f\n", results[i].name, results[i].ops);
    }
    if (peak_kb > 0) {
        fprintf(fcsv, "peak_rss_kb,%zu\n", peak_kb);
        fprintf(fcsv, "mem_limit_kb,%d\n", mem_limit_kb);
        fprintf(fcsv, "mem_check,%s\n", mem_ok ? "PASS" : "FAIL");
    }
    fclose(fcsv);

    printf("[bench] reports: %s  %s\n", md_path, csv_path);
    return mem_ok ? 0 : 1;
}
