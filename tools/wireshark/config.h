/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Directory for data */
#define DATAFILE_DIR "/opt/wireshark/share/wireshark"

/* Directory for docs */
#define DOC_DIR "/opt/wireshark/share/doc/wireshark"

/* Link plugins statically into Wireshark */
/* #undef ENABLE_STATIC */

/* Directory for extcap plugins */
#define EXTCAP_DIR "${datadir}/wireshark/extcap/"

/* Enable AirPcap */
#define HAVE_AIRPCAP 1

/* Define to 1 if you have the <arpa/inet.h> header file. */
#define HAVE_ARPA_INET_H 1

/* Define to 1 if you have the <arpa/nameser.h> header file. */
#define HAVE_ARPA_NAMESER_H 1

/* Define to 1 if you have the `bpf_image' function. */
#define HAVE_BPF_IMAGE 1

/* Define to 1 if you have the `CFPropertyListCreateWithStream' function. */
/* #undef HAVE_CFPROPERTYLISTCREATEWITHSTREAM */

/* Define to use c-ares library */
/* #undef HAVE_C_ARES */

/* Define to 1 if you have the <direct.h> header file. */
/* #undef HAVE_DIRECT_H */

/* Define to 1 if you have the <dirent.h> header file. */
#define HAVE_DIRENT_H 1

/* Define to 1 if you have the `dladdr' function. */
/* #undef HAVE_DLADDR */

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define if echld is enabled */
/* #undef HAVE_ECHLD */

/* Define if external capture sources should be enabled */
#define HAVE_EXTCAP 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to use GeoIP library */
#define HAVE_GEOIP 1

/* Define if GeoIP supports IPv6 (GeoIP 1.4.5 and later) */
#define HAVE_GEOIP_V6 1

/* Define to 1 if you have the `gethostbyname2' function. */
#define HAVE_GETHOSTBYNAME2 1

/* Define to 1 if you have the getopt function. */
#define HAVE_GETOPT 1

/* Define to 1 if you have the <getopt.h> header file. */
#define HAVE_GETOPT_H 1

/* Define to 1 if you have the `getprotobynumber' function. */
#define HAVE_GETPROTOBYNUMBER 1

/* Define if your printf() function supports thousands grouping. */
#define HAVE_GLIB_PRINTF_GROUPING 1

/* Define to use GNU ADNS library */
/* #undef HAVE_GNU_ADNS */

/* Define to 1 if you have the <grp.h> header file. */
#define HAVE_GRP_H 1

/* Define to 1 if -lgtkmacintegration includes the GtkOSXApplication
   Integration functions. */
/* #undef HAVE_GTKOSXAPPLICATION */

/* Define to use heimdal kerberos */
/* #undef HAVE_HEIMDAL_KERBEROS */

/* Define to 1 if the the Gtk+ framework or a separate library includes the
   Imendio IGE Mac OS X Integration functions. */
/* #undef HAVE_IGE_MAC_INTEGRATION */

/* Define to 1 if you have the inet_aton function. */
#define HAVE_INET_ATON 0

/* Define if inet_ntop() prototype exists */
#define HAVE_INET_NTOP_PROTO 1

/* Define to 1 if you have the `inflatePrime' function. */
#define HAVE_INFLATEPRIME 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `issetugid' function. */
/* #undef HAVE_ISSETUGID */

/* Define to use kerberos */
#define HAVE_KERBEROS 1

/* Define if krb5.h defines KEYTYPE_ARCFOUR_56 */
/* #undef HAVE_KEYTYPE_ARCFOUR_56 */

/* Define to 1 if you have the <lauxlib.h> header file. */
/* #undef HAVE_LAUXLIB_H */

/* Define to use the libcap library */
#define HAVE_LIBCAP 1

/* Define to use libgcrypt */
#define HAVE_LIBGCRYPT 1

/* Define to use GnuTLS library */
#define HAVE_LIBGNUTLS 1

/* Enable libnl support */
/* #undef HAVE_LIBNL */

/* libnl version 1 */
/* #undef HAVE_LIBNL1 */

/* libnl version 2 */
/* #undef HAVE_LIBNL2 */

/* libnl version 3 */
/* #undef HAVE_LIBNL3 */

/* Define to use libpcap library */
#define HAVE_LIBPCAP 1

