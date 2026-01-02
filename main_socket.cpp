#include <math.h>
#include <time.h>

#include "MPACK/mpack_usage.h"
#include "NANOPB/nanopb_usage.h"
#include "TPL/tpl_usage.h"
#include "apis.h"
#include "handling_thread.hpp"

#define THREAD_BUFFER_SIZE 7

int SHOW_STRUCTURE = 0;
int SHOW_CAL = 0;
uint8_t bytesBuffer[MAX_BUFFER] = {0};
size_t bufferSize = 0;

static void print_usage(int argc, char** argv) {
    if (argc >= 3) {
        if (strcmp(argv[2], "server") == 0) {
            fprintf(stderr, "usage: %s %s server PORT\n", argv[0], argv[1]);
            return;
        } else if (strcmp(argv[2], "client") == 0) {
            fprintf(stderr, "usage: %s %s client HOST PORT\n", argv[0], argv[1]);
            return;
        }
    }
    fprintf(stderr, "usage: %s SHOW_STRUCTURE(0/1) <server PORT|client HOST PORT>\n", argv[0]);
}

int main(int argc, char** argv) {
    int ret = -1;
    if (argc < 4) {
        print_usage(argc, argv);
        return ret;
    }

    SHOW_STRUCTURE = atoi(argv[1]);
    bool isServer = strcmp(argv[2], "server") == 0;

    if ((isServer && argc == 4) || (strcmp(argv[2], "client") == 0 && argc == 5)) {
        ThreadManager manager{isServer};

        manager.startHandler();
        manager.startRun(argv);

        manager.stopHandler();

        ret = 0;
    } else {
        print_usage(argc, argv);
    }

    return ret;
}
