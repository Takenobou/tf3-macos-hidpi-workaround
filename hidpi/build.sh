#!/bin/sh
set -eu
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
build_output=$(mktemp "$script_dir/libhidpi.build.XXXXXX")
# Publish a fresh file only after signing, so rebuilding does not overwrite a
# library that a running game may still have mapped into memory.
xcrun clang -arch arm64 -dynamiclib -Wall -Wextra -Werror -Os \
    -install_name @rpath/libhidpi.dylib \
    -undefined dynamic_lookup "$script_dir/hidpi.c" \
    -o "$build_output"
codesign --force --sign - "$build_output"
mv -f "$build_output" "$script_dir/libhidpi.dylib"
