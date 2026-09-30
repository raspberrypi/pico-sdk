#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

#define NO_SYS                      1
#define LWIP_SOCKET                 0
#define MEM_LIBC_MALLOC             0
#define MEM_ALIGNMENT               4
#define MEM_SIZE                    8000
#define MEMP_NUM_TCP_SEG            32
#define MEMP_NUM_ARP_QUEUE          10
#define PBUF_POOL_SIZE              28
#define LWIP_ARP                    1
#define LWIP_ETHERNET               1
#define LWIP_ICMP                   1
#define LWIP_RAW                    1
#define TCP_MSS                     1460
#define TCP_SND_BUF                 (2 * TCP_MSS)
#define TCP_SND_QUEUELEN            ((4 * (TCP_SND_BUF) + (TCP_MSS - 1)) / (TCP_MSS))
#define LWIP_NETIF_STATUS_CALLBACK  1
#define LWIP_NETIF_LINK_CALLBACK    1
#define LWIP_NETIF_HOSTNAME         1
#define LWIP_NETCONN                0
#define MEM_STATS                   1
#define SYS_STATS                   1
#define MEMP_STATS                  1
#define LINK_STATS                  0
#define LWIP_CHKSUM_ALGORITHM       3
#define LWIP_DHCP                   1
#define LWIP_TCP                    1
#define LWIP_UDP                    1
#define LWIP_DNS                    1
#define LWIP_TCP_KEEPALIVE          1
#define LWIP_NETIF_TX_SINGLE_PBUF   1
#define DHCP_DOES_ARP_CHECK         0
#define LWIP_DHCP_DOES_ACD_CHECK    0

#if NO_IPV4
#define LWIP_IPV4                   0
#else
#define LWIP_IPV4                   1
#endif
#if NO_IPV6 || PPP_SUPPORT
#define LWIP_IPV6                   0
#else
#define LWIP_IPV6                   1
#endif

#ifndef NDEBUG
#define LWIP_DEBUG                  1
#define LWIP_STATS                  0
#define LWIP_STATS_DISPLAY          0
#endif

#define ETHARP_DEBUG                LWIP_DBG_OFF
#define NETIF_DEBUG                 LWIP_DBG_OFF
#define PBUF_DEBUG                  LWIP_DBG_OFF
#define API_LIB_DEBUG               LWIP_DBG_OFF
#define API_MSG_DEBUG               LWIP_DBG_OFF
#define SOCKETS_DEBUG               LWIP_DBG_OFF
#define ICMP_DEBUG                  LWIP_DBG_OFF
#define INET_DEBUG                  LWIP_DBG_OFF
#define IP_DEBUG                    LWIP_DBG_OFF
#define IP_REASS_DEBUG              LWIP_DBG_OFF
#define RAW_DEBUG                   LWIP_DBG_OFF
#define MEM_DEBUG                   LWIP_DBG_OFF
#define MEMP_DEBUG                  LWIP_DBG_OFF
#define SYS_DEBUG                   LWIP_DBG_OFF
#define TCP_DEBUG                   LWIP_DBG_OFF
#define TCP_INPUT_DEBUG             LWIP_DBG_OFF
#define TCP_OUTPUT_DEBUG            LWIP_DBG_OFF
#define TCP_RTO_DEBUG               LWIP_DBG_OFF
#define TCP_CWND_DEBUG              LWIP_DBG_OFF
#define TCP_WND_DEBUG               LWIP_DBG_OFF
#define TCP_FR_DEBUG                LWIP_DBG_OFF
#define TCP_QLEN_DEBUG              LWIP_DBG_OFF
#define TCP_RST_DEBUG               LWIP_DBG_OFF
#define UDP_DEBUG                   LWIP_DBG_OFF
#define TCPIP_DEBUG                 LWIP_DBG_OFF
#define PPP_DEBUG                   LWIP_DBG_OFF
#define SLIP_DEBUG                  LWIP_DBG_OFF
#define DHCP_DEBUG                  LWIP_DBG_OFF

/* TCP_WND must be strictly greater than the maximum raw TLS record size.
   A 16384-byte plaintext TLS record expands to ~16413 raw bytes (+ header/nonce/auth tag).
   Setting TCP_WND == 16384 deadlocks: the server fills the window before mbedTLS can
   complete the record, so altcp_recved is never called and rcv_wnd stays at 0.
   Use 2x to allow one record in flight while the previous one is being acked. */
#define TCP_WND  32768

#define LWIP_ALTCP 1
#define LWIP_ALTCP_TLS           1
#define LWIP_ALTCP_TLS_MBEDTLS   1

// Pi Connect holds HTTP requests open for a long time (SSE event stream,
// pinged once a minute, and large OTA downloads on slow links), so raise the
// lwIP HTTP client's idle timeout from the 15 s default to 120 s (units of
// HTTPC_POLL_INTERVAL, 500 ms).
#define HTTPC_POLL_TIMEOUT       240
// Pi Connect signs its request headers, so the client must not add its own
// Accept header, and the event stream (server-sent events) needs the server
// to keep the connection open. pico_rpi_connect refuses to build without these.
#define HTTPC_SEND_ACCEPT_HEADER    0
#define HTTPC_SEND_CONNECTION_CLOSE 0

// Note bug in lwip with LWIP_ALTCP and LWIP_DEBUG
// https://savannah.nongnu.org/bugs/index.php?62159
#define LWIP_DEBUG 1
//#undef LWIP_DEBUG
#define ALTCP_MBEDTLS_DEBUG  LWIP_DBG_ON
#define ALTCP_MBEDTLS_LIB_DEBUG LWIP_DBG_ON

#endif
