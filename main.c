#include "MPACK/mpack_usage.h"
#include "NANOPB/nanopb_usage.h"
#include "TPL/tpl_usage.h"
#include <time.h>

int SHOW_STRUCTURE = 0;
int SHOW_CAL = 0;

static void print_usage(int argc, char** argv) {
    if (argc >= 4) {
        if (strcmp(argv[2], "tpl") == 0 ||
            strcmp(argv[2], "mpack") == 0 ||
            strcmp(argv[2], "nanopb") == 0) {
            if (strcmp(argv[3], "server") == 0) {
                fprintf(stderr, "usage: %s %s %s server PORT\n", argv[0], argv[1], argv[2]);
                return;
            } else if (strcmp(argv[3], "client") == 0) {
                fprintf(stderr, "usage: %s %s %s client HOST PORT\n", argv[0], argv[1], argv[2]);
                return;
            }
        }
    }

    fprintf(stderr, "usage: %s SHOW_STRUCTURE(0/1) <tpl | mpack | nanopb> <benchmark_test [TEST_NUMBER]|no_socket|array_test [NUMBER]|server PORT|client HOST PORT>\n", argv[0]);
}

/* High-resolution wall-clock time in microseconds (uses CLOCK_MONOTONIC) */
static double now_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e6 + (double)ts.tv_nsec / 1e3;
}

/* calculateAverage
 *  - inputs: array of durations in microseconds (double)
 *  - returns: average in microseconds
 */
double calculateAverage(double* arr, int n) {
    if (n == 0) return 0.0;
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        sum += arr[i];
        if (SHOW_CAL) printf("arr[%d]=%.2f, sum=%.2f\n", i, arr[i], sum);
    }
    return sum / (double)n;
}

/* encode the wifi_softap_info_t struct
 * library: "tpl", "mpack", "nanopb"
 * out_buf, out_size: output buffer and size
 * returns 0 on success
 */
