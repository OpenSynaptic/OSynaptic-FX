#include "../include/osfx_id_allocator.h"
#include "../include/osfx_build_config.h"
#include "../include/osfx_storage.h"

#include <stdio.h>
#include <string.h>

static uint32_t count_in_use(const osfx_id_allocator* alloc) {
    size_t i;
    uint32_t used = 0U;
    if (!alloc) {
        return 0U;
    }
    for (i = 0; i < OSFX_ID_MAX_ENTRIES; ++i) {
        if (alloc->entries[i].in_use) {
            used++;
        }
    }
    return used;
}

static uint32_t pool_capacity(const osfx_id_allocator* alloc) {
    if (!alloc || alloc->end_id < alloc->start_id) {
        return 0U;
    }
    return (uint32_t)(alloc->end_id - alloc->start_id + 1U);
}

static uint64_t compute_lease_seconds(osfx_id_allocator* alloc, uint64_t now_ts) {
    double rate_per_hour;
    double pressure_ratio;
    double pressure_scale = 1.0;
    uint64_t elapsed;
    uint32_t used;
    uint32_t cap;
    double lease;
    if (!alloc) {
        return 0U;
    }
    if (!alloc->adaptive_enabled) {
        return alloc->default_lease_seconds;
    }
    if (alloc->recent_window_start == 0U) {
        alloc->recent_window_start = now_ts;
    }
    if (now_ts > alloc->recent_window_start + alloc->rate_window_seconds) {
        alloc->recent_window_start = now_ts;
        alloc->recent_alloc_count = 0U;
    }
    elapsed = (now_ts > alloc->recent_window_start) ? (now_ts - alloc->recent_window_start) : 1U;
    rate_per_hour = ((double)alloc->recent_alloc_count * 3600.0) / (double)elapsed;

    used = count_in_use(alloc);
    cap = pool_capacity(alloc);
    pressure_ratio = (cap > 0U) ? ((double)used / (double)cap) : 0.0;

    lease = (double)alloc->default_lease_seconds;
    if (rate_per_hour >= alloc->high_rate_threshold_per_hour) {
        lease = lease * alloc->high_rate_min_factor;
    }

    if (pressure_ratio >= alloc->pressure_high_watermark) {
        pressure_scale = alloc->pressure_min_factor;
    }
    lease = lease * pressure_scale;

    if (lease < (double)alloc->min_lease_seconds) {
        lease = (double)alloc->min_lease_seconds;
    }
    if (lease > (double)alloc->max_lease_seconds) {
        lease = (double)alloc->max_lease_seconds;
    }
    return (uint64_t)lease;
}

static osfx_id_lease_entry* find_entry(osfx_id_allocator* alloc, uint32_t aid) {
    size_t i;
    if (!alloc) {
        return NULL;
    }
    for (i = 0; i < OSFX_ID_MAX_ENTRIES; ++i) {
        if (alloc->entries[i].in_use && alloc->entries[i].aid == aid) {
            return &alloc->entries[i];
        }
    }
    return NULL;
}

static osfx_id_lease_entry* alloc_slot(osfx_id_allocator* alloc) {
    size_t i;
    if (!alloc) {
        return NULL;
    }
    for (i = 0; i < OSFX_ID_MAX_ENTRIES; ++i) {
        if (!alloc->entries[i].in_use) {
            return &alloc->entries[i];
        }
    }
    return NULL;
}

void osfx_id_allocator_init(osfx_id_allocator* alloc, uint32_t start_id, uint32_t end_id, uint64_t default_lease_seconds) {
    if (!alloc) {
        return;
    }
    memset(alloc, 0, sizeof(*alloc));
    alloc->start_id = start_id;
    alloc->end_id = end_id;
    alloc->default_lease_seconds = default_lease_seconds > 0U ? default_lease_seconds : 86400U;
    alloc->min_lease_seconds = 60U;
    alloc->max_lease_seconds = alloc->default_lease_seconds;
    alloc->rate_window_seconds = 3600U;
    alloc->high_rate_threshold_per_hour = 60.0;
    alloc->high_rate_min_factor = 0.2;
    alloc->pressure_high_watermark = 0.8;
    alloc->pressure_min_factor = 0.25;
    alloc->touch_extend_factor = 1.0;
    alloc->adaptive_enabled = 1;
    alloc->recent_window_start = 0U;
    alloc->recent_alloc_count = 0U;
}

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
) {
    if (!alloc) {
        return;
    }
    alloc->min_lease_seconds = (min_lease_seconds > 0U) ? min_lease_seconds : 1U;
    alloc->max_lease_seconds = (max_lease_seconds >= alloc->min_lease_seconds) ? max_lease_seconds : alloc->min_lease_seconds;
    alloc->rate_window_seconds = (rate_window_seconds > 0U) ? rate_window_seconds : 1U;
    alloc->high_rate_threshold_per_hour = (high_rate_threshold_per_hour > 0.0) ? high_rate_threshold_per_hour : 1.0;
    alloc->high_rate_min_factor = (high_rate_min_factor > 0.0 && high_rate_min_factor <= 1.0) ? high_rate_min_factor : 0.2;
    alloc->pressure_high_watermark = (pressure_high_watermark > 0.0 && pressure_high_watermark <= 1.0) ? pressure_high_watermark : 0.8;
    alloc->pressure_min_factor = (pressure_min_factor > 0.0 && pressure_min_factor <= 1.0) ? pressure_min_factor : 0.25;
    alloc->touch_extend_factor = (touch_extend_factor > 0.0) ? touch_extend_factor : 1.0;
    alloc->adaptive_enabled = adaptive_enabled ? 1 : 0;
}

