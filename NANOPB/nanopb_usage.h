#ifndef NANOPB_USAGE_H
#define NANOPB_USAGE_H

#include "../sample_structure.h" /* defines wifi_softap_info_t, constants */
#include "nanopb/pb_decode.h"
#include "nanopb/pb_encode.h"
#include "nanopb/sample_structure.pb.h"

int parse_wifi_softap_info(const wifi_softap_info_t* info, wifi_WifiSoftAPInfo* message) {
    if (!message || !info) return -1;

    message->device_count = info->device_count;
    message->state = info->state;
    message->has_ip_address = true;

    /* ip bytes: the generated types use .size and .bytes members */
    /* copy ipv4/ipv6 (exact sizes) */
    memcpy(message->ip_address.ipv4.bytes, info->ip_address.ipv4, sizeof(info->ip_address.ipv4));
    message->ip_address.ipv4.size = sizeof(info->ip_address.ipv4);

    memcpy(message->ip_address.ipv6.bytes, info->ip_address.ipv6, sizeof(info->ip_address.ipv6));
    message->ip_address.ipv6.size = sizeof(info->ip_address.ipv6);

    /* ssid: store length and bytes (ssid PB array length likely 33) */
    size_t ssid_len = strnlen(info->ssid, WIFI_SSID_MAX_LEN);
    if (ssid_len > (sizeof(message->ssid.bytes) - 1)) { /* defensive */
        ssid_len = sizeof(message->ssid.bytes) - 1;
    }
    memcpy(message->ssid.bytes, info->ssid, ssid_len);
    message->ssid.size = (pb_size_t)ssid_len;

    memcpy(message->bssid.bytes, info->bssid, sizeof(info->bssid));
    message->bssid.size = sizeof(info->bssid);

    message->security = info->security;
    message->channel = info->channel;
    message->frequency = info->frequency;

    return 0;
}

int parse_wifi_WifiSoftAPInfo(wifi_WifiSoftAPInfo* message, wifi_softap_info_t* info) {
    if (!message || !info) return -1;

    info->device_count = message->device_count;
    info->state = message->state;

    if (message->has_ip_address) {
        size_t ipv4_size = (size_t)message->ip_address.ipv4.size;
        if (ipv4_size > sizeof(info->ip_address.ipv4)) ipv4_size = sizeof(info->ip_address.ipv4);
        memcpy(info->ip_address.ipv4, message->ip_address.ipv4.bytes, ipv4_size);

        size_t ipv6_size = (size_t)message->ip_address.ipv6.size;
        if (ipv6_size > sizeof(info->ip_address.ipv6)) ipv6_size = sizeof(info->ip_address.ipv6);
        memcpy(info->ip_address.ipv6, message->ip_address.ipv6.bytes, ipv6_size);
    }

    size_t sslen = (size_t)message->ssid.size;
    if (sslen > WIFI_SSID_MAX_LEN) sslen = WIFI_SSID_MAX_LEN;
    memcpy(info->ssid, message->ssid.bytes, sslen);
    info->ssid[sslen] = '\0';

    memcpy(info->bssid, message->bssid.bytes, sizeof(info->bssid));

    info->security = message->security;
    info->channel = (uint8_t)message->channel;
    info->frequency = message->frequency;

    return 0;
}


/* ---------- nanopb encode / decode ---------- */
/*
 * nanopb_encode
 *  - input: wifi_softap_info_t *info
 *  - output: *out_buffer (allocated via malloc inside and must be freed by caller), *out_size
 *  - return: 0 on success, -1 on failure
 */
int nanopb_encode(const wifi_softap_info_t* info, void** out_buffer, size_t* out_size) {
    if (!info || !out_buffer || !out_size) return -1;

    /* create a WifiSoftAPInfo message and populate it from info */
    wifi_WifiSoftAPInfo message = wifi_WifiSoftAPInfo_init_zero;
    if (parse_wifi_softap_info(info, &message) != 0) {
        return -1;
    }

    /* use provided max size */
    uint8_t buffer[wifi_WifiSoftAPInfo_size];
    /* create a stream that writes to our buffer */
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    if (!pb_encode(&stream, wifi_WifiSoftAPInfo_fields, &message)) {
        fprintf(stderr, "Nanopb encode failed: %s\n", PB_GET_ERROR(&stream));
        return -1;
    }

    /* success */
    *out_size = stream.bytes_written;
    *out_buffer = malloc(*out_size);
    if (!*out_buffer) return -1;
    memcpy(*out_buffer, buffer, *out_size);
    return 0;
}

