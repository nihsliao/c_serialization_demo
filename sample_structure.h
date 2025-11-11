#ifndef SAMPLE_STRUCTURE_H
#define SAMPLE_STRUCTURE_H
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_ARRAY 20
#define MAX_BUFFER 4096

#define WIFI_SSID_MAX_LEN 32
#define WIFI_BT_MAC_ADDRESS_LEN 6
#define IPV4_LEN 4
#define IPV6_LEN 16

/* WiFi scan results */
typedef enum {
    WIFI_AP_STATE_UNKNOWN = 0,
    WIFI_AP_STATE_DISABLED,
    WIFI_AP_STATE_ENABLED,
} wifi_softap_state_t;

/* IP address */
typedef struct {
    uint8_t ipv4[4];  /* IPv4 address in binary format (e.g., "192.168.1.1") */
    uint8_t ipv6[16]; /* IPv6 address in binary format (e.g.,
                         "fe80::1ff:fe23:4567:890a") */
} ip_addr_t;

/* WiFi security types */
typedef enum {
    WIFI_SECURITY_TYPE_NONE = 0, /* Open network */
    WIFI_SECURITY_TYPE_WEP,
    WIFI_SECURITY_TYPE_WPA,
} security_type_t;

/* WiFi SoftAP status */
typedef struct {
    int device_count;                       /* Number of connected devices */
    wifi_softap_state_t state;              /* Current state of the SoftAP */
    ip_addr_t ip_address;                   /* IP address of the SoftAP */
    char ssid[WIFI_SSID_MAX_LEN + 1];       /* SSID of the SoftAP */
    uint8_t bssid[WIFI_BT_MAC_ADDRESS_LEN]; /* BSSID of the SoftAP */
    security_type_t security;               /* Security type of the SoftAP */
    uint8_t channel;                        /* Channel of the SoftAP */
    uint16_t frequency;                     /* Frequency of the SoftAP (e.g., 2412 for 2.4GHz, 5180
                                               for 5GHz) */
} wifi_softap_info_t;

static void getSingleSampleData(wifi_softap_info_t* info, int count) {
    /* prepare a sample payload */
    memset(info, 0, sizeof(*info));
    info->device_count = 2 + count;
    info->state = (int32_t)WIFI_AP_STATE_ENABLED;
    info->ip_address.ipv4[0] = 192;
    info->ip_address.ipv4[1] = 168;
    info->ip_address.ipv4[2] = 1 + count;
    info->ip_address.ipv4[3] = 100 + count;
    /* leave ipv6 zeroed for demo */
    strncpy(info->ssid, "MyAP", sizeof(info->ssid) - 1);
    uint8_t mac[WIFI_BT_MAC_ADDRESS_LEN] = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x01};
    memcpy(info->bssid, mac, WIFI_BT_MAC_ADDRESS_LEN);
    info->security = (int32_t)WIFI_SECURITY_TYPE_WPA;
    info->channel = 6 + count;
    info->frequency = 2437 + (count * 5);
}

static int fulfillSampleData(wifi_softap_info_t* array, int array_size) {
    if (array_size == 0 || array_size > MAX_ARRAY) {
        return -1;
    }

    for (int i = 0; i < array_size; i++) {
        getSingleSampleData(&array[i], i);
        snprintf(array[i].ssid, sizeof(array[i].ssid), "WiFi-%d", i);
    }
    return 0;
}

static void print_wifi_softap_info(const wifi_softap_info_t* info) {
    if (!info) return;

    printf("device_count=%d\n", info->device_count);
    printf("state=%d\n", info->state);
    printf("ipv4=%u.%u.%u.%u\n", info->ip_address.ipv4[0],
           info->ip_address.ipv4[1], info->ip_address.ipv4[2],
           info->ip_address.ipv4[3]);
    printf("ssid=%s\n", info->ssid);
    printf("bssid=%02X:%02X:%02X:%02X:%02X:%02X\n", info->bssid[0],
           info->bssid[1], info->bssid[2], info->bssid[3], info->bssid[4],
           info->bssid[5]);
    printf("security=%d channel=%u freq=%d\n", info->security,
           (unsigned)info->channel, info->frequency);
}