void osfx_id_set_policy(
    osfx_id_allocator* alloc,
    uint64_t min_lease_seconds,
    uint64_t rate_window_seconds,
    double high_rate_threshold_per_hour,
    double high_rate_min_factor,
    int adaptive_enabled
) {
    uint64_t max_lease = alloc ? alloc->max_lease_seconds : min_lease_seconds;
    if (max_lease == 0U) {
        max_lease = (alloc && alloc->default_lease_seconds > 0U) ? alloc->default_lease_seconds : 86400U;
    }
    osfx_id_set_policy_ex(
        alloc,
        min_lease_seconds,
        max_lease,
        rate_window_seconds,
        high_rate_threshold_per_hour,
        high_rate_min_factor,
        alloc ? alloc->pressure_high_watermark : 0.8,
        alloc ? alloc->pressure_min_factor : 0.25,
        alloc ? alloc->touch_extend_factor : 1.0,
        adaptive_enabled
    );
}

void osfx_id_allocator_cleanup_expired(osfx_id_allocator* alloc, uint64_t now_ts) {
    size_t i;
    if (!alloc) {
        return;
    }
    for (i = 0; i < OSFX_ID_MAX_ENTRIES; ++i) {
        if (alloc->entries[i].in_use && now_ts > alloc->entries[i].leased_until) {
            memset(&alloc->entries[i], 0, sizeof(alloc->entries[i]));
        }
    }
}

int osfx_id_allocate(osfx_id_allocator* alloc, uint64_t now_ts, uint32_t* out_aid) {
    uint32_t aid;
    uint64_t lease_seconds;
    if (!alloc || !out_aid || alloc->start_id > alloc->end_id) {
        return 0;
    }
    osfx_id_allocator_cleanup_expired(alloc, now_ts);
    lease_seconds = compute_lease_seconds(alloc, now_ts);

    for (aid = alloc->start_id; aid <= alloc->end_id; ++aid) {
        osfx_id_lease_entry* e = find_entry(alloc, aid);
        if (!e) {
            osfx_id_lease_entry* slot = alloc_slot(alloc);
            if (!slot) {
                return 0;
            }
            slot->in_use = 1;
            slot->aid = aid;
            slot->last_seen = now_ts;
            slot->leased_until = now_ts + lease_seconds;
            alloc->recent_alloc_count++;
            *out_aid = aid;
            return 1;
        }
        if (e->in_use && now_ts > e->leased_until) {
            e->last_seen = now_ts;
            e->leased_until = now_ts + lease_seconds;
            alloc->recent_alloc_count++;
            *out_aid = aid;
            return 1;
        }
    }
    return 0;
}

int osfx_id_release(osfx_id_allocator* alloc, uint32_t aid) {
    osfx_id_lease_entry* e = find_entry(alloc, aid);
    if (!e) {
        return 0;
    }
    memset(e, 0, sizeof(*e));
    return 1;
}

int osfx_id_touch(osfx_id_allocator* alloc, uint32_t aid, uint64_t now_ts) {
    osfx_id_lease_entry* e = find_entry(alloc, aid);
    uint64_t extend;
    if (!e) {
        return 0;
    }
    extend = (uint64_t)((double)alloc->default_lease_seconds * alloc->touch_extend_factor);
    if (extend < alloc->min_lease_seconds) {
        extend = alloc->min_lease_seconds;
    }
    if (extend > alloc->max_lease_seconds) {
        extend = alloc->max_lease_seconds;
    }
    e->last_seen = now_ts;
    e->leased_until = now_ts + extend;
    return 1;
}

