#ifndef OSFX_ID_ALLOCATOR_H
#define OSFX_ID_ALLOCATOR_H

#include <stddef.h>
#include <stdint.h>

#include "osfx_build_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ID lease table capacity.
 *
 * Override priority:
 *   1) User-defined OSFX_ID_MAX_ENTRIES (for example in osfx_user_config.h)
 *   2) Platform default below
 */
#ifndef OSFX_ID_MAX_ENTRIES
#if defined(CH32V003) || defined(CH32V003xx)
#define OSFX_ID_MAX_ENTRIES 16
#elif defined(STM32F103xB) || defined(STM32F103xE) || defined(STM32F1) || defined(STM32F1xx)
#define OSFX_ID_MAX_ENTRIES 64
#elif defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_MBED_RP2040) || defined(PICO_RP2040) || defined(STM32F4) || defined(STM32F4xx)
#define OSFX_ID_MAX_ENTRIES 256
#elif defined(ESP32) || defined(ARDUINO_ARCH_ESP32) || defined(__linux__) || defined(__ANDROID__)
#define OSFX_ID_MAX_ENTRIES 1024
#else
#define OSFX_ID_MAX_ENTRIES 256
#endif
#endif

typedef struct osfx_id_lease_entry {
    uint32_t aid;
    uint64_t leased_until;
    uint64_t last_seen;
    int in_use;
} osfx_id_lease_entry;

typedef struct osfx_id_allocator {
    uint32_t start_id;
    uint32_t end_id;
    uint64_t default_lease_seconds;
    uint64_t min_lease_seconds;
    uint64_t max_lease_seconds;
    uint64_t rate_window_seconds;
    double high_rate_threshold_per_hour;
    double high_rate_min_factor;
    double pressure_high_watermark;
    double pressure_min_factor;
    double touch_extend_factor;
    int adaptive_enabled;
    uint64_t recent_window_start;
    uint32_t recent_alloc_count;
    osfx_id_lease_entry entries[OSFX_ID_MAX_ENTRIES];
} osfx_id_allocator;

void osfx_id_allocator_init(osfx_id_allocator* alloc, uint32_t start_id, uint32_t end_id, uint64_t default_lease_seconds);
void osfx_id_set_policy(
    osfx_id_allocator* alloc,
    uint64_t min_lease_seconds,
    uint64_t rate_window_seconds,
    double high_rate_threshold_per_hour,
    double high_rate_min_factor,
    int adaptive_enabled
);
void osfx_id_set_policy_ex(
    osfx_id_allocator* alloc,
    uint64_t min_lease_seconds,
    uint64_t max_lease_seconds,
    uint64_t rate_window_seconds,
    double high_rate_threshold_per_hour,
    double high_rate_min_factor,
    double pressure_high_watermark,
    double pressure_min_factor,
    double touch_extend_factor,
    int adaptive_enabled
);
void osfx_id_allocator_cleanup_expired(osfx_id_allocator* alloc, uint64_t now_ts);
int osfx_id_allocate(osfx_id_allocator* alloc, uint64_t now_ts, uint32_t* out_aid);
int osfx_id_release(osfx_id_allocator* alloc, uint32_t aid);
int osfx_id_touch(osfx_id_allocator* alloc, uint32_t aid, uint64_t now_ts);
int osfx_id_save(const osfx_id_allocator* alloc, const char* path);
int osfx_id_load(osfx_id_allocator* alloc, const char* path);

#ifdef __cplusplus
}
#endif

#endif

