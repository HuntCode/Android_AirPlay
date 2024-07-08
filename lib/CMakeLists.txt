cmake_minimum_required(VERSION 3.4.1)

#add_definitions(-DHAVE_LIBDL )
add_definitions(-DHAS_HH_RENDERER )

set(distribution_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../distribution)

set(MDNS_LIB_PATH    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/mDNSResponder)
set(FDK_AAC_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/fdk-aac)
set(RAPIDJSON_LIB_PATH    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/rapidjson)

add_subdirectory(${MDNS_LIB_PATH} build_mdns)
add_subdirectory(${FDK_AAC_LIB_PATH} fdk_out)

include_directories(
        ${FDK_AAC_LIB_PATH}/libAACdec/include
        ${FDK_AAC_LIB_PATH}/libAACenc/include
        ${FDK_AAC_LIB_PATH}/libFDK/include
        ${FDK_AAC_LIB_PATH}/libMpegTPDec/include
        ${FDK_AAC_LIB_PATH}/libMpegTPEnc/include
        ${FDK_AAC_LIB_PATH}/libPCMutils/include
        ${FDK_AAC_LIB_PATH}/libSBRdec/include
        ${FDK_AAC_LIB_PATH}/libSBRenc/include
        ${FDK_AAC_LIB_PATH}/libSYS/include
        #${FDK_AAC_LIB_PATH}/libArithCoding/include
        #${FDK_AAC_LIB_PATH}/libDRCdec/include
        #${FDK_AAC_LIB_PATH}/libSACdec/include
        #${FDK_AAC_LIB_PATH}/libSACenc/include
        ${MDNS_LIB_PATH}/mDNSPosix
        ${MDNS_LIB_PATH}/mDNSCore
        ${MDNS_LIB_PATH}/mDNSShared
		${CMAKE_CURRENT_SOURCE_DIR}/common
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/llhttp
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/playfair
        ${CMAKE_CURRENT_SOURCE_DIR}/lib
        ${CMAKE_CURRENT_SOURCE_DIR}/renderers
        ${distribution_DIR}/plist/include
        ${distribution_DIR}/openssl/include
        ${RAPIDJSON_LIB_PATH}/include
        )

set(LIB_LLHTTP_SOURCES ${LIB_LLHTTP_SOURCES}
        lib/llhttp/api.c
        lib/llhttp/http.c
        lib/llhttp/llhttp.c
        )

set(LIB_PLAYFAIR_SOURCES ${LIB_PLAYFAIR_SOURCES}
        lib/playfair/hand_garble.c
        lib/playfair/modified_md5.c
        lib/playfair/omg_hax.c
        lib/playfair/playfair.c
        lib/playfair/sap_hash.c
        )

set(LIB_SOURCES ${LIB_SOURCES}
        lib/byteutils.c
        lib/crypto.c
        lib/dnssd.c
        lib/fairplay_playfair.c
        lib/http_request.c
        lib/http_response.c
        lib/httpd.c
        lib/logger.c
        lib/mirror_buffer.c
        lib/netutils.c
        lib/pairing.c
        lib/raop.c
        lib/raop_buffer.c
        lib/raop_ntp.c
        lib/raop_rtp.c
        lib/raop_rtp_mirror.c
        lib/utils.c
        )

set(LIB_SOURCES ${LIB_SOURCES}
        renderers/audio_renderer_hh.cpp
        renderers/video_renderer_hh.cpp
        )

set(AIRPLAY_SOURCES
        ${LIB_LLHTTP_SOURCES}
        ${LIB_PLAYFAIR_SOURCES}
        ${LIB_SOURCES}
        ${RENDERERS_SOURCES}
        airplay.cpp
        MDNSClient.cpp
        HHMDNSDriver.cpp
        )

if(WIN32)
    add_library( raopserver
            STATIC
            ${DIR_SRCS}
            )
else()
    add_library( airplay
            SHARED
            ${AIRPLAY_SOURCES}
            )
endif()

add_library(plist SHARED IMPORTED)
set_target_properties(
        plist
        PROPERTIES
        IMPORTED_LOCATION ${distribution_DIR}/plist/lib/${ANDROID_ABI}/libplist.so)

add_library(crypto STATIC IMPORTED)
set_target_properties(
        crypto
        PROPERTIES
        IMPORTED_LOCATION ${distribution_DIR}/openssl/lib/${ANDROID_ABI}/libcrypto.a)

add_library(ssl STATIC IMPORTED)
set_target_properties(
        ssl
        PROPERTIES
        IMPORTED_LOCATION ${distribution_DIR}/openssl/lib/${ANDROID_ABI}/libssl.a)

#add_library(dns_sd SHARED IMPORTED)
#set_target_properties(
#        dns_sd
#        PROPERTIES
#        IMPORTED_LOCATION ${distribution_DIR}/mdns/lib/${ANDROID_ABI}/libjdns_sd.so)

if(ANDROID)
    find_library( log-lib
            log )
    target_link_libraries( airplay
            crypto
			ssl
            plist
            fdk-aac
            dns_sd
            ${log-lib})
else()
    find_library(WS2_32_LIBRARY ws2_32)
    target_link_libraries( raopserver
            crypto
            curve25519
            ed25519
            playfair
            plist
            fdk-aac
            dns_sd
            ws2_32)
endif()


target_include_directories(airplay
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)