int osfx_id_save(const osfx_id_allocator* alloc, const char* path) {
    osfx_storage_writer writer;
    size_t i;
    char line[192];
    if (!alloc || !path) {
        return 0;
    }
    if (!osfx_storage_open_writer(path, &writer) || !writer.write || !writer.close) {
        return 0;
    }

    if (snprintf(
            line,
            sizeof(line),
            "range,%u,%u,%llu\n",
            (unsigned)alloc->start_id,
            (unsigned)alloc->end_id,
            (unsigned long long)alloc->default_lease_seconds
        ) <= 0 || !writer.write(writer.handle, line)) {
        (void)writer.close(writer.handle);
        return 0;
    }

    if (snprintf(
            line,
            sizeof(line),
            "policy,%llu,%llu,%.6f,%.6f,%d\n",
            (unsigned long long)alloc->min_lease_seconds,
            (unsigned long long)alloc->rate_window_seconds,
            alloc->high_rate_threshold_per_hour,
            alloc->high_rate_min_factor,
            alloc->adaptive_enabled
        ) <= 0 || !writer.write(writer.handle, line)) {
        (void)writer.close(writer.handle);
        return 0;
    }

    if (snprintf(
            line,
            sizeof(line),
            "policy_ex,%llu,%llu,%.6f,%.6f,%.6f\n",
            (unsigned long long)alloc->max_lease_seconds,
            (unsigned long long)alloc->rate_window_seconds,
            alloc->pressure_high_watermark,
            alloc->pressure_min_factor,
            alloc->touch_extend_factor
        ) <= 0 || !writer.write(writer.handle, line)) {
        (void)writer.close(writer.handle);
        return 0;
    }

    for (i = 0; i < OSFX_ID_MAX_ENTRIES; ++i) {
        if (alloc->entries[i].in_use) {
            if (snprintf(
                    line,
                    sizeof(line),
                    "%u,%llu,%llu\n",
                    (unsigned)alloc->entries[i].aid,
                    (unsigned long long)alloc->entries[i].leased_until,
                    (unsigned long long)alloc->entries[i].last_seen
                ) <= 0 || !writer.write(writer.handle, line)) {
                (void)writer.close(writer.handle);
                return 0;
            }
        }
    }
    return writer.close(writer.handle);
}

int osfx_id_load(osfx_id_allocator* alloc, const char* path) {
    osfx_storage_reader reader;
    char line[256];
    int rc;
    if (!alloc || !path) {
        return 0;
    }
    if (!osfx_storage_open_reader(path, &reader) || !reader.read_line || !reader.close) {
        return 0;
    }

    memset(alloc->entries, 0, sizeof(alloc->entries));
    while ((rc = reader.read_line(reader.handle, line, sizeof(line))) > 0) {
        unsigned aid = 0;
        unsigned long long leased_until = 0;
        unsigned long long last_seen = 0;
        unsigned start_id = 0;
        unsigned end_id = 0;
        unsigned long long lease = 0;
        unsigned long long min_lease = 0;
        unsigned long long rate_window = 0;
        unsigned long long max_lease = 0;
        double threshold = 0.0;
        double factor = 0.0;
        double pressure_high = 0.0;
        double pressure_min = 0.0;
        double touch_factor = 0.0;
        int adaptive = 0;
        if (sscanf(line, "range,%u,%u,%llu", &start_id, &end_id, &lease) == 3) {
            alloc->start_id = (uint32_t)start_id;
            alloc->end_id = (uint32_t)end_id;
            alloc->default_lease_seconds = (uint64_t)lease;
            continue;
        }
        if (sscanf(line, "policy,%llu,%llu,%lf,%lf,%d", &min_lease, &rate_window, &threshold, &factor, &adaptive) == 5) {
            osfx_id_set_policy(alloc, (uint64_t)min_lease, (uint64_t)rate_window, threshold, factor, adaptive);
            continue;
        }
        if (sscanf(line, "policy_ex,%llu,%llu,%lf,%lf,%lf", &max_lease, &rate_window, &pressure_high, &pressure_min, &touch_factor) == 5) {
            osfx_id_set_policy_ex(
                alloc,
                alloc->min_lease_seconds,
                (uint64_t)max_lease,
                (uint64_t)rate_window,
                alloc->high_rate_threshold_per_hour,
                alloc->high_rate_min_factor,
                pressure_high,
                pressure_min,
                touch_factor,
                alloc->adaptive_enabled
            );
            continue;
        }
        if (sscanf(line, "%u,%llu,%llu", &aid, &leased_until, &last_seen) == 3) {
            osfx_id_lease_entry* slot = alloc_slot(alloc);
            if (slot) {
                slot->in_use = 1;
                slot->aid = (uint32_t)aid;
                slot->leased_until = (uint64_t)leased_until;
                slot->last_seen = (uint64_t)last_seen;
            }
        }
    }
    if (rc < 0) {
        (void)reader.close(reader.handle);
        return 0;
    }
    return reader.close(reader.handle);
}

