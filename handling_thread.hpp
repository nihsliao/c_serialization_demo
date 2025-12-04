// handling_thread.hpp
#ifndef HANDLING_THREAD_HPP
#define HANDLING_THREAD_HPP

#include <sys/socket.h>
#include <atomic>
#include <functional>
#include <unordered_map>
#include <iostream>

#include "sample_structure.h"
#include "apis.h"

using namespace std;

typedef std::function<void()> handleCommandFunction;
std::unordered_map<uint16_t, handleCommandFunction> handleCommandFunctionMap;

// Utility function to convert integer to hex string
void printBufferHex(const void* buffer, size_t size) {
    printf("Buffer size: %zu\n", size);
    for (size_t i = 0; i < size; i++) {
        printf("%02X ", ((unsigned char*)buffer)[i]);
    }
    printf("\n");
}

class ThreadManager {
    public:
    atomic<UserCommand> currentCommandId{UserCommand::UNINITIAL};

    ThreadManager(bool server) {
        isServer = server;
        inputQueue = BlockingConcurrentQueue<QueuElement>(20); 
        outputQueue = BlockingConcurrentQueue<QueuElement>(20);
    }

    void startHandler() {
        isHandlerRunning = true;
        auto handlerFunction = [this]() {
            if (isServer)
                serverHandler();
            else
                clientHandler();
        };

        handlerThread = std::thread(handlerFunction);
    }

    void stopHandler() {
        printf("(stopHandler)\n");
        isHandlerRunning = false;
        // unblock handler thread if blocked on wait_dequeue_timed
        enqueueElement(outputQueue, QueuElement{});

        if (handlerThread.joinable()) {
            handlerThread.join();
        }
    }

    void startSocket(char** argv) {
        socketThread = std::thread([this, argv]() {
            if (isServer)
                runServer(argv);
            else
                runClient(argv);
        });
    }

    void stopSocket() {
        printf("(stopSocket)\n");
        isSocketRunning = false;
        // unblock socket thread if blocked on read/write
        ::shutdown(sock, SHUT_RDWR);
        // unblock writer thread if blocked on wait_dequeue_timed
        enqueueElement(outputQueue, QueuElement{});

        if (socketThread.joinable()) {
            socketThread.join();
        }
    }

    void stopThreads() {
        printf("(stopThreads)\n");
        stopSocket();
        stopHandler();
        handleSocketClosed();
        clearQueue(inputQueue);
        clearQueue(outputQueue);
    }

    void setShouldTerminate(bool action) {
        shouldTerminate = action;
    }

    bool getShouldTerminate() {
        return shouldTerminate;
    }

    void handleCommandUpdated() {
        if (currentCommandId == UserCommand::EXIT) setShouldTerminate(true);
        else if (!isServer) enqueueCommand(currentCommandId);
    }

    void enqueueCommand(UserCommand command) {
        enqueueElement(inputQueue, {command});
    }

    private:
    int sock;
    bool isServer{true};
    int dequeue_timeout = 1;
    std::chrono::seconds DEQUEUE_TIMEOUT = std::chrono::seconds(dequeue_timeout);
    BlockingConcurrentQueue<QueuElement> inputQueue;
    BlockingConcurrentQueue<QueuElement> outputQueue;
    atomic<bool> isSocketRunning{true};
    atomic<bool> isHandlerRunning{false};
    atomic<bool> shouldTerminate{false};
    thread socketThread;
    thread readerThread;
    thread handlerThread;
    thread writerThread;

    void handleSocketClosed() {
        printf("(handleSocketClosed)\n");
        if (sock) close(sock);
        if (!isServer) setShouldTerminate(true);
    }

    // Use the receiver thread to read the encoded message from socket
    void socketReceiver() {
        QueuElement result = {};
        memset(result.buffer, 0, MAX_BUFFER);

        while (isSocketRunning) {
            printf("(socketReceiver) Before reading socket ...\n");

            uint64_t data = 0;
            if (recv_all(sock, &data, sizeof(data)) != 0) {
                perror("recv commandId");
                isSocketRunning = false;
                break;
            }
            result.commandId = (UserCommand)be64toh(data);

            data = 0;
            if (recv_all(sock, &data, sizeof(data)) != 0) {
                perror("recv len");
                isSocketRunning = false;
                break;
            }

            result.size = (size_t)be64toh(data);

            if (result.size == 0) {
                perror("invalid size 0");
                isSocketRunning = false;
                break;
            } else if (result.size > MAX_BUFFER) {
                perror("size too large");
                isSocketRunning = false;
                break;
            }

            if (recv_all(sock, result.buffer, result.size) != 0) {
                perror("recv payload");
                isSocketRunning = false;
                break;
            }

            printf("(socketReceiver) After reading socket ...\n");
            enqueueElement(inputQueue, result);
        }
    }

    // Use the writer thread to write the encoded message to socket
    void socketWriter() {
        printf("(socketWriter) start\n");
        QueuElement result;
        uint64_t data = 0;
        while (isSocketRunning) {
            if (!outputQueue.wait_dequeue_timed(result, DEQUEUE_TIMEOUT)) {
                continue;
            }

            UserCommand commandId = result.commandId;
            printf("(socketWriter) commandId=%d\n", (int)commandId);

            if (commandId < UserCommand::TPL_SINGLE_STRUCTURE || commandId > UserCommand::NANOPB_TEN_STRUCTURES_ARRAY) continue;
            data = htobe64((uint64_t)commandId);
            if (send_all(sock, &data, sizeof(data)) != 0) {
                perror("send len");
                isSocketRunning = false;
                break;
            }

            data = htobe64((uint64_t)result.size);
            if (send_all(sock, &data, sizeof(data)) != 0) {
                perror("send len");
                isSocketRunning = false;
                break;
            }

            if (send_all(sock, result.buffer, result.size) != 0) {
                perror("send payload");
                isSocketRunning = false;
                break;
            }
            printf("(socketWriter) end\n");
        }
    }

