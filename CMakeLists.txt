cmake_minimum_required(VERSION 3.25)
project(NUVIO C)

# Build through xenosworks/xsdk's cross-compiler. A normal PC compiler cannot
# create a native Xbox 360 executable.
if(NOT XSDK)
  message(FATAL_ERROR "Use -DCMAKE_TOOLCHAIN_FILE=<xsdk>/cmake/xsdk.cmake")
endif()

set(MBEDTLS_XENON_DIR "" CACHE PATH "Path to the mbedtls-xenon fork")
set(CURL_XENON_DIR "" CACHE PATH "Path to the curl-xenon fork")
if(NOT EXISTS "${MBEDTLS_XENON_DIR}/CMakeLists.txt" OR
   NOT EXISTS "${CURL_XENON_DIR}/CMakeLists.txt")
  message(FATAL_ERROR "Set MBEDTLS_XENON_DIR and CURL_XENON_DIR")
endif()

set(ENABLE_PROGRAMS OFF CACHE BOOL "" FORCE)
set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory("${MBEDTLS_XENON_DIR}" "${CMAKE_BINARY_DIR}/deps/mbedtls")

set(MBEDTLS_INCLUDE_DIR "${MBEDTLS_XENON_DIR}/include" CACHE PATH "" FORCE)
set(MBEDTLS_LIBRARY mbedtls CACHE STRING "" FORCE)
set(MBEDX509_LIBRARY mbedx509 CACHE STRING "" FORCE)
set(MBEDCRYPTO_LIBRARY mbedcrypto CACHE STRING "" FORCE)
set(CURL_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
add_subdirectory("${CURL_XENON_DIR}" "${CMAKE_BINARY_DIR}/deps/curl")

add_executable(NUVIO
  src/main.c
  src/stremio.c
  src/net.c
  third_party/cjson/cJSON.c
)
target_include_directories(NUVIO PRIVATE src third_party/cjson)
target_link_libraries(NUVIO PRIVATE CURL::libcurl mbedtls)
set_target_properties(NUVIO PROPERTIES C_STANDARD 17 C_EXTENSIONS ON)
target_compile_options(NUVIO PRIVATE -Wall -Wextra)
xsdk_add_xex(NUVIO TITLE_ID 0x584E5354 NAME "NUVIO.XEX" VERSION 0.3.0.0)

# Mozilla's certificate authority bundle for verified HTTPS connections.
configure_file("${MBEDTLS_XENON_DIR}/programs/xsdk/cacert.pem" cacert.pem COPYONLY)
