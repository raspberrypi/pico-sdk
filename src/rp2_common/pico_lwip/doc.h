/**
 * \defgroup pico_lwip pico_lwip
 * \brief Integration/wrapper libraries for <a href="https://savannah.nongnu.org/projects/lwip/lwIP">lwIP</a>
 * the documentation for which is <a href="https://www.nongnu.org/lwip/2_1_x/index.html">here</a>.
 *
 * The main \c \b pico_lwip library itself aggregates the lwIP RAW API: \c \b pico_lwip_core, \c \b pico_lwip_core4, \c \b pico_lwip_core6, \c \b pico_lwip_api, \c \b pico_lwip_netif, \c \b pico_lwip_sixlowpan and \c \b pico_lwip_ppp.
 *
 * If you wish to run in NO_SYS=1 mode, then you can link \c \b pico_lwip along with \ref pico_lwip_nosys.
 *
 * If you wish to run in NO_SYS=0 mode, then you can link \c \b pico_lwip with (for instance) \ref pico_lwip_freertos,
 * and also link in pico_lwip_api for the additional blocking/thread-safe APIs.
 *
 * Additionally you must link in \ref pico_lwip_arch unless you provide your own compiler bindings for lwIP.
 *
 * Additional individual pieces of lwIP functionality are available à la cart, by linking any of the libraries below.
 *
 * The following libraries are provided that contain exactly the equivalent lwIP functionality groups:
 *
 * * \c \b pico_lwip_core -
 * * \c \b pico_lwip_core4 -
 * * \c \b pico_lwip_core6 -
 * * \c \b pico_lwip_netif -
 * * \c \b pico_lwip_sixlowpan -
 * * \c \b pico_lwip_ppp -
 * * \c \b pico_lwip_api -
 *
 * The following libraries are provided that contain exactly the equivalent lwIP application support:
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
 * * \c \b pico_lwip_mqtt -
 *
 */

/** \defgroup pico_lwip_arch pico_lwip_arch
 * \ingroup pico_lwip
 * \brief lwIP compiler adapters. This is not included by default in \c \b pico_lwip in case you wish to implement your own.
 */

/** \defgroup pico_lwip_http pico_lwip_http
 * \ingroup pico_lwip
 * \brief LwIP HTTP client and server library
 *
 * This library enables you to make use of the LwIP HTTP client and server library
 *
 * \par LwIP HTTP server
 *
 * To make use of the LwIP HTTP server you need to provide the HTML that the server will return to the client.
 * This is done by compiling the content directly into the executable.
 *
 * \par makefsdata
 *
 * LwIP provides a c-library tool `makefsdata` to compile your HTML into a source file for inclusion into your program.
 * This is quite hard to use as you need to compile the tool as a native binary, then run the tool to generate a source file
 * before compiling your code for the Pico device.
 *
 * \par pico_set_lwip_httpd_content
 *
 * To make this whole process easier, a python script `makefsdata.py` is provided to generate a source file for your HTML content.
 * A CMake function `pico_set_lwip_httpd_content` takes care of running the `makefsdata.py` python script for you.
 * To make use of this, specify the name of the source file as `pico_fsdata.inc` in `lwipopts.h`.
 *
 * \code
 * #define HTTPD_FSDATA_FILE "pico_fsdata.inc"
 * \endcode
 *
 * Then call the CMake function `pico_set_lwip_httpd_content` in your `CMakeLists.txt` to add your content to a library.
 * Make sure you add this library to your executable by adding it to your target_link_libraries list.
 * Here is an example from the httpd example in pico-examples.
  *
 * \code
 * pico_add_library(pico_httpd_content NOFLAG)
 * pico_set_lwip_httpd_content(pico_httpd_content INTERFACE
 *        ${CMAKE_CURRENT_LIST_DIR}/content/404.html
 *        ${CMAKE_CURRENT_LIST_DIR}/content/index.shtml
 *        ${CMAKE_CURRENT_LIST_DIR}/content/test.shtml
 *        ${CMAKE_CURRENT_LIST_DIR}/content/ledpass.shtml
 *        ${CMAKE_CURRENT_LIST_DIR}/content/ledfail.shtml
 *        ${CMAKE_CURRENT_LIST_DIR}/content/img/rpi.png
 *        )
 * \endcode
 */
