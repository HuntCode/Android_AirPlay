# Install script for directory: C:/A_ReserveLand/AirplayServerEX/lib

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/Project")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "0")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "C:/Users/wanglv/AppData/Local/Android/Sdk/ndk/23.1.7779620/toolchains/llvm/prebuilt/windows-x86_64/bin/llvm-objdump.exe")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("C:/A_ReserveLand/AirplayServerEX/android/example/app/.cxx/Debug/43172k4g/armeabi-v7a/build_raop/build_mdns/cmake_install.cmake")
  include("C:/A_ReserveLand/AirplayServerEX/android/example/app/.cxx/Debug/43172k4g/armeabi-v7a/build_raop/crypto/cmake_install.cmake")
  include("C:/A_ReserveLand/AirplayServerEX/android/example/app/.cxx/Debug/43172k4g/armeabi-v7a/build_raop/curve25519/cmake_install.cmake")
  include("C:/A_ReserveLand/AirplayServerEX/android/example/app/.cxx/Debug/43172k4g/armeabi-v7a/build_raop/ed25519/cmake_install.cmake")
  include("C:/A_ReserveLand/AirplayServerEX/android/example/app/.cxx/Debug/43172k4g/armeabi-v7a/build_raop/playfair/cmake_install.cmake")
  include("C:/A_ReserveLand/AirplayServerEX/android/example/app/.cxx/Debug/43172k4g/armeabi-v7a/build_raop/plist/cmake_install.cmake")
  include("C:/A_ReserveLand/AirplayServerEX/android/example/app/.cxx/Debug/43172k4g/armeabi-v7a/build_raop/fdk_out/cmake_install.cmake")

endif()

