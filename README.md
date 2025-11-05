# serialization demo for c with libraries
Use same sample structure and encode / decode by the libries, and send the byte through socket

## Library
- [tpl](https://github.com/troydhanson/tpl)
- [mapck](https://github.com/ludocode/mpack)
- [nanopb](https://github.com/nanopb/nanopb)

### Compare
| Library               | tpl              | mpack            | nanopb           |
| --------------------- | ---------------- | ---------------- | ---------------- |
| license               | BSD license      | MIT license      | zlib License     |
| serializtion format   | tpl image        | MessagePack      | protobuf         |
| single structure      | 118<br>(785 ns)  | 46<br>(460 ns)   | 51<br>(588 ns)   |
| array of 2 structure  | 199<br>(1150 ns) | 97<br>(335 ns)   | 110<br>(1595 ns) |
| array of 10 structure | 791<br>(2760 ns) | 481<br>(1150 ns) | 550<br>(7254 ns) |

- The micorseconds are the average time of 10000 executions

## Usage
```shell
usage: ./serialize_demo SHOW_STRUCTURE(0/1) LIBRARY COMMAND
LIBRARY: tpl|mpack|nanopb
COMMAND: benchmark_test [TEST_NUMBER]
         no_socket
         array_test [NUMBER]
         server PORT
         client HOST PORT
# e.g.
./serialize_demo 1 nanopb server 8888
./serialize_demo 1 mpack client "127.0.0.1" 8888 (need server exist)
./serialize_demo 1 tpl no_socket
./serialize_demo 1 tpl array_test
./serialize_demo 0 mpack benchmark_test 10000
```

```mermaid
graph TD;

    classDef socket fill:#CCC,color:#888,font-weight:bold,stroke-width:2px,stroke:#CCC
    source[wifi_softap_info_t source]-->|library encode| buffer1[*buffer + buffer_size];
    buffer1-->|client|socket_send[Socket send]:::socket;
    buffer1-->|no socket|buffer2;
    socket_send-->|socket communication|socket_receive[Socket receive]:::socket;
    socket_receive[Socket receive]-->|server|buffer2[*buffer + buffer_size]
    buffer2-->|library decode|result[wifi_softap_info_t result]
```

## Demo structure
- Target structure: `wifi_softap_info_t`
```c
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
```

### socket function
```c
/*
 * socket_send
 *  - host: IP or hostname (we use inet_pton for simplicity; pass IP string)
 *  - portstr: decimal port string
 *  - buffer, size: payload to send
 *  - return 0 on success, -1 on failure
 */
static int socket_send(const char* host, const char* portstr, void* buffer, size_t size);

/*
 * socket_receive
 *  - portstr: port to listen
 *  - buffer: pointer to malloc'd buffer containing payload (returned)
 *  - size: payload size returned
 *  - returns 0 on success, -1 on failure
 *
 * Note: this function accepts one client connection and returns its payload.
 */
static int socket_receive(const char* portstr, void** buffer, size_t* size);
```

### encode / decode single structure
```c
/* encode the wifi_softap_info_t struct 
 * library: "tpl", "mpack", "nanopb"
 * out_buf, out_size: output buffer and size
 * returns 0 on success
*/
static int encode(char* library, wifi_softap_info_t* info, void** out_buf, size_t* out_size);

/* decode the wifi_softap_info_t struct 
 * library: "tpl", "mpack", "nanopb"
 * buf, sz: input buffer and size
 * out_info: output struct
 * returns 0 on success
*/
static int decode(char* library, void* buf, size_t sz, wifi_softap_info_t* out_info);
```

### encode / decode structure array
```c
/* encode array of wifi_softap_info_t structs
 * library: "tpl", "mpack", "nanopb"
 * infos: input array of structs
 * count: number of structs
 * out_buf, out_size: output buffer and size
 * returns 0 on success
 */
static int encode_array(char* library, const wifi_softap_info_t* infos, int count, void** out_buf, size_t* out_size);

/* decode array of wifi_softap_info_t structs
 * library: "tpl", "mpack", "nanopb"
 * buf, sz: input buffer and size
 * out_infos: output array of structs
 * out_count: number of structs decoded
 * returns 0 on success
 */
static int decode_array(char* library, void* buf, size_t sz, wifi_softap_info_t** out_infos, int* out_count);
```