/* Define to use libportaudio library */
/* #undef HAVE_LIBPORTAUDIO */

/* Define to 1 if you have the `smi' library (-lsmi). */
/* #undef HAVE_LIBSMI */

/* Define to use libz library */
#define HAVE_LIBZ 1

/* Define to use Lua */
/* #undef HAVE_LUA */

/* Define to 1 if you have the <lualib.h> header file. */
/* #undef HAVE_LUALIB_H */

/* Define to 1 if you have the <lua.h> header file. */
/* #undef HAVE_LUA_H */

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to use MIT kerberos */
#define HAVE_MIT_KERBEROS 1

/* Define to 1 if you have the `mkdtemp' function. */
#define HAVE_MKDTEMP 1

/* Define to 1 if you have the `mkstemp' function. */
#define HAVE_MKSTEMP 1

/* Define to 1 if you have the `mmap' function. */
#define HAVE_MMAP 1

/* Define to 1 if you have the `mprotect' function. */
#define HAVE_MPROTECT 1

/* Define to 1 if you have the <netdb.h> header file. */
#define HAVE_NETDB_H 1

/* Define to 1 if you have the <netinet/in.h> header file. */
#define HAVE_NETINET_IN_H 1

/* nl80211.h is new enough */
#define HAVE_NL80211 1

/* SET_CHANNEL is supported */
#define HAVE_NL80211_CMD_SET_CHANNEL 1

/* SPLIT_WIPHY_DUMP is supported */
#define HAVE_NL80211_SPLIT_WIPHY_DUMP 1

/* Define to 1 if you have OS X frameworks */
/* #undef HAVE_OS_X_FRAMEWORKS */

/* Define if pcap_breakloop is known */
#define HAVE_PCAP_BREAKLOOP 1

/* Define to 1 if you have the `pcap_create' function. */
#define HAVE_PCAP_CREATE 1

/* Define to 1 if you have the `pcap_createsrcstr' function. */
/* #undef HAVE_PCAP_CREATESRCSTR */

/* Define to 1 if you have the `pcap_datalink_name_to_val' function. */
#define HAVE_PCAP_DATALINK_NAME_TO_VAL 1

/* Define to 1 if you have the `pcap_datalink_val_to_description' function. */
#define HAVE_PCAP_DATALINK_VAL_TO_DESCRIPTION 1

/* Define to 1 if you have the `pcap_datalink_val_to_name' function. */
#define HAVE_PCAP_DATALINK_VAL_TO_NAME 1

/* Define to 1 if you have the `pcap_findalldevs' function and a pcap.h that
   declares pcap_if_t. */
#define HAVE_PCAP_FINDALLDEVS 1

/* Define to 1 if you have the `pcap_findalldevs_ex' function. */
/* #undef HAVE_PCAP_FINDALLDEVS_EX */

/* Define to 1 if you have the `pcap_freecode' function. */
#define HAVE_PCAP_FREECODE 1

/* Define to 1 if you have the `pcap_free_datalinks' function. */
#define HAVE_PCAP_FREE_DATALINKS 1

/* Define to 1 if you have the `pcap_get_selectable_fd' function. */
#define HAVE_PCAP_GET_SELECTABLE_FD 1

/* Define to 1 if you have the `pcap_lib_version' function. */
#define HAVE_PCAP_LIB_VERSION 1

/* Define to 1 if you have the `pcap_list_datalinks' function. */
#define HAVE_PCAP_LIST_DATALINKS 1

/* Define to 1 if you have the `pcap_open' function. */
/* #undef HAVE_PCAP_OPEN */

/* Define to 1 if you have the `pcap_open_dead' function. */
#define HAVE_PCAP_OPEN_DEAD 1

/* Define to 1 if you have WinPcap remote capturing support and prefer to use
   these new API features. */
/* #undef HAVE_PCAP_REMOTE */

/* Define to 1 if you have the `pcap_setsampling' function. */
/* #undef HAVE_PCAP_SETSAMPLING */

/* Define to 1 if you have the `pcap_set_datalink' function. */
#define HAVE_PCAP_SET_DATALINK 1

/* Define to 1 if you have the `pcap_set_tstamp_precision' function. */
#define HAVE_PCAP_SET_TSTAMP_PRECISION 1

