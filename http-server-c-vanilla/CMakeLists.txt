cmake_minimum_required(VERSION 3.16)

project(http_server_c)

file(GLOB_RECURSE SOURCE_FILES src/*.c src/*.h)
file(GLOB_RECURSE LIB_SOURCE_FILES lib/*.c lib/*.h)

set(CMAKE_CXX_STANDARD 23) # Correctly set the C++ standards
set(CMAKE_C_STANDARD 11) # Correctly set the C standard

# Add vcpkg toolchain file
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
endif()

find_package(unofficial-http-parser CONFIG REQUIRED)

# Include the lib directory for header files
include_directories(${CMAKE_SOURCE_DIR}/lib)

# Include the http-parser directory for header files
include_directories(${unofficial-http-parser_INCLUDE_DIRS})

add_library(thread_pool STATIC lib/thread_pool.c)

add_library(connection_handler STATIC lib/connection_handler.c)

# Link the http-parser library to the connection_handler library
target_link_libraries(connection_handler PRIVATE unofficial::http_parser::http_parser)

add_executable(http_server_c ${SOURCE_FILES})

target_link_libraries(http_server_c PRIVATE pthread unofficial::http_parser::http_parser thread_pool connection_handler)

# Add an option to toggle the use of http-parser
option(USE_HTTP_PARSER "Set to ON to use http-parser, OFF for manual parsing" OFF)

# Pass the option as a compile definition
if(USE_HTTP_PARSER)
    target_compile_definitions(connection_handler PRIVATE USE_HTTP_PARSER=1)
else()
    target_compile_definitions(connection_handler PRIVATE USE_HTTP_PARSER=0)
endif()