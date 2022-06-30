/**
 * \defgroup pico_lwip pico_lwip
 * \brief Wrapper libraries for <a href="https://savannah.nongnu.org/projects/lwip/lwIP">lwIP</a>
 *
 * The following libraries are provided that contain the equivalent lwIP functionality groups:
 *
 * * \c \b pico_lwip_core - 
 * * \c \b pico_lwip_core4 - 
 * * \c \b pico_lwip_core6 - 
 * * \c \b pico_lwip_netif -
 * * \c \b pico_lwip_sixlowpan - 
 * * \c \b pico_lwip_ppp -
 * * \c \b pico_lwip_api -
 *
 * The following libraries are provided that contain the equivalent lwIP application support:
 *
 * * \c \b pico_lwip_snmp -
 * * \c \b pico_lwip_http -
 * * \c \b pico_lwip_makefsdata - 
 * * \c \b pico_lwip_iperf - 
 * * \c \b pico_lwip_smtp - 
 * * \c \b pico_lwip_sntp - 
 * * \c \b pico_lwip_mdns - 
 * * \c \b pico_lwip_netbios - 
 * * \c \b pico_lwip_tftp - 
 * * \c \b pico_lwip_mbedtls -
 *
 * The SDK Provides a common set of functionality in \c \p pico_lwip which aggregates:
 *
 * * \c \b pico_lwip_core -
 * * \c \b pico_lwip_core4 -
 * * \c \b pico_lwip_core6 -
 * * \c \b pico_lwip_netif -
 * * \c \b pico_lwip_sixlowpan -
 * * \c \b pico_lwip_ppp -
 *
 * The following additional libraries are provided:
 *
 * * \c \b pico_lwip - Aggregates the lwIP RAW API: \c \b pico_lwip_core, \c \b pico_lwip_core4, \c \b pico_lwip_core6, \c \b pico_lwip_api, \c \b pico_lwip_netif, \c \b pico_lwip_sixlowpan and \c \b pico_lwip_ppp. It does
 * not include \c \b pico_lwip_api, which requires NO_SYS=0. You should include the latter separately if you want it.
 *
 * * \c \b pico_lwip_arch - lwIP required compiler adapters. This is not included in \c \b pico_lwip in case you wish to replace them.
 * * \c \b pico_lwip_nosys - basic stub functions for NO_SYS mode.
 */
