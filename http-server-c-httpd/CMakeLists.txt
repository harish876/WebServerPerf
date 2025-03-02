cmake_minimum_required(VERSION 3.16)

project(http_server_c-httpd)

file(GLOB_RECURSE SOURCE_FILES src/*.c src/*.h)
file(GLOB_RECURSE LIB_SOURCE_FILES lib/*.c lib/*.h)

set(CMAKE_CXX_STANDARD 23) # Correctly set the C++ standards
set(CMAKE_C_STANDARD 11) # Correctly set the C standard

# Add vcpkg toolchain file
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
else()
    set(CMAKE_TOOLCHAIN_FILE /root/vcpkg/scripts/buildsystems/vcpkg.cmake)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBMICROHTTPD REQUIRED libmicrohttpd)

# Include the lib directory for header files
include_directories(${CMAKE_SOURCE_DIR}/lib)

include(FetchContent)

# Let CMake download yyjson
FetchContent_Declare(
    yyjson
    GIT_REPOSITORY https://github.com/ibireme/yyjson.git
    GIT_TAG master # master, or version number, e.g. 0.6.0
)
FetchContent_GetProperties(yyjson)

if(NOT yyjson_POPULATED)
    FetchContent_Populate(yyjson)
    add_subdirectory(${yyjson_SOURCE_DIR} ${yyjson_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

add_executable(http_server_c-httpd
    ${SOURCE_FILES}main.c
)

target_link_libraries(http_server_c-httpd PRIVATE ${LIBMICROHTTPD_LIBRARIES} yyjson)
include_directories(${LIBMICROHTTPD_INCLUDE_DIRS})
link_directories(${LIBMICROHTTPD_LIBRARY_DIRS})