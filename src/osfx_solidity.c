#include "../include/osfx_solidity.h"

static const char OSFX_B62[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

int osfx_b62_encode_i64(long long value, char* out, size_t out_len) {
    unsigned long long n;
    char buf[96];
    size_t idx = 0;
    size_t w = 0;

    if (!out || out_len < 2) {
        return 0;
    }
    if (value == 0) {
        out[0] = '0';
        out[1] = '\0';
        return 1;
    }

    n = (value < 0) ? (unsigned long long)(-value) : (unsigned long long)value;
    while (n > 0 && idx < sizeof(buf) - 1) {
        buf[idx++] = OSFX_B62[n % 62ULL];
        n /= 62ULL;
    }
    if (idx == 0) {
        return 0;
    }
    if (idx + ((value < 0) ? 1 : 0) + 1 > out_len) {
        return 0;
    }
    if (value < 0) {
        out[w++] = '-';
    }
    while (idx > 0) {
        out[w++] = buf[--idx];
    }
    out[w] = '\0';
    return 1;
}

long long osfx_b62_decode_i64(const char* s, int* ok) {
    unsigned long long val = 0;
    const char* p = s;
    int neg = 0;

    if (ok) {
        *ok = 0;
    }
    if (!s || !*s) {
        return 0;
    }
    if (*p == '-') {
        neg = 1;
        p++;
        if (!*p) {
            return 0;
        }
    }
    while (*p) {
        unsigned char c = (unsigned char)*p;
        int d = -1;
        if (c >= '0' && c <= '9') {
            d = (int)(c - '0');
        } else if (c >= 'a' && c <= 'z') {
            d = 10 + (int)(c - 'a');
        } else if (c >= 'A' && c <= 'Z') {
            d = 36 + (int)(c - 'A');
        } else {
            return 0;
        }
        val = val * 62ULL + (unsigned long long)d;
        p++;
    }
    if (ok) {
        *ok = 1;
    }
    return neg ? -(long long)val : (long long)val;
}

uint8_t osfx_crc8(const uint8_t* data, size_t len, uint16_t poly, uint8_t init) {
    uint8_t crc = init;
    size_t i;
    int j;
    if (!data || len == 0) {
        return crc;
    }
    for (i = 0; i < len; ++i) {
        crc ^= data[i];
        for (j = 0; j < 8; ++j) {
            if (crc & 0x80U) {
                crc = (uint8_t)(((crc << 1) ^ (uint8_t)poly) & 0xFFU);
            } else {
                crc = (uint8_t)((crc << 1) & 0xFFU);
            }
        }
    }
    return crc;
}

uint16_t osfx_crc16_ccitt(const uint8_t* data, size_t len, uint16_t poly, uint16_t init) {
    uint16_t crc = init;
    size_t i;
    int j;
    if (!data || len == 0) {
        return crc;
    }
    for (i = 0; i < len; ++i) {
        crc ^= (uint16_t)(data[i] << 8);
        for (j = 0; j < 8; ++j) {
            if (crc & 0x8000U) {
                crc = (uint16_t)(((crc << 1) ^ poly) & 0xFFFFU);
            } else {
                crc = (uint16_t)((crc << 1) & 0xFFFFU);
            }
        }
    }
    return crc;
}

int osfx_u32toa(uint32_t n, char* buf, size_t cap) {
    char tmp[11];
    size_t idx = 0;
    size_t i;
    if (!buf || cap < 2) { return 0; }
    if (n == 0) { buf[0] = '0'; buf[1] = '\0'; return 1; }
    while (n > 0 && idx < sizeof(tmp)) {
        tmp[idx++] = (char)('0' + (int)(n % 10));
        n /= 10;
    }
    if (idx >= cap) { return 0; }
    for (i = 0; i < idx; ++i) {
        buf[i] = tmp[idx - 1 - i];
    }
    buf[idx] = '\0';
    return (int)idx;
}

static const char OSFX_B64URL[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

void osfx_b64url_ts6(uint64_t timestamp_raw, char out[9]) {
    uint8_t b[6];
    uint32_t g1;
    uint32_t g2;
    b[0] = (uint8_t)((timestamp_raw >> 40) & 0xFFU);
    b[1] = (uint8_t)((timestamp_raw >> 32) & 0xFFU);
    b[2] = (uint8_t)((timestamp_raw >> 24) & 0xFFU);
    b[3] = (uint8_t)((timestamp_raw >> 16) & 0xFFU);
    b[4] = (uint8_t)((timestamp_raw >>  8) & 0xFFU);
    b[5] = (uint8_t)( timestamp_raw        & 0xFFU);
    /* Group 1: bytes 0,1,2 */
    g1 = ((uint32_t)b[0] << 16) | ((uint32_t)b[1] << 8) | b[2];
    out[0] = OSFX_B64URL[(g1 >> 18) & 0x3Fu];
    out[1] = OSFX_B64URL[(g1 >> 12) & 0x3Fu];
    out[2] = OSFX_B64URL[(g1 >>  6) & 0x3Fu];
    out[3] = OSFX_B64URL[ g1        & 0x3Fu];
    /* Group 2: bytes 3,4,5 */
    g2 = ((uint32_t)b[3] << 16) | ((uint32_t)b[4] << 8) | b[5];
    out[4] = OSFX_B64URL[(g2 >> 18) & 0x3Fu];
    out[5] = OSFX_B64URL[(g2 >> 12) & 0x3Fu];
    out[6] = OSFX_B64URL[(g2 >>  6) & 0x3Fu];
    out[7] = OSFX_B64URL[ g2        & 0x3Fu];
    out[8] = '\0';
}

