#ifndef APIS_H
#define APIS_H

#include "MPACK/mpack_usage.h"
#include "NANOPB/nanopb_usage.h"
#include "TPL/tpl_usage.h"

extern int SHOW_STRUCTURE;
extern int SHOW_CAL;

/* encode the wifi_softap_info_t struct
 * library: "tpl", "mpack", "nanopb"
 * out_buffer, out_size: output buffer and size
 * returns 0 on success
 */
static int encode(const char* library, wifi_softap_info_t* info, void* out_buffer, size_t* out_size) {
    if (strcmp(library, "tpl") == 0) {
        if (tpl_encode(info, out_buffer, out_size) != 0) {
            return -1;
        }
    } else if (strcmp(library, "mpack") == 0) {
        if (mpack_encode(info, out_buffer, out_size) != 0) {
            return -1;
        }
    } else if (strcmp(library, "nanopb") == 0) {
        if (nanopb_encode(info, out_buffer, out_size) != 0) {
            return -1;
        }
    } else {
        fprintf(stderr, "unsupported library: %s\n", library);
        return -1;
    }

    if (SHOW_STRUCTURE) {
        printf("Serialized struct done\nBuffer size: %zu\n", *out_size);
        for (size_t i = 0; i < *out_size; i++) {
            printf("%02X ", ((unsigned char*)out_buffer)[i]);
        }
        printf("\n");
    }

    return 0;
}

/* decode the wifi_softap_info_t struct
 * library: "tpl", "mpack", "nanopb"
 * buf, sz: input buffer and size
 * out_info: output struct
 * returns 0 on success
 */
static int decode(const char* library, void* buf, size_t sz, wifi_softap_info_t* out_info) {
    if (strcmp(library, "tpl") == 0) {
        if (tpl_decode(buf, sz, out_info) != 0) {
            return -1;
        }
    } else if (strcmp(library, "mpack") == 0) {
        if (mpack_decode(buf, sz, out_info) != 0) {
            return -1;
        }
    } else if (strcmp(library, "nanopb") == 0) {
        if (nanopb_decode(buf, sz, out_info) != 0) {
            return -1;
        }
    } else {
        fprintf(stderr, "unsupported library: %s\n", library);
        return -1;
    }
    return 0;
}

/* encode array of wifi_softap_info_t structs
 * library: "tpl", "mpack", "nanopb"
 * infos: input array of structs
 * count: number of structs
 * out_buffer, out_size: output buffer and size
 * returns 0 on success
 */
static int encode_array(const char* library, const wifi_softap_info_t* infos, int count, void* out_buffer, size_t* out_size) {
    if (strcmp(library, "tpl") == 0) {
        if (tpl_encode_array(infos, count, out_buffer, out_size) != 0) {
            return -1;
        }
    } else if (strcmp(library, "mpack") == 0) {
        if (mpack_encode_array(infos, count, out_buffer, out_size) != 0) {
            return -1;
        }
    } else if (strcmp(library, "nanopb") == 0) {
        if (nanopb_encode_array(infos, count, out_buffer, out_size) != 0) {
            return -1;
        }
    } else {
        fprintf(stderr, "unsupported library: %s\n", library);
        return -1;
    }

    if (SHOW_STRUCTURE) {
        printf("Serialized array done\n");
        printf("Buffer size: %zu\n", *out_size);
        for (size_t i = 0; i < *out_size; i++) {
            printf("%02X ", ((unsigned char*)out_buffer)[i]);
        }
        printf("\n");
    }

    return 0;
}

/* decode array of wifi_softap_info_t structs
 * library: "tpl", "mpack", "nanopb"
 * buf, sz: input buffer and size
 * out_infos: output array of structs
 * out_count: number of structs decoded
 * returns 0 on success
 */
static int decode_array(const char* library, void* buf, size_t sz, wifi_softap_info_t* out_infos, int* out_count) {
    if (strcmp(library, "tpl") == 0) {
        if (tpl_decode_array(buf, sz, out_infos, out_count) != 0) {
            return -1;
        }
    } else if (strcmp(library, "mpack") == 0) {
        if (mpack_decode_array(buf, sz, out_infos, out_count) != 0) {
            return -1;
        }
    } else if (strcmp(library, "nanopb") == 0) {
        if (nanopb_decode_array(buf, sz, out_infos, out_count) != 0) {
            return -1;
        }
    } else {
        fprintf(stderr, "unsupported library: %s\n", library);
        return -1;
    }
    return 0;
}
#endif