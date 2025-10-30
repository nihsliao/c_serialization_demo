#include "TPL/tpl_usage.h"

static void print_usage(char** argv) {
    fprintf(stderr, "usage: %s <tpl | mpack | nanopb> <no_socket|server PORT|client HOST PORT>\n", argv[0]);
}

static int encode(char* library, wifi_softap_info_t* info, void** out_buf, size_t* out_size) {
    if (strcmp(library, "tpl") == 0) {
        if (tpl_encode(info, out_buf, out_size) != 0) {
            return -1;
        }
    } else if (strcmp(library, "mpack") == 0) {
        fprintf(stdout, "TODO library: %s\n", library);

    } else if (strcmp(library, "nanopb") == 0) {
        fprintf(stdout, "TODO library: %s\n", library);
    } else {
        fprintf(stderr, "unsupported library: %s\n", library);
        return -1;
    }
    return 0;
}

static int decode(char* library, void* buf, size_t sz, wifi_softap_info_t* out_info) {
    if (strcmp(library, "tpl") == 0) {
        if (tpl_decode(buf, sz, out_info) != 0) {
            return -1;
        }
    } else if (strcmp(library, "mpack") == 0) {
        fprintf(stdout, "TODO library: %s\n", library);

    } else if (strcmp(library, "nanopb") == 0) {
        fprintf(stdout, "TODO library: %s\n", library);
    } else {
        fprintf(stderr, "unsupported library: %s\n", library);
        return -1;
    }
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        print_usage(argv);
        return -1;
    }

    wifi_softap_info_t info;
    void* buf = NULL;
    size_t sz = 0;

    if (strcmp(argv[2], "no_socket") == 0) {
        /* test encode/decode without socket */
        getSampleData(&info);

        // encode
        if (encode(argv[1], &info, &buf, &sz) != 0) {
            free(buf);
            perror("encode failed\n");
            return -1;
        }

        wifi_softap_info_t decoded_info;
        // decode
        int result = decode(argv[1], buf, sz, &decoded_info);
        free(buf);
        if (result != 0) {
            fprintf(stderr, "decode failed\n");
            return -1;
        }

        printf("Decoded struct:\n");
        print_wifi_softap_info(&decoded_info);
        return 0;

    } else if (strcmp(argv[2], "server") == 0) {
        if (argc != 4) {
            fprintf(stderr, "usage: %s %s server PORT\n", argv[0], argv[1]);
            return -1;
        }

        // get data from socket
        if (do_server(argv[3], &buf, &sz) != 0) {
            fprintf(stderr, "server failed\n");
            return -1;
        }

        // decode tpl data
        int result = decode(argv[1], buf, sz, &info);
        free(buf);
        if (result != 0) {
            fprintf(stderr, "decode failed\n");
            return -1;
        }

        // print received info
        print_wifi_softap_info(&info);
        return 0;

    } else if (strcmp(argv[2], "client") == 0) {
        if (argc != 5) {
            fprintf(stderr, "usage: %s %s client HOST PORT\n", argv[0], argv[1]);
            return -1;
        }

        getSampleData(&info);
        // encode tpl data
        if (encode(argv[1], &info, &buf, &sz) != 0) {
            free(buf);
            perror("encode failed\n");
            return -1;
        }

        // send tpl data
        int result = do_client(argv[3], argv[4], buf, sz);
        free(buf);
        if (result != 0) {
            fprintf(stderr, "do_client failed\n");
            return -1;
        }

        return 0;
    } else {
        print_usage(argv);
        return -1;
    }
}
