cmake_minimum_required(VERSION 3.4.1)

add_definitions(-D_CRT_SECURE_NO_WARNINGS -DWIN32_LEAN_AND_MEAN )

include_directories( playfair llhttp )

file(GLOB DIR_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/*c ${CMAKE_CURRENT_SOURCE_DIR}/*h)

add_library( airplay
        STATIC
        ${DIR_SRCS}
        )

find_library( LIBPLIST NAMES plist plist-2.0 )

if(UNIX)
target_link_libraries( airplay
	    pthread
        playfair
        llhttp
        ${LIBPLIST} )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")

target_link_libraries(airplay playfair 
							  llhttp 
							  ws2_32)

endif()

if(UNIX)

if(NOT APPLE )
  find_package(OpenSSL 1.1.1 REQUIRED)
  target_compile_definitions(airplay PUBLIC OPENSSL_API_COMPAT=0x10101000L)
  target_link_libraries( airplay OpenSSL::Crypto )
  target_link_libraries( airplay dns_sd )
else()
  include_directories( /usr/local/opt/openssl@1.1/include/ )
  target_link_libraries( airplay /usr/local/opt/openssl@1.1/lib/libcrypto.a )
endif()

endif()
        
