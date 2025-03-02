#!/bin/sh

cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1
cmake --build ./build
exec sudo taskset -c 0 $(pwd)/build/http_server_c-httpd  "$@"
