#include "../sample_structure.h"
#include "tpl.h"

/* ---------- tpl encode / decode ---------- */

/*
 * tpl_encode
 *  - input: wifi_softap_info_t *info
 *  - output: *out_buf (allocated via malloc inside), *out_size
 *  - return: 0 on success, -1 on failure
 */
int tpl_encode(wifi_softap_info_t* info, void** out_buf, size_t* out_size) {
    if (!info || !out_buf || !out_size) return -1;

    /* Step 1. setup tpl map, format: wifi_softap_info_t */
    tpl_node* tn = tpl_map("iic#c#c#c#ici", &info->device_count, &info->state,
                           info->ip_address.ipv4, (int)sizeof(info->ip_address.ipv4),
                           info->ip_address.ipv6, (int)sizeof(info->ip_address.ipv6),
                           info->ssid, (int)sizeof(info->ssid), info->bssid,
                           (int)sizeof(info->bssid), &info->security, &info->channel,
                           &info->frequency);
    if (!tn) {
        fprintf(stderr, "tpl_map failed\n");
        return -1;
    }

    /* Step 2. pack data */
    if (tpl_pack(tn, 0) != 0) {
        tpl_free(tn);
        return -1;
    }

    /* get size */
    if (tpl_dump(tn, TPL_GETSIZE, out_size) != 0) {
        fprintf(stderr, "tpl_dump TPL_GETSIZE fail\n");
        tpl_free(tn);
        return -1;
    }

    /* Step 3. write out buffer */
    *out_buf = malloc(*out_size);
    if (!*out_buf) {
        fprintf(stderr, "malloc fail\n");
        tpl_free(tn);
        return -1;
    }

    int result = tpl_dump(tn, TPL_MEM | TPL_PREALLOCD, *out_buf, *out_size);
    tpl_free(tn);
    if (result != 0) {
        fprintf(stderr, "tpl_dump failed\n");
        free(*out_buf);
        return -1;
    }

    printf("Serialized struct done\n");
    printf("TPL buffer size: %zu\n", *out_size);
    for (size_t i = 0; i < *out_size; i++) {
        printf("%02X ", ((unsigned char*)(*out_buf))[i]);
    }
    printf("\n");

    return 0;
}

/*
 * tpl_decode
 *  - input: tpl_buf, tpl_size
 *  - output: out_info (filled)
 *  - return: 0 on success, -1 on failure
 */
int tpl_decode(void* tpl_buf, size_t tpl_size, wifi_softap_info_t* out_info) {
    if (!tpl_buf || tpl_size == 0 || !out_info) return -1;

    memset(out_info, 0, sizeof(*out_info));

    tpl_node* tn = tpl_map(
        "iic#c#c#c#ici", &out_info->device_count, &out_info->state,
        out_info->ip_address.ipv4, (int)sizeof(out_info->ip_address.ipv4),
        out_info->ip_address.ipv6, (int)sizeof(out_info->ip_address.ipv6),
        out_info->ssid, (int)sizeof(out_info->ssid), out_info->bssid,
        (int)sizeof(out_info->bssid), &out_info->security, &out_info->channel,
        &out_info->frequency);
    if (!tn) return -1;

    if (tpl_load(tn, TPL_MEM, tpl_buf, tpl_size) != 0) {
        tpl_free(tn);
        return -1;
    }

    if (tpl_unpack(tn, 0) != 1) {
        tpl_free(tn);
        return -1;
    }
    tpl_free(tn);
    return 0;
}