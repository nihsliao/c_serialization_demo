# tpl library introduction

### First of all
- [Git repository](https://github.com/troydhanson/tpl)
- [Documentation ](https://troydhanson.github.io/tpl/)
    - Use [tpl v1.6.1](https://github.com/troydhanson/tpl/releases/tag/v1.6.1)
- This demo use the `A(S(ii$(c#c#)c#c#icv))` format string to encode and decode


### Encode / Decode
- Encode
    - Detail could refer to [api concepts - tpl User Guide](https://troydhanson.github.io/tpl/userguide.html#_api_concepts)
    - This demo use structure format which don't need to specify each data for map
        - `S(...)` for structure
        - `$(...)` for nested structure
        - `c` for byte
        - `v` for 2 bytes unsigned int
        - `i` for 4 bytes signed int
    - **Remember to free the `tpl_node` when error or finish**
    ```c
    /*
    * tpl_encode
    *  - input: wifi_softap_info_t *info
    *  - output: **out_buffer (allocated via malloc inside), *out_size
    *  - return: 0 on success, -1 on failure
    */
    int tpl_encode(wifi_softap_info_t* info, void** out_buffer, size_t* out_size) {
        tpl_node* tn = tpl_map("S(ii$(c#c#)c#c#icv)", info,
                            (int)sizeof(info->ip_address.ipv4),
                            (int)sizeof(info->ip_address.ipv6),
                            (int)sizeof(info->ssid),
                            (int)sizeof(info->bssid));
        tpl_pack(tn, 0);
        // get the buffer and the buffer size
        int result = tpl_dump(tn, TPL_MEM, out_buffer, out_size);
        tpl_free(tn);
        ...
    }
    ```
- Decode
    - Similar flow to encode, but use `tpl_load()` then `tpl_unpack()`
    ```c
    /*
    * tpl_decode
    *  - input: buffer, size
    *  - output: out_info (filled)
    *  - return: 0 on success, -1 on failure
    */
    int tpl_decode(void* buffer, size_t size, wifi_softap_info_t* out_info) {
        tpl_node* tn = tpl_map("S(ii$(c#c#)c#c#icv)", out_info,
                            (int)sizeof(out_info->ip_address.ipv4),
                            (int)sizeof(out_info->ip_address.ipv6),
                            (int)sizeof(out_info->ssid),
                            (int)sizeof(out_info->bssid));

        tpl_load(tn, TPL_MEM, buffer, size);
        tpl_unpack(tn, 0);
        tpl_free(tn);
        ...
    }
    ```

### Structure array
- Refer to [Arrays - tpl User Guide](https://troydhanson.github.io/tpl/userguide.html#arrays)
- Use the `A(...)` to surrounding the `S(...)`
- Encode and decode with a temp structure
    ```c
    int tpl_encode_array(const wifi_softap_info_t* infos, int count, void** out_buffer, size_t* out_size) {
        wifi_softap_info_t tmp;

        tpl_node* tn = tpl_map("A(S(ii$(c#c#)c#c#icv))", &tmp,
                    sizeof(tmp.ip_address.ipv4),
                    sizeof(tmp.ip_address.ipv6),
                    sizeof(tmp.ssid),
                    sizeof(tmp.bssid));

        for (size_t i = 0; i < count; i++) {
            memcpy(&tmp, &infos[i], sizeof(wifi_softap_info_t));
            if (tpl_pack(tn, 1) != 0) {
                fprintf(stderr, "tpl_pack struct %ld failed\n", i);
                goto cleanup;
            }
        }

        tpl_dump(tn, TPL_MEM, out_buffer, out_size);
        tpl_free(tn);
    }
    ```

    ```c
    int tpl_decode_array(const void* buf, size_t size, wifi_softap_info_t** out_infos, int* out_count) {
        wifi_softap_info_t tmp;

        tpl_node* tn = tpl_map("A(S(ii$(c#c#)c#c#icv))", &tmp,
                    sizeof(tmp.ip_address.ipv4),
                    sizeof(tmp.ip_address.ipv6),
                    sizeof(tmp.ssid),
                    sizeof(tmp.bssid));

        tpl_load(tn, TPL_MEM, buf, size);

        *out_count = tpl_Alen(tn, 1);

        for  (i = 0; i < *out_count; i++) {
            if (tpl_unpack(tn, 1) == 0) break;
            memcpy((*out_infos) + i, &tmp, sizeof(wifi_softap_info_t));
        }
        tpl_free(tn);
        ...
    }
    ```

### Stack buffer
- Use `TPL_MEM | TPL_PREALLOCD` for stack buffer
- Use `TPL_GETSIZE` to get the size that the dump would require
    ```c
    /*
    * tpl_encode
    *  - input: wifi_softap_info_t *info
    *  - output: *out_buffer, *out_size
    *  - return: 0 on success, -1 on failure
    */
    int tpl_encode(wifi_softap_info_t* info, void* out_buffer, size_t* out_size) {
        tpl_node* tn = tpl_map("S(ii$(c#c#)c#c#icv)", info,
                            (int)sizeof(info->ip_address.ipv4),
                            (int)sizeof(info->ip_address.ipv6),
                            (int)sizeof(info->ssid),
                            (int)sizeof(info->bssid));

        tpl_pack(tn, 0);

        tpl_dump(tn, TPL_MEM | TPL_PREALLOCD, out_buffer, MAX_BUFFER);
        tpl_dump(tn, TPL_GETSIZE, out_size);
        tpl_free(tn);
        ...
    }
    ```