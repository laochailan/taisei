#!/bin/bash

set -eo pipefail

source "$DEVKITPRO/switchvars.sh"

function meson_arg_list() {
	while (( "$#" )); do
		echo -n "'$1'";
		if [ $# -gt 1 ]; then
				echo -n ",";
		fi
		shift
	done
}

function bin_path() {
	which "${TOOL_PREFIX}$1"
}

ADDITIONAL_LINK_FLAGS="-specs=$DEVKITPRO/libnx/switch.specs"

# Revert upstream MSYS2 support mitigation
CPPFLAGS="$(echo "$CPPFLAGS" | sed "s/-I /-I/g")"

cat <<DOCEND
[binaries]
c = '$(bin_path gcc)'
cpp = '$(bin_path g++)'
ar = '$(bin_path gcc-ar)'
strip = '$(bin_path strip)'
pkgconfig = '$(bin_path pkg-config)'
elf2nro = '$(which elf2nro)'
nacptool = '$(which nacptool)'

[project options]
deprecations_warnings = 'no-error'

[built-in options]
c_args = [$(meson_arg_list $CPPFLAGS $CFLAGS)]
c_link_args = [$(meson_arg_list $LDFLAGS $LIBS $ADDITIONAL_LINK_FLAGS)]
cpp_args = [$(meson_arg_list $CPPFLAGS $CXXFLAGS)]
cpp_link_args = [$(meson_arg_list $LDFLAGS $LIBS $ADDITIONAL_LINK_FLAGS)]
werror = false

[host_machine]
system = 'nx'
cpu_family = 'aarch64'
cpu = 'cortex-a57'
endian = 'little'
DOCEND
