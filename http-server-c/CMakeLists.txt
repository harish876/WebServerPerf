cmake_minimum_required(VERSION 3.16)

project(http-server-c)

file(GLOB_RECURSE SOURCE_FILES src/*.c src/*.h)

set(CMAKE_CXX_STANDARD 23) # Correctly set the C++ standards
set(CMAKE_C_STANDARD 11) # Correctly set the C standard

# Add vcpkg toolchain file
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
endif()

find_package(unofficial-http-parser CONFIG REQUIRED)

add_executable(http-server ${SOURCE_FILES})
target_link_libraries(http-server PRIVATE pthread unofficial::http_parser::http_parser)