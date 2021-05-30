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

clean:
	rm -rf build/

submodule-update:
	git submodule update --init --recursive

all: clean setup compile install

all/debug: clean setup setup/developer setup/debug setup/build-speed compile install

#########
# linux #
#########

linux/setup: setup

linux/tar:
	meson compile xz -C build/

###########
# windows #
###########

windows/setup: setup/check/prefix
	meson setup build/ \
		--cross-file scripts/llvm-mingw-x86_64 \
		-Ddeprecation_warnings=no-error \
		$(PREFIX)

windows/zip:
	meson compile zip -C build/

windows/installer:
	meson compile nsis -C build/

windows/all: clean setup-windows compile install

#########
# macos #
#########

macos/setup:
	meson configure build/ \
		-Dmacos_bundle=true

macos/dmg:
	meson compile dmg -C build/

macos/all: setup/macos install/macos

##########
# switch #
##########

# TODO:

##########
# docker #
##########

docker/angle:
	@echo "To build this on Windows, you must have '"storage-opt": ["size=130GB"]' in your daemon.json for Docker"
	docker build ./scripts/docker/ -m 12GB -t taisei-angle-builder -f "scripts/docker/Dockerfile.angle" --build-arg ANGLE_VERSION=4484 # build the image, this can take 2 hours so please wait warmly
	docker run -m 4GB --name taisei-temp taisei-angle-builder:latest # start up a container that dies immediately because of no entrypoint
	docker cp taisei-temp:"C:\\GOOGLE\\angle\\out\\Release\\libGLESv2.dll" ./ # copy locally
	docker cp taisei-temp:"C:\\GOOGLE\\angle\\out\\Release\\libEGL.dll" ./ # copy locally
	docker rm taisei-temp # remove unneeded container (the image is still available as taisei-angle-builder)

####################
# internal helpers #
####################

# not intended to be run on their own, meant for CI

_docker/windows/zip:
	zip -r /opt/taisei-exe.zip /opt/taisei-exe