    /*
    * The main server loop to manage socket connection and respond to transfer the incoming encoded messages to handler thread through inputQueue
    *  - portstr: port to listen
    * 
    */
    void runServer(char** argv) {
        const char* portstr = argv[3];
        if (!portstr) return;
        int port = atoi(portstr);
        int lsock;
        struct sockaddr_in addr;
        int opt = 1;

        printf("(runServer) start...\n");
        lsock = socket(AF_INET, SOCK_STREAM, 0);
        if (lsock < 0) {
            perror("socket start fail");
            goto cleanup_lsock;
        }

        if (setsockopt(lsock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            perror("setsockopt fail");
            goto cleanup_lsock;
        }

        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(lsock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("bind fail");
            goto cleanup_lsock;
        }

        if (listen(lsock, 1) < 0) {
            perror("listen fail");
            goto cleanup_lsock;
        }
        printf("Server listening on %d ...\n", port);


        while (currentCommandId.load() != UserCommand::EXIT) {
            // accept a client connection
            printf("Waiting for client connection ...\n");
            sock = accept(lsock, NULL, NULL);
            if (shouldTerminate) {
                printf("Socket accepted but terminated by user input\n");
                goto cleanup_sock;
            }
            if (sock < 0) {
                perror("accept fail");
                goto cleanup_sock;
            }

            isSocketRunning = true;
            readerThread = std::thread([this]() {
                socketReceiver();
            });

            while (isSocketRunning && currentCommandId.load() != UserCommand::EXIT) {
                this_thread::sleep_for(chrono::seconds(1));
            }
            if (readerThread.joinable()) readerThread.join();
            printf("(runServer) Socket has closed. Retry later...\n");
        cleanup_sock:
            handleSocketClosed();
        }
    cleanup_lsock:
        if (lsock) close(lsock);
    }

    // The server handler thread respond to decode the queued messages from the socket through the inputQueue
    void serverHandler() {
        printf("handler start...\n");
        QueuElement element;
        wifi_softap_info_t outInfos[MAX_ARRAY];
        int outInfoSize = 0;
        while (isHandlerRunning) {
            if (!inputQueue.wait_dequeue_timed(element, DEQUEUE_TIMEOUT)) {
                continue;
            }

            memset(outInfos, 0, sizeof(outInfos));
            outInfoSize = 0;
            UserCommand commandId = element.commandId;
            const char* library = getLibraryFromCommand(commandId);
            if (library == nullptr) continue;

            int mode = static_cast<int>(commandId) % 3;
            if (mode == 1) {
                decode(library, element.buffer, element.size, outInfos);
                outInfoSize = 1;
            } else {
                decode_array(library, element.buffer, element.size, outInfos, &outInfoSize);
            }
            
            for (int i = 0; i < outInfoSize; i++) {
                print_wifi_softap_info(outInfos + i);
            }
        }

        printf("handler end\n");
    }

    /*
    * The main client loop to manage socket connection and respond to transfer the user command to handler thread through inputQueue
    *  - host: IP or hostname (we use inet_pton for simplicity; pass IP string)
    *  - portstr: decimal port string
    * 
    */
    void runClient(char** argv) {
        const char* host = argv[3];
        const char* portstr = argv[4];
        if (!portstr) return;
        int port = atoi(portstr);
        struct sockaddr_in addr;

        printf("Client start...\n");
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            perror("socket start fail");
            goto socket_close;
        }

        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);

        if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
            fprintf(stderr, "inet_pton fail for host %s\n", host);
            goto socket_close;
        }

        if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("connect");
            goto socket_close;
        }

        isSocketRunning = true;
        writerThread =  std::thread([this]() {
            socketWriter();
        });

        while (isSocketRunning) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        if (writerThread.joinable()) writerThread.join();
        printf("(runClient) Socket has closed. Ending...\n");
    socket_close:
        handleSocketClosed();
    }

    // The client handler thread respond to decode the queued messages from the socket through the inputQueue
    void clientHandler() {
        printf(("handler start..."));
        QueuElement input, result;

        wifi_softap_info_t infos[MAX_ARRAY];
        fulfillSampleData(infos, MAX_ARRAY);

        while (isHandlerRunning) {
            if (!inputQueue.wait_dequeue_timed(input, DEQUEUE_TIMEOUT)) {
                continue;
            }

            UserCommand commandId = input.commandId;
            const char* library = getLibraryFromCommand(commandId);
            if (library == nullptr) continue;

            result = QueuElement{commandId};

            int mode = static_cast<int>(commandId) % 3;
            printf("(clientHandler) commandId=%d, mode=%d\n", (int)commandId, mode);
            if (mode == 1) {
                // encode(library, input.buffer, input.size, outInfos);
                encode(library, &infos[0], result.buffer, &result.size);
            } else {
                // decode_array(library, input.buffer, input.size, outInfos, &outInfoSize);
                encode_array(library, infos, 10-(4*mode), result.buffer, &result.size);
            }
            enqueueElement(outputQueue, result);
        }
        printf(("handler end\n"));
    }

    const char* getLibraryFromCommand(UserCommand command) {
        if (command < UserCommand::TPL_SINGLE_STRUCTURE) {
            return nullptr;
        } else if (command < UserCommand::MPACK_SINGLE_STRUCTURE) {
            return "tpl";
        } else if (command < UserCommand::NANOPB_SINGLE_STRUCTURE) {
            return "mpack";
        } else if (command <= UserCommand::NANOPB_TEN_STRUCTURES_ARRAY) {
            return "nanopb";
        } else return nullptr;
    }
};

#endif