/* helper: send all */
static int send_all(int fd, const void* buf, size_t len) {
    const uint8_t* p = buf;
    size_t sent = 0;
    while (sent < len) {
        ssize_t s = send(fd, p + sent, len - sent, 0);
        if (s < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (s == 0) return -1;
        sent += (size_t)s;
    }
    return 0;
}

/* helper: recv exactly len */
static int recv_all(int fd, void* buf, size_t len) {
    uint8_t* p = buf;
    size_t got = 0;
    while (got < len) {
        ssize_t r = recv(fd, p + got, len - got, 0);
        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (r == 0) return -1;
        got += (size_t)r;
    }
    return 0;
}

/* ---------- socket helpers (modular) ---------- */

/*
 * socket_send
 *  - host: IP or hostname (we use inet_pton for simplicity; pass IP string)
 *  - portstr: decimal port string
 *  - buffer, size: payload to send
 *  - return 0 on success, -1 on failure
 */
static int socket_send(const char* host, const char* portstr, void* buffer, size_t size) {
    int ret = -1;
    /* Create socket */
    int port = atoi(portstr);
    struct sockaddr_in addr;
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        perror("socket");
        goto cleanup;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
        fprintf(stderr, "inet_pton fail for host %s\n", host);
        goto cleanup;
    }

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        goto cleanup;
    }

    /* send 8-byte length in network order, then payload */
    uint64_t netlen = htobe64((uint64_t)size);
    if (send_all(sock, &netlen, sizeof(netlen)) != 0) {
        perror("send len");
        goto cleanup;
    }

    if (send_all(sock, buffer, size) != 0) {
        perror("send payload");
        goto cleanup;
    }

    printf("Client: sent %zu bytes\n", size);
    ret = 0;

cleanup:
    if (sock) close(sock);
    return ret;
}

/*
 * socket_receive
 *  - portstr: port to listen
 *  - buffer: pointer to malloc'd buffer containing payload (returned)
 *  - size: payload size returned
 *  - returns 0 on success, -1 on failure
 *
 * Note: this function accepts one client connection and returns its payload.
 */
static int socket_receive(const char* portstr, void** buffer, size_t* size) {
    int ret = -1;
    if (!portstr || !buffer || !size) return ret;
    int port = atoi(portstr);
    int lsock, csock;
    struct sockaddr_in addr;
    int opt = 1;

    lsock = socket(AF_INET, SOCK_STREAM, 0);
    if (lsock < 0) {
        perror("socket");
        goto cleanup_lsock;
    }

    setsockopt(lsock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(lsock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        goto cleanup_lsock;
    }

    if (listen(lsock, 1) < 0) {
        perror("listen");
        goto cleanup_lsock;
    }

    printf("Server listening on %d ...\n", port);
    csock = accept(lsock, NULL, NULL);
    if (csock < 0) {
        perror("accept");
        goto cleanup_all;
    }

    uint64_t netlen;
    if (recv_all(csock, &netlen, sizeof(netlen)) != 0) {
        perror("recv len");
        goto cleanup_all;
    }

    *size = (size_t)be64toh(netlen);
    if (*size == 0) {
        perror("invalid size 0");
        goto cleanup_all;
    } else if (*size > MAX_BUFFER) {
        perror("size too large");
        goto cleanup_all;
    }

    void* buf = malloc(*size);
    if (!buf) {
        fprintf(stderr, "malloc fail\n");
        goto cleanup_all;
    }

    if (recv_all(csock, buf, *size) != 0) {
        perror("recv payload");
        goto cleanup_all;
    }

    printf("Server: expecting %zu bytes\n", *size);
    *buffer = buf;
    ret = 0;

cleanup_all:
    if (csock) close(csock);
cleanup_lsock:
    if (lsock) close(lsock);
    return ret;
}

/* ---------- high level do_client / do_server using modular helpers ----------
 */

/*
 * do_client
 *  - host, portstr
 *  - buf: buffer to send
 *  - size: size of buffer
 *  - returns 0 on success
 */
int do_client(const char* host, const char* portstr, void* buffer, size_t size) {
    if (!host || !portstr || !buffer || !size) return -1;

    int result = socket_send(host, portstr, buffer, size);
    if (result != 0) {
        printf("Socket send failed\n");
        return -1;
    }

    printf("do_client: sent %zu bytes to %s:%s\n", size, host, portstr);
    return 0;
}

/*
 * do_server
 *  - portstr to listen
 *  - out_buffer: pointer to malloc'd buffer containing payload (returned)
 *  - out_size: payload size returned
 *  - returns 0 on success
 */
int do_server(const char* portstr, void** out_buffer, size_t* out_size) {
    if (!portstr || !out_buffer || !out_size) return -1;

    int result = socket_receive(portstr, out_buffer, out_size);
    if (result != 0) {
        printf("Socket receive failed\n");
        return -1;
    }

    printf("do_server: received and decoded %zu bytes\n", *out_size);
    return 0;
}
#endif /* SAMPLE_STRUCTURE_H */