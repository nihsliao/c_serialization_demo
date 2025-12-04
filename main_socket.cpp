#include <math.h>
#include <time.h>

#include "MPACK/mpack_usage.h"
#include "NANOPB/nanopb_usage.h"
#include "TPL/tpl_usage.h"
#include "queue_common.h"
#include "handling_thread.hpp"
#include "apis.h"

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

UserCommand getNextCommandID(bool isServer) {
    int result = -1;

    while (result < 0) {
        cout << "(getNextCommandID) Waiting for command... (0:Exit";
        if (!isServer) {
            cout << ",\n TPL   : 1:Single Structure, 2:Two Structures Array, 3:Ten Structures Array";
            cout << ",\n MPACK : 4:Single Structure, 5:Two Structures Array, 6:Ten Structures Array";
            cout << ",\n NANOPB: 7:Single Structure, 8:Two Structures Array, 9:Ten Structures Array";
        } 
        cout << ")" << endl;
        cin >> result;
        if (isServer && result != 0) result = -1;
        else if (result <= (int)UserCommand::NANOPB_TEN_STRUCTURES_ARRAY) break;
    }

    return (UserCommand)result;
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
        manager.startSocket(argv);
        
        do {
            manager.currentCommandId = getNextCommandID(isServer);
            cout << "(main) Current Command ID: " << (int)manager.currentCommandId.load() << endl;
            if (manager.getShouldTerminate()) {
                cout << "The socket has terminated. Exit process...";
                break;
            }
            manager.handleCommandUpdated();
        } while (manager.currentCommandId.load() != UserCommand::EXIT);

        // user indicated exit, stop the handler thread
        manager.stopThreads();

        ret = 0;
    } else {
        print_usage(argc, argv);
    }

    return ret;
}
