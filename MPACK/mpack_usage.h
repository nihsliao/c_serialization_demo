/* MPACK/mpack_usage.h
 *
 * Requires: mpack.h (MPack library)
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
 */

#ifndef MPACK_USAGE_H
#define MPACK_USAGE_H

#include "../sample_structure.h" /* defines wifi_softap_info_t, constants */
#include "mpack/mpack.h"

/* Helper function to write a single wifi_softap_info_t structure */
int write_single_structure(mpack_writer_t* writer, const wifi_softap_info_t* info) {
    if (!writer || !info) return -1;

    /* write an array of 9 elements (fixed-order schema) */
    mpack_start_array(writer, 9);

    mpack_write_i32(writer, (int32_t)info->device_count);
    mpack_write_i32(writer, (int32_t)info->state);
    mpack_write_bin(writer, (const char*)info->ip_address.ipv4, sizeof(info->ip_address.ipv4));
    mpack_write_bin(writer, (const char*)info->ip_address.ipv6, sizeof(info->ip_address.ipv6));

    /* ssid: variable length binary/string (we store raw bytes length = strlen) */
    size_t sslen = strnlen(info->ssid, WIFI_SSID_MAX_LEN);
    if (sslen > 0) {
        mpack_write_bin(writer, info->ssid, (uint32_t)sslen);
    } else {
        /* write empty bin */
        mpack_write_bin(writer, "", 0);
    }

    mpack_write_bin(writer, (const char*)info->bssid, WIFI_BT_MAC_ADDRESS_LEN);
    mpack_write_i32(writer, (int32_t)info->security);
    mpack_write_u8(writer, info->channel);
    mpack_write_u16(writer, info->frequency);

    mpack_finish_array(writer);
    return 0;
}

/* Helper function to read a single wifi_softap_info_t structure */
int read_single_structure(mpack_reader_t* reader, wifi_softap_info_t* info) {
    if (!reader || !info) return -1;

    /* read an array of 9 elements (fixed-order schema) */
    mpack_expect_array_match(reader, 9);
    memset(info, 0, sizeof(*info));

    info->device_count = mpack_expect_i32(reader);
    info->state = mpack_expect_i32(reader);

    char buf[WIFI_SSID_MAX_LEN];
    size_t binlen = mpack_expect_bin_buf(reader, buf, sizeof(buf));
    if (binlen != sizeof(info->ip_address.ipv4)) {
        return -1;
    }
    memcpy(info->ip_address.ipv4, buf, binlen);

    binlen = mpack_expect_bin_buf(reader, buf, sizeof(buf));
    if (binlen != sizeof(info->ip_address.ipv6)) {
        return -1;
    }
    memcpy(info->ip_address.ipv6, buf, binlen);

    binlen = mpack_expect_bin_buf(reader, buf, sizeof(buf));
    if (binlen > WIFI_SSID_MAX_LEN) {
        return -1;
    }
    if (binlen > 0) memcpy(info->ssid, buf, binlen);
    info->ssid[binlen] = '\0';

    binlen = mpack_expect_bin_buf(reader, buf, sizeof(buf));
    if (binlen != WIFI_BT_MAC_ADDRESS_LEN) {
        return -1;
    }
    memcpy(info->bssid, buf, binlen);

    info->security = mpack_expect_i32(reader);
    info->channel = mpack_expect_u8(reader);
    info->frequency = mpack_expect_u16(reader);

    mpack_done_array(reader);
    return 0;
}

/* ---------- mpack encode / decode ---------- */
/*
 * mpack_encode
 *  - input: wifi_softap_info_t *info
 *  - output: **out_buffer, *out_size
 *  - return: 0 on success, -1 on failure
 */
int mpack_encode(wifi_softap_info_t* info, void** out_buffer, size_t* out_size) {
    if (!info || !out_buffer || !out_size) return -1;

    char* data;
    size_t size;
    mpack_writer_t writer;
    /* init growable writer: writer will allocate 'data' and set 'size' */
    mpack_writer_init_growable(&writer, &data, &size);

    if (write_single_structure(&writer, info) != 0) {
        mpack_writer_flag_error(&writer, mpack_error_data);
    }

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
int mpack_decode(void* buffer, size_t size, wifi_softap_info_t* out_info) {
    if (!buffer || size == 0 || !out_info) return -1;

    mpack_reader_t reader;
    mpack_reader_init_data(&reader, (const char*)buffer, size);

    if (read_single_structure(&reader, out_info) != 0) {
        mpack_reader_flag_error(&reader, mpack_error_data);
    }

    mpack_error_t err = mpack_reader_destroy(&reader);
    if (err != mpack_ok) {
        fprintf(stderr, "mpack: reader error %d\n", err);
        return -1;
    }
    return 0;
}

int mpack_encode_array(const wifi_softap_info_t* infos, int count, void** out_buffer, size_t* out_size) {
    if (!infos || count == 0 || !out_buffer || !out_size) return -1;

    char* data;
    size_t size;
    mpack_writer_t writer;
    mpack_writer_init_growable(&writer, &data, &size);

    mpack_start_array(&writer, (uint32_t)count);
    for (int i = 0; i < count; i++) {
        if (write_single_structure(&writer, &infos[i]) != 0) {
            mpack_writer_flag_error(&writer, mpack_error_data);
            break;
        }
    }
    mpack_finish_array(&writer);

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

int mpack_decode_array(void* buffer, size_t size, wifi_softap_info_t** out_infos, int* out_count) {
    if (!buffer || size == 0 || !out_infos || !out_count) return -1;

    mpack_reader_t reader;
    mpack_reader_init_data(&reader, (const char*)buffer, size);

    int count = mpack_expect_array(&reader);
    if (mpack_reader_error(&reader) != mpack_ok) {
        mpack_reader_destroy(&reader);
        return -1;
    }

    wifi_softap_info_t* infos = (wifi_softap_info_t*)malloc(sizeof(wifi_softap_info_t) * count);
    if (!infos) {
        mpack_reader_destroy(&reader);
        return -1;
    }

    for (int i = 0; i < count; i++) {
        if (read_single_structure(&reader, &infos[i]) != 0) {
            mpack_reader_flag_error(&reader, mpack_error_data);
            break;
        }
    }
    mpack_done_array(&reader);

    mpack_error_t err = mpack_reader_destroy(&reader);
    if (err != mpack_ok) {
        free(infos);
        return -1;
    }

    *out_infos = infos;
    *out_count = count;
    return 0;
}

#endif /* MPACK_USAGE_H */