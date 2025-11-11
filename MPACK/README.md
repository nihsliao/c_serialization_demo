# mpack library introduction

### First of all
- [Git repository](https://github.com/ludocode/mpack)
- [Extensively documented](https://ludocode.github.io/mpack/)
    - 使用 [MPack v1.1.1](https://github.com/ludocode/mpack/releases/tag/v1.1.1)
- This demo use the `Reader API` and `Expect API` to encode and decode

### Explain
#### Encode / Decode
- Encode
    ```c
    mpack_writer_t writer;
    /* init growable writer: writer will allocate 'data' and set 'size' */
    mpack_writer_init_growable(&writer, &data, &size);

    /* Helper function to write data with mpack apis */
    ...

    mpack_error_t err = mpack_writer_destroy(&writer);
    ```

- Decode:
    ```c
    mpack_reader_t reader;
    mpack_reader_init_data(&reader, (const char*)buffer, size);

    /* Helper function to read data with mpack apis */
    ...

    mpack_error_t err = mpack_reader_destroy(&reader);
    ```

#### Helper function to read / write a single wifi_softap_info_t structure
- Pass the `wifi_softap_info_t` data to writer
    ```c
    /* Helper function to write a single wifi_softap_info_t structure */
    int write_single_structure(mpack_writer_t* writer, const wifi_softap_info_t* info) {
        /* write an array of 9 elements (fixed-order schema) */
        mpack_start_array(writer, 9);

        /* Use apis mapkc_write_* to encode data */
        ...

        mpack_finish_array(writer);
    }
    ```

- Get the `wifi_softap_info_t` data from reader (Expect API)
    ```c
    /* Helper function to read a single wifi_softap_info_t structure */
    int read_single_structure(mpack_reader_t* reader, wifi_softap_info_t* info) {
        /* read an array of 9 elements (fixed-order schema) */
        mpack_expect_array_match(reader, 9);

        /* Use apis mpack_expect_* to encode data */
        ...

        mpack_done_array(reader);
    }
    ```

#### Structure array
- The struct array could be accomplished by using nested array api
- encode `count` number of structures
    ```c
    mpack_writer_init_growable(&writer, &data, &size);

    mpack_start_array(&writer, (uint32_t)count); // the outer array count
    for (int i = 0; i < count; i++) {
        if (write_single_structure(&writer, &infos[i]) != 0) { // write single structure
            mpack_writer_flag_error(&writer, mpack_error_data);
            break;
        }
    }
    mpack_finish_array(&writer); // finish writing the outer array

    mpack_error_t err = mpack_writer_destroy(&writer);
    ```
- decode structure array
    - `mpack_expect_array()` should better be replace with `mpack_expect_array_max()`
    ```c
    mpack_reader_t reader;
    mpack_reader_init_data(&reader, (const char*)buffer, size);

    int count = mpack_expect_array(&reader);  // get the outer array count

    // malloc the structure array to read data
    wifi_softap_info_t* infos = (wifi_softap_info_t*)malloc(sizeof(wifi_softap_info_t) * count);

    for (int i = 0; i < count; i++) {
        if (read_single_structure(&reader, &infos[i]) != 0) { // read single structure
            mpack_reader_flag_error(&reader, mpack_error_data);
            break;
        }
    }
    mpack_done_array(&reader);// finish reading the outer array

    mpack_error_t err = mpack_reader_destroy(&reader);
    ```