/* Define if plugins are enabled */
#define HAVE_PLUGINS 1

/* Define to 1 if you have the <portaudio.h> header file. */
/* #undef HAVE_PORTAUDIO_H */

/* Define to 1 if you have the <pwd.h> header file. */
#define HAVE_PWD_H 1

/* Define to 1 to enable remote capturing feature in WinPcap library */
/* #undef HAVE_REMOTE */

/* Define if sa_len field exists in struct sockaddr */
/* #undef HAVE_SA_LEN */

/* Define to support playing SBC by standalone BlueZ SBC library */
/* #undef HAVE_SBC */

/* Define to 1 if you have the `setresgid' function. */
#define HAVE_SETRESGID 1

/* Define to 1 if you have the `setresuid' function. */
#define HAVE_SETRESUID 1

/* Support SSSE4.2 (Streaming SIMD Extensions 4.2) instructions */
#define HAVE_SSE4_2 1

/* Define to 1 if you have the <stdarg.h> header file. */
#define HAVE_STDARG_H 1

/* Define to 1 if you have the <stddef.h> header file. */
#define HAVE_STDDEF_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define if you have the strptime function. */
#define HAVE_STRPTIME 1

/* Define if st_flags field exists in struct stat */
/* #undef HAVE_ST_FLAGS */

/* Define to 1 if you have the `sysconf' function. */
#define HAVE_SYSCONF 1

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#define HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/param.h> header file. */
#define HAVE_SYS_PARAM_H 1

/* Define to 1 if you have the <sys/socket.h> header file. */
#define HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/sockio.h> header file. */
/* #undef HAVE_SYS_SOCKIO_H */

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/utsname.h> header file. */
#define HAVE_SYS_UTSNAME_H 1

/* Define to 1 if you have the <sys/wait.h> header file. */
#define HAVE_SYS_WAIT_H 1

/* Define if tm_zone field exists in struct tm */
#define HAVE_TM_ZONE 1

/* Define if tzname array exists */
/* #undef HAVE_TZNAME */

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define if we have xdg-open */
#define HAVE_XDG_OPEN 1

/* HTML viewer, e.g. mozilla */
#define HTML_VIEWER "xdg-open"

/* Define if the platform supports IPv6 */
#define INET6 1

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Define if we need memory allocations to be 8-byte aligned */
#define NEED_8_BYTE_ALIGNMENT 1

/* Define if inet/v6defs.h needs to be included */
/* #undef NEED_INET_V6DEFS_H */

/* Name of package */
#define PACKAGE "wireshark"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "http://bugs.wireshark.org/"

/* Define to the full name of this package. */
#define PACKAGE_NAME "wireshark"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "wireshark 1.99.1"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "wireshark"

/* Define to the home page for this package. */
#define PACKAGE_URL "http://www.wireshark.org/"

/* Define to the version of this package. */
#define PACKAGE_VERSION "1.99.1"

/* Support for pcap-ng */
#define PCAP_NG_DEFAULT 1

/* Define if we are using version of of the Portaudio library API */
/* #undef PORTAUDIO_API_1 */

/* Define if we have QtMacExtras */
/* #undef QT_MACEXTRAS_LIB */

/* Define if we have QtMultimediaWidgets */
/* #undef QT_MULTIMEDIAWIDGETS_LIB */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Version number of package */
#define VERSION "1.99.1"

/* Wireshark's major version */
#define VERSION_MAJOR 1

/* Wireshark's micro version */
#define VERSION_MICRO 1

/* Wireshark's minor version */
#define VERSION_MINOR 99

/* Support for packet editor */
#define WANT_PACKET_EDITOR 1

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

/* Define as the string to precede declarations of routines that never return
   */
#define WS_MSVC_NORETURN /**/

/* Define to 1 if `lex' declares `yytext' as a `char *' by default, not a
   `char[]'. */
/* #undef YYTEXT_POINTER */

/* Enable large inode numbers on Mac OS X 10.5.  */
#ifndef _DARWIN_USE_64_BIT_INODE
# define _DARWIN_USE_64_BIT_INODE 1
#endif

/* Number of bits in a file offset, on hosts where this is settable. */
/* #undef _FILE_OFFSET_BITS */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */

/* Hint to the compiler that a function parameters is not used */
#define _U_ __attribute__((unused))
