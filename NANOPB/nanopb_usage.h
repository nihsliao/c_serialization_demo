#ifndef NANOPB_USAGE_H
#define NANOPB_USAGE_H

#include "../sample_structure.h" /* defines wifi_softap_info_t, constants */
#include "nanopb/sample_structure.pb.h"
#include "nanopb/pb_encode.h"
#include "nanopb/pb_decode.h"

/* ---------- nanopb encode / decode ---------- */
/*
 * nanopb_encode
 *  - input: wifi_softap_info_t *info
 *  - output: *out_buffer (allocated via malloc inside and must be freed by caller), *out_size
 *  - return: 0 on success, -1 on failure
 */
int nanopb_encode(wifi_softap_info_t *info, void **out_buffer, size_t *out_size) {
    if (!info || !out_buffer || !out_size) return -1;

    /* create a WifiSoftAPInfo message and populate it from info */
    wifi_WifiSoftAPInfo message = wifi_WifiSoftAPInfo_init_default;

    /* simple integer assignments */
    message.device_count = info->device_count;
    message.state = info->state;
    message.has_ip_address = true;

    /* ip bytes: the generated types use .size and .bytes members */
    /* copy ipv4/ipv6 (exact sizes) */
    memcpy(message.ip_address.ipv4.bytes, info->ip_address.ipv4, sizeof(info->ip_address.ipv4));
    message.ip_address.ipv4.size = sizeof(info->ip_address.ipv4);

    memcpy(message.ip_address.ipv6.bytes, info->ip_address.ipv6, sizeof(info->ip_address.ipv6));
    message.ip_address.ipv6.size = sizeof(info->ip_address.ipv6);

    /* ssid: store length and bytes (ssid PB array length likely 33) */
    size_t ssid_len = strnlen(info->ssid, WIFI_SSID_MAX_LEN);
    if (ssid_len > (sizeof(message.ssid.bytes) - 1)) { /* defensive */
        ssid_len = sizeof(message.ssid.bytes) - 1;
    }
    memcpy(message.ssid.bytes, info->ssid, ssid_len);
    message.ssid.size = (pb_size_t)ssid_len;

    /* bssid */
    memcpy(message.bssid.bytes, info->bssid, sizeof(info->bssid));
    message.bssid.size = sizeof(info->bssid);

    message.security = info->security;
    message.channel = info->channel;
    message.frequency = info->frequency;

    /* Allocate output buffer sized by compile-time constant if available, else use stream sizing */
#ifdef wifi_WifiSoftAPInfo_size
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
#else
    /* fallback: use sizing stream first (dynamic) */
    pb_ostream_t sizing = PB_OSTREAM_SIZING;
    if (!pb_encode(&sizing, wifi_WifiSoftAPInfo_fields, &message)) {
        fprintf(stderr, "Nanopb sizing encode failed: %s\n", PB_GET_ERROR(&sizing));
        return -1;
    }
    *out_size = sizing.bytes_written;
    uint8_t *buf = malloc(*out_size);
    if (!buf) return -1;
    pb_ostream_t stream = pb_ostream_from_buffer(buf, *out_size);
    if (!pb_encode(&stream, wifi_WifiSoftAPInfo_fields, &message)) {
        fprintf(stderr, "Nanopb encode failed (2): %s\n", PB_GET_ERROR(&stream));
        free(buf);
        return -1;
    }
    *out_buf = buf;
    return 0;
#endif
}

/*
 * nanopb_decode
 *  - input: *buffer, size
 *  - output: wifi_softap_info_t *info
 *  - return: 0 on success, -1 on failure
 */
int nanopb_decode(void *buffer, size_t size, wifi_softap_info_t *out_info) {
    if (!buffer || size == 0 || !out_info) return -1;
    /* create a stream that reads from the buffer */
    pb_istream_t stream = pb_istream_from_buffer((const pb_byte_t *)buffer, size);

    /* create a WifiSoftAPInfo message to hold the decoded data */
    wifi_WifiSoftAPInfo message = wifi_WifiSoftAPInfo_init_default;

    if (!pb_decode(&stream, wifi_WifiSoftAPInfo_fields, &message)) {
        fprintf(stderr, "Nanopb decode failed: %s\n", PB_GET_ERROR(&stream));
        return -1;
    }

    memset(out_info, 0, sizeof(*out_info));
    /* populate out_info from the decoded message */
    out_info->device_count = message.device_count;
    out_info->state = message.state;

    /* decode ip_address */
    if (message.has_ip_address) {
        /* copy bytes (use size but should equal expected size) */
        size_t ipv4_size = (size_t)message.ip_address.ipv4.size;
        if (ipv4_size > sizeof(out_info->ip_address.ipv4)) ipv4_size = sizeof(out_info->ip_address.ipv4);
        memcpy(out_info->ip_address.ipv4, message.ip_address.ipv4.bytes, ipv4_size);

        size_t ipv6_size = (size_t)message.ip_address.ipv6.size;
        if (ipv6_size > sizeof(out_info->ip_address.ipv6)) ipv6_size = sizeof(out_info->ip_address.ipv6);
        memcpy(out_info->ip_address.ipv6, message.ip_address.ipv6.bytes, ipv6_size);
    }

    /* decode ssid */
    size_t sslen = (size_t)message.ssid.size;
    if (sslen > WIFI_SSID_MAX_LEN) sslen = WIFI_SSID_MAX_LEN;
    memcpy(out_info->ssid, message.ssid.bytes, sslen);
    out_info->ssid[sslen] = '\0';

    /* decode bssid */
    memcpy(out_info->bssid, message.bssid.bytes, sizeof(out_info->bssid));

    out_info->security = message.security;
    out_info->channel = (uint8_t)message.channel;
    out_info->frequency = message.frequency;

    return 0;
}
#endif /* NANOPB_USAGE_H */