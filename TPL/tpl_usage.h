#include "../sample_structure.h"
#include "tpl.h"

/* ---------- tpl encode / decode ---------- */

/*
 * tpl_encode
 *  - input: wifi_softap_info_t *info
 *  - output: *out_buffer (allocated via malloc inside), *out_size
 *  - return: 0 on success, -1 on failure
 */
int tpl_encode(wifi_softap_info_t* info, void** out_buffer, size_t* out_size) {
    int ret = -1;
    if (!info || !out_buffer || !out_size) return ret;

    /* Step 1. setup tpl map, format: wifi_softap_info_t */
    tpl_node* tn = tpl_map("S(ii$(c#c#)c#c#icv)", info,
                           (int)sizeof(info->ip_address.ipv4),
                           (int)sizeof(info->ip_address.ipv6),
                           (int)sizeof(info->ssid),
                           (int)sizeof(info->bssid));

    if (!tn) {
        fprintf(stderr, "tpl_map failed\n");
        goto cleanup;
    }

    /* Step 2. pack data */
    if (tpl_pack(tn, 0) != 0) {
        goto cleanup;
    }

    int result = tpl_dump(tn, TPL_MEM, out_buffer, out_size);
    if (result != 0) {
        fprintf(stderr, "tpl_dump failed\n");
        goto cleanup;
    }

    ret = 0;
cleanup:
    if (tn) tpl_free(tn);
    return ret;
}

/*
 * tpl_decode
 *  - input: buffer, size
 *  - output: out_info (filled)
 *  - return: 0 on success, -1 on failure
 */
int tpl_decode(void* buffer, size_t size, wifi_softap_info_t* out_info) {
    int ret = -1;
    if (!buffer || size == 0 || !out_info) return ret;

    memset(out_info, 0, sizeof(*out_info));
    tpl_node* tn = tpl_map("S(ii$(c#c#)c#c#icv)", out_info,
                           (int)sizeof(out_info->ip_address.ipv4),
                           (int)sizeof(out_info->ip_address.ipv6),
                           (int)sizeof(out_info->ssid),
                           (int)sizeof(out_info->bssid));
    if (!tn) goto cleanup;

    if (tpl_load(tn, TPL_MEM, buffer, size) != 0) {
        goto cleanup;
    }

    if (tpl_unpack(tn, 0) != 1) {
        goto cleanup;
    }

    ret = 0;
cleanup:
    if (tn) tpl_free(tn);
    return ret;
}

int tpl_encode_array(const wifi_softap_info_t* infos, int count, void** out_buffer, size_t* out_size) {
    tpl_node* tn;
    int ret = -1;
    if (!infos || count <= 0 || !out_buffer || !out_size) return ret;
    wifi_softap_info_t tmp;
    memset(&tmp, 0, sizeof(tmp));

    tn = tpl_map("A(S(ii$(c#c#)c#c#icv))", &tmp,
                 sizeof(tmp.ip_address.ipv4),
                 sizeof(tmp.ip_address.ipv6),
                 sizeof(tmp.ssid),
                 sizeof(tmp.bssid));

    if (!tn) {
        fprintf(stderr, "tpl_map failed\n");
        goto cleanup;
    }

    for (size_t i = 0; i < count; i++) {
        memcpy(&tmp, &infos[i], sizeof(wifi_softap_info_t));
        if (tpl_pack(tn, 1) != 0) {
            fprintf(stderr, "tpl_pack struct %ld failed\n", i);
            goto cleanup;
        }
    }

    int result = tpl_dump(tn, TPL_MEM, out_buffer, out_size);
    if (result != 0) {
        fprintf(stderr, "tpl_dump failed\n");
        goto cleanup;
    }

    ret = 0;
cleanup:
    if (tn) tpl_free(tn);
    return ret;
}

int tpl_decode_array(const void* buf, size_t size, wifi_softap_info_t** out_infos, int* out_count) {
    tpl_node* tn;
    int ret = -1;
    if (!buf || size == 0 || !out_infos || !out_count) return ret;

    wifi_softap_info_t tmp;
    memset(&tmp, 0, sizeof(tmp));

    tn = tpl_map("A(S(ii$(c#c#)c#c#icv))", &tmp,
                 sizeof(tmp.ip_address.ipv4),
                 sizeof(tmp.ip_address.ipv6),
                 sizeof(tmp.ssid),
                 sizeof(tmp.bssid));

    if (!tn) {
        fprintf(stderr, "tpl_map failed\n");
        goto cleanup;
    }

    if (tpl_load(tn, TPL_MEM, buf, size) != 0) {
        fprintf(stderr, "tpl_load failed\n");
        goto cleanup;
    }

    int count = tpl_Alen(tn, 1);
    if (count <= 0) {
        fprintf(stderr, "invalid array length %d\n", count);
        goto cleanup;
    }

    // unpack each struct
    *out_infos = malloc(sizeof(wifi_softap_info_t) * count);
    if (!*out_infos) {
        fprintf(stderr, "malloc fail\n");
        goto cleanup;
    }

    int i = 0;
    while (tpl_unpack(tn, 1) > 0) {
        memcpy((*out_infos) + i, &tmp, sizeof(wifi_softap_info_t));
        i++;
        if (i >= count) break;
    }

    *out_count = count;
    ret = 0;
cleanup:
    if (tn) tpl_free(tn);
    return ret;
}