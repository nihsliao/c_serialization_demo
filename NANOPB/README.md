# nanopb library introduction

## First of all
- **!!請不要手動去修改 STRUCTURE.pb.\* 檔案內容!!**
    - **請修改 `.proto` OR `.options`** 再以指令重新產生
- `nanopb` 為 zlib license
- 詳細可以參考 `nanopb` 下的說明
    - [Git repository](https://github.com/nanopb/nanopb/)
    - [Homepage](https://jpa.kapsi.fi/nanopb/)
    - 使用 [Nanopb 0.4.9.1](https://github.com/nanopb/nanopb/releases/tag/nanopb-0.4.9.1)

### Requirement
- Install `python`, `protobuf`

##### 可選： python enviroment
```shell
python3 -m venv path/to/venv
source path/to/venv/bin/activate
python3 -m pip install protobuf
```
### Steps
1. Create `.proto` and `.options`(optional) 並移至 `nanopb` 檔案夾中
    - 可以參考 [sample_structure.h](./nanopb/sample_structure.proto)
2. 在 `nanopb` 檔案夾中執行 command 產生 `myprotocol.pb.c` & `myprotocol.pb.h`
    ```shell
    python generator/nanopb_generator.py myprotocol.proto  # For source checkout
    generator-bin/nanopb_generator myprotocol.proto        # For binary package
    ```
3. 在使用的地方 `include`
    ```c
    #include "myprotocol.pb.h"
    ```
4. `Makefile` 添加 library & 產生的 `.c` file
    ```shell
    NANOPB = NANOPB/nanopb/*.c
    SRC = main.c $(NANOPB)
    ```

### Explain
- The encode and decode both need to create a stream and call the encode/decode functions
#### Encode / Decode
- Encode
    ```c
    /*
    * nanopb_encode
    *  - input: wifi_softap_info_t *info
    *  - output: *out_buffer (allocated via malloc inside and must be freed by caller), *out_size
    *  - return: 0 on success, -1 on failure
    */
    int nanopb_encode(const wifi_softap_info_t* info, void** out_buffer, size_t* out_size) {
        wifi_WifiSoftAPInfo message = wifi_WifiSoftAPInfo_init_zero;

        pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
        if (!pb_encode(&stream, wifi_WifiSoftAPInfo_fields, &message)) {
            fprintf(stderr, "Nanopb encode failed: %s\n", PB_GET_ERROR(&stream));
            return -1;
        }

        *out_size = stream.bytes_written;
        memcpy(*out_buffer, buffer, *out_size);
    }
    ```

- Decode
    ```c
    /*
    * nanopb_decode
    *  - input: *buffer, size
    *  - output: wifi_softap_info_t *info
    *  - return: 0 on success, -1 on failure
    */
    int nanopb_decode(void* buffer, size_t size, wifi_softap_info_t* out_info) {
        /* create a WifiSoftAPInfo message to hold the decoded data */
        wifi_WifiSoftAPInfo message = wifi_WifiSoftAPInfo_init_zero;

        pb_istream_t stream = pb_istream_from_buffer((const pb_byte_t*)buffer, size);
        if (!pb_decode(&stream, wifi_WifiSoftAPInfo_fields, &message)) {
            fprintf(stderr, "Nanopb decode failed: %s\n", PB_GET_ERROR(&stream));
            return -1;
        }
        parse_wifi_WifiSoftAPInfo(&message, out_info);
        ...
    }
    ```

#### Structure 轉換
- 定義的 `wifi_softap_info_t` 與 protobuf 產生的 `wifi_WifiSoftAPInfo` 不能直接對應，所以需要轉換兩者的 functions
    ```c
    /*
    * parse_wifi_softap_info
    *  - input: wifi_softap_info_t* info
    *  - output: wifi_WifiSoftAPInfo* message
    *  - return: 0 on success, -1 on failure
    */
    int parse_wifi_softap_info(const wifi_softap_info_t* info, wifi_WifiSoftAPInfo* message);

    /*
    * parse_wifi_WifiSoftAPInfo
    *  - input: wifi_WifiSoftAPInfo* message
    *  - output: wifi_softap_info_t* info
    *  - return: 0 on success, -1 on failure
    */
    int parse_wifi_WifiSoftAPInfo(wifi_WifiSoftAPInfo* message, wifi_softap_info_t* info);
    ```

#### Structure array
- 以 protobuf 宣告 structure array，並定義上限為 20
    ```protobuf
    // .proto
    message WifiSoftAPList {
        repeated WifiSoftAPInfo ap_list = 1;
    }

    // .options
    wifi.WifiSoftAPList.ap_list max_count: 20
    ```

- encode / decode 差異主要在修改單結構的 `wifi_WifiSoftAPInfo` 成 `wifi_WifiSoftAPList`
    - encode
        ```c
        /*
        * nanopb_encode_array
        *  - input: wifi_softap_info_t *infos, int count
        *  - output: *out_buffer (allocated via malloc inside and must be freed by caller), *out_size
        *  - return: 0 on success, -1 on failure
        */
        int nanopb_encode_array(const wifi_softap_info_t* infos, int count, void** out_buffer, size_t* out_size) {
            wifi_WifiSoftAPList list = wifi_WifiSoftAPList_init_zero;

            // fulfill wifi_WifiSoftAPList with wifi_softap_info_t array and size
            for (size_t i = 0; i < count; i++) {
                if (parse_wifi_softap_info(&infos[i], &list.ap_list[i]) != 0) {
                    return -1;
                }
            }
            list.ap_list_count = count;
            ...

            pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
            if (!pb_encode(&stream, wifi_WifiSoftAPList_fields, &list)) {
                fprintf(stderr, "Nanopb encode failed: %s\n", PB_GET_ERROR(&stream));
                return -1;
            }

            *out_size = stream.bytes_written;
            memcpy(*out_buffer, buffer, *out_size);
            ...
        }
        ```

    - decode
        ```c
        /*
        * nanopb_decode_array
        *  - input: *buf, size, int count
        *  - output: wifi_softap_info_t *out_infos
        *  - return: 0 on success, -1 on failure
        */
        int nanopb_decode_array(void* buf, size_t size, wifi_softap_info_t** out_infos, int* out_count) {
            wifi_WifiSoftAPList list = wifi_WifiSoftAPList_init_zero;

            pb_istream_t stream = pb_istream_from_buffer((const pb_byte_t*)buf, size);
            if (!pb_decode(&stream, wifi_WifiSoftAPList_fields, &list)) {
                fprintf(stderr, "Nanopb decode failed: %s\n", PB_GET_ERROR(&stream));
                return -1;
            }

            /* Transfer the wifi_WifiSoftAPList back to wifi_softap_info_t array */
            ...
        }
        ```
