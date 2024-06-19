cmake_minimum_required(VERSION 3.4.1)
#add_definitions(-DNDEBUG -D_USRDLL -DMDNS_DEBUGMSGS=0 -DWIN32_LEAN_AND_MEAN -DUSE_TCP_LOOPBACK -D_CRT_SECURE_NO_DEPRECATE -D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1 -DNOT_HAVE_SA_LEN)
add_definitions(-D_USRDLL -DMDNS_DEBUGMSGS=0 -DWIN32_LEAN_AND_MEAN -DUSE_TCP_LOOPBACK -D_CRT_SECURE_NO_DEPRECATE -D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1 -DNOT_HAVE_SA_LEN)
set(RAOP_LIB_PATH    ${CMAKE_CURRENT_SOURCE_DIR}/../lib)
add_subdirectory(${RAOP_LIB_PATH} build_raop)

add_executable( raopserver_ex airplay_server.cpp )

target_link_libraries( raopserver_ex
        raopserver
	)