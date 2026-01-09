// handling_thread.hpp
#ifndef HANDLING_THREAD_HPP
#define HANDLING_THREAD_HPP

#include <errno.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <functional>
#include <iostream>
#include <unordered_map>

#include "apis.h"
#include "queue_common.h"
#include "sample_structure.h"

using namespace std;

typedef std::function<void()> handleCommandFunction;
std::unordered_map<uint16_t, handleCommandFunction> handleCommandFunctionMap;

uint8_t buffer[MAX_BUFFER] = {0};
size_t size = 0;

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
        enqueueElement(inputQueue, QueuElement{});

        if (handlerThread.joinable()) {
            handlerThread.join();
        }
        clearQueue(inputQueue);
        clearQueue(outputQueue);
    }

    void startRun(char** argv) {
        if (isServer)
            runServer(argv);
        else
            runClient(argv);
    }

   private:
    int writeEventfd{-1};
    int sock;
    bool isServer{true};
    int dequeue_timeout = 1;
    std::chrono::seconds DEQUEUE_TIMEOUT = std::chrono::seconds(dequeue_timeout);
    BlockingConcurrentQueue<QueuElement> inputQueue;
    BlockingConcurrentQueue<QueuElement> outputQueue;
    atomic<bool> isHandlerRunning{false};
    thread handlerThread;

    void enqueueCommand(UserCommand command) {
        enqueueElement(inputQueue, {command});
    }

    void notifyPoll(int fd) {
        if (fd == -1) return;
        uint64_t u = 1;
        ssize_t s = write(fd, &u, sizeof(uint64_t));
        if (s != sizeof(uint64_t)) {
            perror("write to eventfd");
        }
    }

    /*
     * Add a new file descriptor to the set.
     */
    void addToPfds(struct pollfd pfds[], int newfd, int* pfNum, int events) {
        pfds[*pfNum].fd = newfd;
        pfds[*pfNum].events = events;
        pfds[*pfNum].revents = 0;

        (*pfNum)++;
    }

    /*
     * Remove a file descriptor at a given index from the set.
     */
    void delFromPfds(struct pollfd pfds[], int i, int* pfNum) {
        // Copy the one from the end over this one
        pfds[i] = pfds[*pfNum - 1];

        (*pfNum)--;
    }

    UserCommand getNextCommandID(bool isServer) {
        int result = -1;
        char buf[64];
        int n = read(STDIN_FILENO, buf, sizeof(buf) - 1);
        if (n >= 32) {
            std::cerr << "Input too long, max 32 characters." << std::endl;
            return UserCommand::UNINITIAL;
        }

        buf[n] = '\0';
        result = atoi(buf);
        std::cout << "Input: " << result << "\n";

        if (isServer && result != 0) result = -1;
        else if (result > (int)UserCommand::NANOPB_TEN_STRUCTURES_ARRAY || result < 0) result = -1;

        return (UserCommand)result;
    }

    void handleSocketClosed() {
        printf("(handleSocketClosed)\n");
        if (sock) close(sock);
    }

    int socketReceiver() {
        int ret = -1;
        QueuElement result = {};

        printf("(socketReceiver) Start reading socket ...\n");
        uint64_t data = 0;
        if (recv_all(sock, &data, sizeof(data)) != 0) {
            perror("recv commandId");
            return ret;
        }
        result.commandId = (UserCommand)be64toh(data);

        data = 0;
        if (recv_all(sock, &data, sizeof(data)) != 0) {
            perror("recv len");
            return ret;
        }

        size_t size = (size_t)be64toh(data);
        if (size == 0) {
            perror("invalid size 0");
            return ret;
        } else if (size > MAX_BUFFER) {
            perror("size too large");
            return ret;
        }
        result.data.resize(size);

        if (recv_all(sock, result.data.data(), size) != 0) {
            perror("recv payload");
            return ret;
        }

        printf("(socketReceiver) Done reading socket ...\n");
        enqueueElement(inputQueue, result);
        ret = 0;
        return ret;
    }

    int socketWriter() {
        int ret = -1;
        printf("(socketWriter) start writing\n");
        QueuElement result;
        uint64_t data = 0;
        if (!outputQueue.try_dequeue(result)) {
            return ret;
        }

        UserCommand commandId = result.commandId;
        printf("(socketWriter) commandId=%d\n", (int)commandId);
        if (commandId < UserCommand::TPL_SINGLE_STRUCTURE || commandId > UserCommand::NANOPB_TEN_STRUCTURES_ARRAY) return ret;
        data = htobe64((uint64_t)commandId);
        if (send_all(sock, &data, sizeof(data)) != 0) {
            perror("send len");
            return ret;
        }

        size_t size = result.data.size();
        data = htobe64((uint64_t)size);
        if (send_all(sock, &data, sizeof(data)) != 0) {
            perror("send len");
            return ret;
        }

        if (send_all(sock, result.data.data(), size) != 0) {
            perror("send payload");
            return ret;
        }
        printf("(socketWriter) done writing\n");
        ret = 0;
        return ret;
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
        struct pollfd pfs[3] = {};
        int pfNum = 0;
        int pollCount = 0;

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

        addToPfds(pfs, lsock, &pfNum, POLLIN);
        addToPfds(pfs, STDIN_FILENO, &pfNum, POLLIN);

        while (currentCommandId.load() != UserCommand::EXIT) {
            // accept a client connection
            pollCount = poll(pfs, pfNum, -1);
            if (pollCount == -1) {
                perror("poll fail");
                goto cleanup_lsock;
            } else if (pollCount == 0) {
                // timeout, continue to check for exit condition
                continue;
            }

            printf("Got poll() return %d\n", pollCount);
            for (int i = 0; i < pfNum; i++) {
                if (pfs[i].revents & POLLHUP) {
                    printf("  -> hang up detected\n");
                    printf("(runServer) Socket has closed. Retry later...\n");
                    break;
                } else if (pfs[i].revents & POLLIN) {
                    printf("  pfs[%d]: fd=%d, revents=0x%x\n", i, pfs[i].fd, pfs[i].revents);

                    if (pfs[i].fd == lsock) {
                        printf("  -> listening socket\n");
                        sock = accept(lsock, NULL, NULL);
                        if (sock < 0) {
                            perror("accept fail");
                            handleSocketClosed();
                            break;
                        }
                        addToPfds(pfs, sock, &pfNum, POLLIN | POLLHUP | POLLERR);
                    } else if (pfs[i].fd == STDIN_FILENO) {
                        printf("  -> user input\n");
                        // user input available
                        currentCommandId.store(getNextCommandID(true));
                        if (currentCommandId.load() == UserCommand::EXIT) {
                            // user indicated exit
                            printf("User indicated exit. Closing socket...\n");
                            break;
                        } else
                            cout << "Server only accepts 0 for Exit" << endl;
                    } else {
                        printf("  -> client socket\n");
                        if (socketReceiver() != 0) {
                            printf("Client socket error or closed\n");
                            // remove client socket from pfds
                            delFromPfds(pfs, i, &pfNum);
                            handleSocketClosed();
                            break;
                        }
                    }
                }
            }
        }
    cleanup_lsock:
        stopHandler();
        handleSocketClosed();
        clearQueue(inputQueue);
        printf("(runServer) Server exiting...\n");
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
            memset(buffer, 0, MAX_BUFFER);
            size_t size = min(element.data.size(), (size_t)MAX_BUFFER);
            copy(element.data.begin(), element.data.begin() + size, buffer);

            int mode = static_cast<int>(commandId) % 3;
            if (mode == 1) {
                decode(library, buffer, size, outInfos);
                outInfoSize = 1;
            } else {
                decode_array(library, buffer, size, outInfos, &outInfoSize);
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
        struct pollfd pfs[3] = {};
        int pfNum = 0;
        int pollCount = 0;

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

        writeEventfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (writeEventfd < 0) {
            perror("eventfd");
            goto socket_close;
        }
        addToPfds(pfs, sock, &pfNum, POLLIN | POLLHUP | POLLERR);
        addToPfds(pfs, STDIN_FILENO, &pfNum, POLLIN);
        addToPfds(pfs, writeEventfd, &pfNum, POLLIN);

        while (currentCommandId.load() != UserCommand::EXIT) {
            // wait for socket or user input
            pollCount = poll(pfs, pfNum, -1);
            if (pollCount == -1) {
                perror("poll fail");
                goto socket_close;
            } else if (pollCount == 0) {
                // timeout, continue to check for exit condition
                continue;
            }

            printf("Got poll() return %d\n", pollCount);
            for (int i = 0; i < pfNum; i++) {
                if (pfs[i].revents & (POLLHUP | POLLERR)) {
                    printf("  pfs[%d]: fd=%d, revents=0x%x\n", i, pfs[i].fd, pfs[i].revents);
                    printf("  -> hang up or error detected\n");
                    printf("(runClient) Socket has closed. Ending...\n");
                    goto socket_close;
                }

                if (pfs[i].revents & POLLIN) {
                    printf("  pfs[%d]: fd=%d, revents=0x%x\n", i, pfs[i].fd, pfs[i].revents);
                    if (pfs[i].fd == sock) {
                        printf("  -> socket\n");
                        char tmp[1];
                        int n = recv(sock, tmp, sizeof(tmp), MSG_DONTWAIT);

                        if (n == 0) {
                            // server closed gracefully
                            printf("Server closed connection\n");
                            goto socket_close;
                        }

                        if (n < 0) {
                            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                // nothing to read, false alarm
                                continue;
                            }
                            perror("recv error");
                            goto socket_close;
                        }
                        // n > 0 : server sent data
                        // since client "doesn't read", discard it
                    } else if (pfs[i].fd == STDIN_FILENO) {
                        printf("  -> user input\n");
                        // user input available
                        currentCommandId.store(getNextCommandID(false));
                        UserCommand commandId = currentCommandId.load();
                        if (commandId >= UserCommand::TPL_SINGLE_STRUCTURE && commandId <= UserCommand::NANOPB_TEN_STRUCTURES_ARRAY) {
                            printf("Enqueue commandId=%d to handler\n", (int)commandId);
                            enqueueCommand(commandId);
                        } else if (currentCommandId.load() == UserCommand::EXIT) {
                            // user indicated exit
                            printf("User indicated exit. Closing socket...\n");
                            break;
                        } else {
                            cout << "Client supports command from 0-9, 0:Exit";
                            cout << ",\n TPL   : 1:Single Structure, 2:Two Structures Array, 3:Ten Structures Array";
                            cout << ",\n MPACK : 4:Single Structure, 5:Two Structures Array, 6:Ten Structures Array";
                            cout << ",\n NANOPB: 7:Single Structure, 8:Two Structures Array, 9:Ten Structures Array)" << endl;
                        }
                    } else if (pfs[i].fd == writeEventfd) {
                        printf("  -> eventfd\n");
                        uint64_t u;
                        ssize_t s = read(writeEventfd, &u, sizeof(uint64_t));  // clear the eventfd
                        if (s != sizeof(uint64_t)) {
                            perror("read from eventfd");
                        }

                        if (socketWriter() != 0) {
                            printf("Client socket error or closed\n");
                            // remove client socket from pfds
                            delFromPfds(pfs, i, &pfNum);
                            goto socket_close;
                        }
                    }
                }
            }
        }

    socket_close:
        printf("(runClient) Socket has closed. Ending...\n");
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
            memset(buffer, 0, MAX_BUFFER);
            size_t size = 0;

            int mode = static_cast<int>(commandId) % 3;
            printf("(clientHandler) commandId=%d, mode=%d\n", (int)commandId, mode);
            if (mode == 1) {
                encode(library, &infos[0], buffer, &size);
            } else {
                encode_array(library, infos, 10 - (4 * mode), buffer, &size);
            }
            result.data.assign(buffer, buffer + size);
            enqueueElement(outputQueue, result);
            notifyPoll(writeEventfd);
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
        } else
            return nullptr;
    }
};

#endif