static int encode(char* library, wifi_softap_info_t* info, void** out_buf, size_t* out_size) {
    if (strcmp(library, "tpl") == 0) {
        if (tpl_encode(info, out_buf, out_size) != 0) {
            return -1;
        }
    } else if (strcmp(library, "mpack") == 0) {
        if (mpack_encode(info, out_buf, out_size) != 0) {
            return -1;
        }
    } else if (strcmp(library, "nanopb") == 0) {
        if (nanopb_encode(info, out_buf, out_size) != 0) {
            return -1;
        }
    } else {
        fprintf(stderr, "unsupported library: %s\n", library);
        return -1;
    }

    if (SHOW_STRUCTURE) {
        printf("Serialized struct done\nBuffer size: %zu\n", *out_size);
        for (size_t i = 0; i < *out_size; i++) {
            printf("%02X ", ((unsigned char*)(*out_buf))[i]);
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
static int decode(char* library, void* buf, size_t sz, wifi_softap_info_t* out_info) {
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
 * out_buf, out_size: output buffer and size
 * returns 0 on success
 */
static int encode_array(char* library, const wifi_softap_info_t* infos, int count, void** out_buf, size_t* out_size) {
    if (strcmp(library, "tpl") == 0) {
        if (tpl_encode_array(infos, count, out_buf, out_size) != 0) {
            return -1;
        }
    } else if (strcmp(library, "mpack") == 0) {
        if (mpack_encode_array(infos, count, out_buf, out_size) != 0) {
            return -1;
        }
    } else if (strcmp(library, "nanopb") == 0) {
        if (nanopb_encode_array(infos, count, out_buf, out_size) != 0) {
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
            printf("%02X ", ((unsigned char*)(*out_buf))[i]);
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
static int decode_array(char* library, void* buf, size_t sz, wifi_softap_info_t** out_infos, int* out_count) {
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

int do_no_socket_test(char* library, wifi_softap_info_t *info, double* total_time) {
    double start = now_us();
    void* buf = NULL;
    size_t sz = 0;

    if (encode(library, info, &buf, &sz) != 0) {
        perror("encode failed\n");
        return -1;
    }

    wifi_softap_info_t decoded_info;
    int result = decode(library, buf, sz, &decoded_info);
    free(buf);
    if (result != 0) {
        fprintf(stderr, "decode failed\n");
        return -1;
    }
    if (SHOW_STRUCTURE) {
        printf("Decoded struct:\n");
        print_wifi_softap_info(&decoded_info);
    }
    double end = now_us();
    *total_time = end - start; /* microseconds */
    return 0;
}

int do_array_no_socket_test(char* library, wifi_softap_info_t* infos, int array_size, double* total_time) {
    double start = now_us();

    void* buf = NULL;
    size_t sz = 0;
    if (encode_array(library, infos, array_size, &buf, &sz) != 0) {
        perror("encode failed\n");
        return -1;
    }
    if (SHOW_STRUCTURE) printf("Done encode array, start to decode\n");

    wifi_softap_info_t* decoded_infos = NULL;
    int count = 0;
    int result = decode_array(library, buf, sz, &decoded_infos, &count);
    free(buf);
    if (result != 0) {
        fprintf(stderr, "decode failed\n");
        return -1;
    }
    if (SHOW_STRUCTURE) {
        printf("Decoded struct:\n");
        for (int i = 0; i < count; i++) {
            print_wifi_softap_info(&decoded_infos[i]);
        }
    }
    double end = now_us();
    *total_time = end - start; /* microseconds */
    return 0;
}

int main(int argc, char** argv) {
    int ret = -1;
    if (argc < 4) {
        print_usage(argc, argv);
        return ret;
    }

    double total_time = 0.0;
    wifi_softap_info_t info;
    SHOW_STRUCTURE = atoi(argv[1]);

    if (strcmp(argv[3], "benchmark_test") == 0) {
        int test_number = 20;
        if (argc < 4) {
            print_usage(argc, argv);
            goto done;
        } else if (argc >= 5) test_number = atoi(argv[4]);
        if (test_number <= 0) {
            print_usage(argc, argv);
            goto done;
        }

        double no_socket_time[test_number];
        double two_structure_time[test_number];
        double ten_structure_time[test_number];
        memset(no_socket_time, 0, sizeof(no_socket_time));
        memset(two_structure_time, 0, sizeof(two_structure_time));
        memset(ten_structure_time, 0, sizeof(ten_structure_time));

        char* library = argv[2];

        int array_size = 10;
        wifi_softap_info_t infos[array_size];
        memset(infos, 0, sizeof(infos));

        for (int j = 0; j < array_size; j++) {
            getSampleData(&infos[j], j);
            snprintf(infos[j].ssid, sizeof(infos[j].ssid), "WiFi-%d", j);
        }

        double test_time = 0;
        // For the first run since the first time would take much time
        if (do_no_socket_test(library, &infos[0], &test_time) != 0) {
            fprintf(stderr, "no_socket test failed\n");
            goto done;
        }
        printf("First test-test time: %.2f\n", test_time);

        for (size_t i = 0; i < test_number; i++) {
            if (do_no_socket_test(library, &infos[0], no_socket_time + i) != 0) {
                fprintf(stderr, "no_socket test failed\n");
                goto done;
            }

            if (do_array_no_socket_test(library, infos, 2, &two_structure_time[i]) != 0) {
                fprintf(stderr, "array no_socket test failed\n");
                goto done;
            }

            if (do_array_no_socket_test(library, infos, array_size, ten_structure_time + i) != 0) {
                fprintf(stderr, "array no_socket test failed\n");
                goto done;
            }
        }

        printf("Average time for %s:\n", library);
        printf("Single structure: %.2f microseconds\n", calculateAverage(no_socket_time, test_number));
        printf("Array of 2 structures: %.2f microseconds\n", calculateAverage(two_structure_time, test_number));
        printf("Array of 10 structures: %.2f microseconds\n", calculateAverage(ten_structure_time, test_number));

        ret = 0;

    } else if (strcmp(argv[3], "no_socket") == 0) {
        /* test encode/decode without socket */
        getSampleData(&info, 0);
        if (do_no_socket_test(argv[2], &info, &total_time) != 0) {
            fprintf(stderr, "no_socket test failed\n");
            goto done;
        }

        ret = 0;

    } else if (strcmp(argv[3], "array_test") == 0) {
        int array_size = 2;
        if (argc == 5) {
            array_size = atoi(argv[4]);
            if (array_size <= 0) {
                print_usage(argc, argv);
                goto done;
            }
        }
        /* test encode/decode array without socket */
        wifi_softap_info_t infos[array_size];
        memset(infos, 0, sizeof(infos));

        for (int i = 0; i < array_size; i++) {
            getSampleData(&infos[i], i);
            snprintf(infos[i].ssid, sizeof(infos[i].ssid), "WiFi-%d", i);
        }

        if (do_array_no_socket_test(argv[2], infos, array_size, &total_time) != 0) {
            fprintf(stderr, "array no_socket test failed\n");
            goto done;
        }

        ret = 0;

    } else if (strcmp(argv[3], "server") == 0) {
        if (argc != 5) {
            print_usage(argc, argv);
            goto done;
        }

        void* buf = NULL;
        size_t sz = 0;

        // get data from socket
        if (do_server(argv[4], &buf, &sz) != 0) {
            fprintf(stderr, "server failed\n");
            goto done;
        }

        int result = decode(argv[2], buf, sz, &info);
        free(buf);
        if (result != 0) {
            fprintf(stderr, "decode failed\n");
            goto done;
        }

        if (SHOW_STRUCTURE) print_wifi_softap_info(&info);
        ret = 0;

    } else if (strcmp(argv[3], "client") == 0) {
        if (argc != 6) {
            print_usage(argc, argv);
            goto done;
        }

        void* buf = NULL;
        size_t sz = 0;

        getSampleData(&info, 0);
        if (encode(argv[2], &info, &buf, &sz) != 0) {
            perror("encode failed\n");
            goto done;
        }

        // send data
        int result = do_client(argv[4], argv[5], buf, sz);
        free(buf);
        if (result != 0) {
            fprintf(stderr, "do_client failed\n");
            goto done;
        }

        ret = 0;

    } else {
        print_usage(argc, argv);
    }

done:
    if (total_time) {
        printf("Total time: %.2f microseconds\n", total_time);
    }
    return ret;
}
