cmake_minimum_required(VERSION 3.4.1)

set(RAOP_LIB_PATH    ${CMAKE_CURRENT_SOURCE_DIR}/../../../lib)
add_subdirectory(${RAOP_LIB_PATH} build_raop)

add_library( raopserver_android
        SHARED
        src/main/cpp/jni_raop_server.cpp
        )

find_library( log-lib
              log )

target_link_libraries(
        raopserver_android
        ${log-lib}
        airplay)