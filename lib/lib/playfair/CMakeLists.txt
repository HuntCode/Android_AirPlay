cmake_minimum_required(VERSION 3.4.1)

file(GLOB DIR_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/*c ${CMAKE_CURRENT_SOURCE_DIR}/*h)

include_directories(.)

add_library( playfair
        STATIC
        ${DIR_SRCS})