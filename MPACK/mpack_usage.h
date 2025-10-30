/* MPACK/mpack_usage.h
 *
 * Requires: mpack.h (MPack library)
 * Compile: add -I/path/to/mpack/include and link against mpack (if you build it as a lib)
 *
 * Exports:
 *   int mpack_encode(wifi_softap_info_t *info, void **out_buffer, size_t *out_size);
 *   int mpack_decode(void *buffer, size_t size, wifi_softap_info_t *out_info);
 *
 * Notes:
 * - This implementation uses MPack growable buffer writer (mpack_writer_init_growable)
 *   and MPack reader (mpack_reader_init_data).
 * - Schema: array of 9 elements in this exact order:
 *     [ device_count (int32),
 *       state (int32),
 *       ipv4 (bin 4),
 *       ipv6 (bin 16),
 *       ssid (bin N),
 *       bssid (bin 6),
 *       security (int32),
 *       channel (uint8),
 *       frequency (int32) ]
 *
 * If you get compile errors about missing mpack_* symbols, replace the names with
 * the versions available in your local mpack (common variants: mpack_write_u32 / mpack_write_uint).
 */

#ifndef MPACK_USAGE_H
#define MPACK_USAGE_H

#include "../sample_structure.h" /* defines wifi_softap_info_t, constants */
/* include the MPack header; adjust include path if necessary */
#include "mpack/mpack.h"

/* ---------- mpack encode / decode ---------- */
/*
 * mpack_encode
 *  - input: wifi_softap_info_t *info
 *  - output: *out_buffer (allocated via malloc inside and must be freed by caller), *out_size
 *  - return: 0 on success, -1 on failure
 */
int mpack_encode(wifi_softap_info_t *info, void **out_buffer, size_t *out_size) {
    if (!info || !out_buffer || !out_size) return -1;

    char *data;
    size_t size;
    mpack_writer_t writer;
    /* init growable writer: writer will allocate 'data' and set 'size' */
    mpack_writer_init_growable(&writer, &data, &size);

    /* write an array of 9 elements (fixed-order schema) */
    mpack_start_array(&writer, 9);

    /* device_count (int32) */
    mpack_write_i32(&writer, (int32_t)info->device_count);

    /* state (int32) */
    mpack_write_i32(&writer, (int32_t)info->state);

    /* ipv4: write as bin (4 bytes) */
    mpack_write_bin(&writer, (const char*)info->ip_address.ipv4, sizeof(info->ip_address.ipv4));

    /* ipv6: 16 bytes */
    mpack_write_bin(&writer, (const char*)info->ip_address.ipv6, sizeof(info->ip_address.ipv6));

    /* ssid: variable length binary/string (we store raw bytes length = strlen) */
    size_t sslen = strnlen(info->ssid, WIFI_SSID_MAX_LEN);
    if (sslen > 0) {
        mpack_write_bin(&writer, info->ssid, (uint32_t)sslen);
    } else {
        /* write empty bin */
        mpack_write_bin(&writer, "", 0);
    }

    /* bssid: 6 bytes */
    mpack_write_bin(&writer, (const char*)info->bssid, WIFI_BT_MAC_ADDRESS_LEN);

    /* security (int32) */
    mpack_write_i32(&writer, (int32_t)info->security);

    /* channel (single byte) -> write as uint */
    mpack_write_u8(&writer, (uint8_t)info->channel);

    /* frequency (int32) */
    mpack_write_i32(&writer, (int32_t)info->frequency);

    mpack_finish_array(&writer);

    /* finalize writer and check error */
    mpack_error_t err = mpack_writer_destroy(&writer);
    if (err != mpack_ok) {
        fprintf(stderr, "mpack: writer error %d\n", err);
        if (data) free(data);
        return -1;
    }

    *out_buffer = data;
    *out_size = size;
    return 0;
}

/*
* mpack_decode
*  - input: *buffer, size
*  - output: wifi_softap_info_t *info
*  - return: 0 on success, -1 on failure
*/
int mpack_decode(void *buffer, size_t size, wifi_softap_info_t *out_info) {
    if (!buffer || size == 0 || !out_info) return -1;

    mpack_reader_t reader;
    mpack_reader_init_data(&reader, (const char*)buffer, size);

    /* expect array of 9 */
    uint32_t count = mpack_expect_array(&reader);
    if (mpack_reader_error(&reader) != mpack_ok) {
        mpack_reader_destroy(&reader);
        return -1;
    }
    if (count != 9) {
        /* wrong schema */
        mpack_reader_destroy(&reader);
        return -1;
    }

    memset(out_info, 0, sizeof(*out_info));

    /* device_count */
    out_info->device_count = (int32_t)mpack_expect_i32(&reader);

    /* state */
    out_info->state = (int32_t)mpack_expect_i32(&reader);

    /* ipv4 (bin 4) */
    size_t binlen = 0;
    const char *binptr = mpack_expect_bin_alloc(&reader, sizeof(out_info->ip_address.ipv4), &binlen);
    if (mpack_reader_error(&reader) != mpack_ok || binlen != sizeof(out_info->ip_address.ipv4)) {
        mpack_reader_destroy(&reader);
        return -1;
    }
    memcpy(out_info->ip_address.ipv4, binptr, binlen);

    /* ipv6 (bin 16) */
    binptr = mpack_expect_bin_alloc(&reader, sizeof(out_info->ip_address.ipv6), &binlen);
    if (mpack_reader_error(&reader) != mpack_ok || binlen != sizeof(out_info->ip_address.ipv6)) {
        mpack_reader_destroy(&reader);
        return -1;
    }
    memcpy(out_info->ip_address.ipv6, binptr, binlen);

    /* ssid (bin N) */
    binptr = mpack_expect_bin_alloc(&reader, sizeof(out_info->ssid), &binlen);
    if (mpack_reader_error(&reader) != mpack_ok || binlen > WIFI_SSID_MAX_LEN) {
        mpack_reader_destroy(&reader);
        return -1;
    }
    if (binlen > 0) memcpy(out_info->ssid, binptr, binlen);
    out_info->ssid[binlen] = '\0';

    /* bssid (bin 6) */
    binptr = mpack_expect_bin_alloc(&reader, sizeof(out_info->bssid), &binlen);
    if (mpack_reader_error(&reader) != mpack_ok || binlen != WIFI_BT_MAC_ADDRESS_LEN) {
        mpack_reader_destroy(&reader);
        return -1;
    }
    memcpy(out_info->bssid, binptr, binlen);

    /* security */
    out_info->security = (int32_t)mpack_expect_i32(&reader);

    /* channel (uint8) */
    out_info->channel = (uint8_t)mpack_expect_u8(&reader);

    /* frequency */
    out_info->frequency = (int32_t)mpack_expect_i32(&reader);

    /* ensure no leftover */
    mpack_done_array(&reader);
    if (mpack_reader_error(&reader) != mpack_ok) {
        mpack_reader_destroy(&reader);
        return -1;
    }
    mpack_reader_destroy(&reader);
    return 0;
}

#endif /* MPACK_USAGE_H */