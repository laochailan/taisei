# Taisei Project Makefile

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

setup: setup/check/prefix
	meson setup build/ \
		--wrap-mode=nofallback \
		-Ddeprecation_warnings=no-error \
		-Dwerror=true \
		$(PREFIX)

setup-windows: setup/check/prefix
	meson setup build/ \
		--cross-file scripts/llvm-mingw-x86_64 \
		-Ddeprecation_warnings=no-error \
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

setup/all: setup setup/build-speed setup/developer setup/debug

compile:
	meson compile -C build/

install:
	meson install -C build/

clean:
	rm -rf build/

submodule-update:
	git submodule update --init --recursive

docker/windows:
	docker build -t taisei -f "scripts/docker/Dockerfile.windows"
	docker cp taisei:/opt/taisei-exe.zip ./

docker/angle:
	docker build ./scripts/docker/ -m 4GB -t taisei -f "scripts/docker/Dockerfile.angle" --build-arg ANGLE_VERSION=4484

docker/windows/zip:
	zip -r /opt/taisei-exe.zip /opt/taisei-exe

all: clean setup compile install
all-windows: clean setup-windows compile install docker/windows/zip

all/debug: clean setup setup/developer setup/debug compile install
