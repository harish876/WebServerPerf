#!/bin/sh

cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DUSE_HTTP_PARSER=OFF
cmake --build ./build
exec $(pwd)/build/http_server_c  "$@"
