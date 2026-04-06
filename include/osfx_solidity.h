#ifndef OSFX_SOLIDITY_H
#define OSFX_SOLIDITY_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int osfx_b62_encode_i64(long long value, char* out, size_t out_len);
long long osfx_b62_decode_i64(const char* s, int* ok);

uint8_t osfx_crc8(const uint8_t* data, size_t len, uint16_t poly, uint8_t init);
uint16_t osfx_crc16_ccitt(const uint8_t* data, size_t len, uint16_t poly, uint16_t init);

/*
 * Convert a uint32 to a decimal ASCII string.
 * Returns the number of characters written (excluding NUL), or 0 on error.
 */
int osfx_u32toa(uint32_t n, char* buf, size_t cap);

/*
 * Encode a 48-bit timestamp (lower 6 bytes of timestamp_raw) to the
 * 8-character base64url string used in the OpenSynaptic wire body header.
 * Fields are encoded big-endian.  out must hold at least 9 bytes.
 */
void osfx_b64url_ts6(uint64_t timestamp_raw, char out[9]);

#ifdef __cplusplus
}
#endif

#endif