/*
 * nanopb_decode
 *  - input: *buffer, size
 *  - output: wifi_softap_info_t *info
 *  - return: 0 on success, -1 on failure
 */
int nanopb_decode(void* buffer, size_t size, wifi_softap_info_t* out_info) {
    if (!buffer || size == 0 || !out_info) return -1;
    /* create a stream that reads from the buffer */
    pb_istream_t stream = pb_istream_from_buffer((const pb_byte_t*)buffer, size);

    /* create a WifiSoftAPInfo message to hold the decoded data */
    wifi_WifiSoftAPInfo message = wifi_WifiSoftAPInfo_init_default;

    if (!pb_decode(&stream, wifi_WifiSoftAPInfo_fields, &message)) {
        fprintf(stderr, "Nanopb decode failed: %s\n", PB_GET_ERROR(&stream));
        return -1;
    }

    memset(out_info, 0, sizeof(*out_info));
    parse_wifi_WifiSoftAPInfo(&message, out_info);
    return 0;
}

/*
 * nanopb_encode_array
 *  - input: wifi_softap_info_t *infos, int count
 *  - output: *out_buf (allocated via malloc inside and must be freed by caller), *out_size
 *  - return: 0 on success, -1 on failure
 */
int nanopb_encode_array(const wifi_softap_info_t* infos, int count, void** out_buf, size_t* out_size) {
    if (!infos || count == 0 || !out_buf || !out_size) return -1;

    wifi_WifiSoftAPList list = wifi_WifiSoftAPList_init_zero;

    for (size_t i = 0; i < count; i++) {
        if (parse_wifi_softap_info(&infos[i], &list.ap_list[i]) != 0) {
            return -1;
        }
    }
    list.ap_list_count = count;

    /* use provided max size */
    uint8_t buffer[wifi_WifiSoftAPList_size];
    /* create a stream that writes to our buffer */
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));

    if (!pb_encode(&stream, wifi_WifiSoftAPList_fields, &list)) {
        fprintf(stderr, "Nanopb encode failed: %s\n", PB_GET_ERROR(&stream));
        return -1;
    }

    /* success */
    *out_size = stream.bytes_written;
    *out_buf = malloc(*out_size);
    if (!*out_buf) return -1;
    memcpy(*out_buf, buffer, *out_size);
    return 0;
}

/*
 * nanopb_decode_array
 *  - input: *buf, size, int count
 *  - output: wifi_softap_info_t *out_infos
 *  - return: 0 on success, -1 on failure
 */
int nanopb_decode_array(void* buf, size_t size, wifi_softap_info_t** out_infos, int *out_count) {
    if (!buf || size == 0 || !out_infos || !out_count) return -1;

    pb_istream_t stream = pb_istream_from_buffer((const pb_byte_t*)buf, size);

    wifi_WifiSoftAPList list = wifi_WifiSoftAPList_init_zero;

    if (!pb_decode(&stream, wifi_WifiSoftAPList_fields, &list)) {
        fprintf(stderr, "Nanopb decode failed: %s\n", PB_GET_ERROR(&stream));
        return -1;
    }

    *out_count = list.ap_list_count;
    *out_infos = malloc(list.ap_list_count * sizeof(wifi_softap_info_t));
    for (size_t i = 0; i < list.ap_list_count; i++) {
        memset((*out_infos) + i, 0, sizeof(wifi_softap_info_t));
        parse_wifi_WifiSoftAPInfo(&list.ap_list[i], (*out_infos) + i);
    }

    return 0;
}
#endif /* NANOPB_USAGE_H */