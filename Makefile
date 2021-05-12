# Taisei Project Makefile

###########
# helpers #
###########

setup/check/prefix:
ifdef TAISEI_PREFIX
	@echo "TAISEI_PREFIX is set to: $(TAISEI_PREFIX)"
	$(eval PREFIX=--prefix=$(TAISEI_PREFIX))
else
	@echo "To set the install path of Taisei Project when using 'make install', you can export TAISEI_PREFIX to a directory of your choice."
	@echo "i.e: [ TAISEI_PREFIX=/home/user/bin/taisei make install ] -- or --  [ export TAISEI_PREFIX=/home/user/bin/taisei && make install ] "
endif

setup/check/angle:
ifndef LIBEGL
	$(error LIBEGL path is not set)
endif
ifndef LIBGLES
	$(error LIBGLES path is not set)
endif

###########
# general #
###########

setup: setup/check/prefix
	meson setup build/ \
		-Ddeprecation_warnings=no-error \
		-Dwerror=true \
		$(PREFIX)

setup/developer:
	meson configure build/ \
		-Ddeveloper=true

setup/build-speed:
	meson configure build/ \
		-Db_lto=false \
		-Dstrip=false

setup/debug:
	meson configure build/ \
		-Dbuildtype=debug \
		-Db_ndebug=false

setup/install-relative:
	meson configure build/ \
 		-Dinstall_relative=true

setup/debug-asan:
	meson configure build/
		-Db_sanitize=address,undefined

setup/fallback:
	meson configure build/
		--wrap-mode=forcefallback \

setup/gles/angle:
	meson configure build/ \
		-Dinstall_angle=true \
		-Dangle_libgles=$(LIBGLES) \
		-Dangle_libegl=$(LIBEGL)

setup/gles/20: setup/check/angle
	meson configure build/ \
		-Dr_gles20=true \
		-Dshader_transpiler=true \
		-Dr_default=gles20

setup/gles/30: setup/check/angle
	meson configure \
		-Dr_gles30=true \
		-Dshader_transpiler=true \
		-Dr_default=gles30

compile:
	meson compile -C build/

install:
	meson install -C build/

zip:
	ninja zip -C build/

clean:
	rm -rf build/

submodule-update:
	git submodule update --init --recursive

all: clean setup compile install

all/debug: clean setup setup/developer setup/debug setup/build-speed compile install

#########
# linux #
#########

linux/setup:
	meson setup build/linux \
		--native-file misc/ci/linux-x86_64-build-release.ini \
		$(PREFIX)

linux/compile:
	meson compile -C build/linux

linux/txz:
	ninja xz -C build/linux

linux/install:
	meson install -C build/linux

linux/all: clean linux/setup linux/compile linux/install

###########
# windows #
###########

# note: for building on Linux, not directly on Windows
windows/setup: setup/check/prefix
	meson setup build/windows \
		--cross-file misc/ci/windows-llvm_mingw-x86_64-build-release.ini \
		$(PREFIX)

windows/compile:
	meson compile -C build/windows

windows/zip:
	ninja zip -C build/windows

windows/installer:
	ninja nsis -C build/windows

windows/all: clean windows/setup windows/installer

#########
# macos #
#########

macos/setup:
	meson setup build/mac \
		--native-file misc/ci/macos-x86_64-build-release.ini \
		$(PREFIX)

macos/compile:
	meson compile -C build/mac

macos/dmg:
	ninja dmg -C build/mac

macos/install:
	meson install -C build/mac

macos/all: clean macos/setup macos/compile macos/install

##########
# switch #
##########

switch/setup:
	mkdir -p build/nx
	switch/crossfile.sh > build/nx/crossfile.txt
	meson setup build/nx \
		--cross-file="build/nx/crossfile.txt" \
		$(PREFIX)

switch/compile:
	meson compile -C build/nx

switch/zip:
	ninja zip -C build/nx

switch/all: clean switch/setup switch/compile switch/zip
