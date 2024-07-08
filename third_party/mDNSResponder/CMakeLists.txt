cmake_minimum_required(VERSION 3.19)

# Embedded，default ON
option(Embed "Compile for embedded system" ON)

# print Embedded
message(STATUS "Embedded: ${Embed}")

if(WIN32)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNDEBUG -D_USRDLL -DMDNS_DEBUGMSGS=0 -DWIN32_LEAN_AND_MEAN -DUSE_TCP_LOOPBACK -D_CRT_SECURE_NO_DEPRECATE -D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1 -DNOT_HAVE_SA_LEN")

    include_directories(mDNSCore mDNSShared)
    add_library( dns_sd
            STATIC
            mDNSWindows/DLL/dllmain.c
            mDNSShared/DebugServices.c
            mDNSShared/dnssd_clientlib.c
            mDNSShared/dnssd_clientstub.c
            mDNSShared/dnssd_ipc.c
            mDNSShared/GenLinkedList.c)
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -g -fno-strict-aliasing -D_GNU_SOURCE -DHAVE_IPV6 -DHAVE_LINUX -DNOT_HAVE_SA_LEN")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS} -DPLATFORM_NO_RLIMIT -DTARGET_OS_LINUX -DUSES_NETLINK -DMDNS_DEBUGMSGS=0")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS} -W -Wall -Wextra -Wno-array-bounds -Wno-pointer-sign -Wno-unused")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS} -Wno-unused-parameter -Werror=implicit-function-declaration")

    if(Embed)
        add_definitions(-DSO_REUSEADDR -DUNICAST_DISABLED -DMDNS_VERSIONSTR_NODTS=1 -DAUTO_CALLBACKS=1 -DEMBEDDED)
    else()
        add_definitions(-DMDNS_UDS_SERVERPATH="/dev/socket/mdnsd" )
        add_definitions(-DMDNS_USERNAME="mdnsr" )
    endif() #Embedded

    include_directories(mDNSPosix mDNSCore mDNSShared)
    find_library(log-lib log)
    if(Embed)
        add_library( dns_sd
                    SHARED
                mDNSCore/mDNS.c
                mDNSCore/DNSDigest.c
                mDNSCore/uDNS.c
                mDNSPosix/mDNSPosix.c
                mDNSPosix/mDNSUNP.c
                mDNSShared/mDNSDebug.c
                mDNSShared/dnssd_clientlib.c
                mDNSShared/dnssd_clientshim.c
                mDNSShared/dnssd_ipc.c
                mDNSShared/GenLinkedList.c
                mDNSShared/PlatformCommon.c
                mDNSCore/DNSCommon.c
                mDNSPosix/PosixDaemon.c
                mDNSShared/uds_daemon.c
                JNISupport.c)
    else()
        add_library( dns_sd
                SHARED
            mDNSShared/dnssd_clientlib.c
            mDNSShared/dnssd_clientstub.c
            mDNSShared/dnssd_ipc.c
            JNISupport.c)
    endif()

    target_link_libraries(dns_sd ${log-lib})
endif()

target_include_directories(dns_sd
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/mDNSShared>)