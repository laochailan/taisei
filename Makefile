# Taisei Project Makefile

.DEFAULT_GOAL := help

UNAME := $(shell uname)

###########
##@ General
###########

.PHONY: all
all: clean setup compile install ## Clean, setup, compile, and install

.PHONY: all/debug
all/debug: clean setup setup/developer setup/debug compile install ## Clean, setup, compile, and install (with developer/debug options)

.PHONY: help
help: ## Show this help prompt
	@echo ""
	@echo "☯ Taisei Project Makefile ☯"
	@awk 'BEGIN {FS = ":.*##"; printf "\nUsage: make \033[36m<command>\033[0m\n"} /^[0-9a-zA-Z_-]+[\/]?.*:.*?##/ { printf "  \033[36m%-15s\033[0m %s\n", $$1, $$2 } /^##@/ { printf "\n\033[1m%s\033[0m\n", substr($$0, 5) } ' $(MAKEFILE_LIST)

#########
##@ Setup
#########

.PHONY: setup/check/_prefix
setup/check/_prefix:
ifdef TAISEI_PREFIX
	@echo "TAISEI_PREFIX is set to: $(TAISEI_PREFIX)"
	$(eval PREFIX=--prefix=$(TAISEI_PREFIX))
else
	@echo "To set the install path of Taisei Project when using 'make install', you can export TAISEI_PREFIX to a directory of your choice."
	@echo "i.e: [ TAISEI_PREFIX=/home/user/bin/taisei make install ]  -- or --  [ export TAISEI_PREFIX=/home/user/bin/taisei && make install ] "
endif

.PHONY: setup
setup: setup/check/_prefix ## Setup the build/ directory
	meson setup build/ \
		--wrap-mode=nofallback \
		-Ddeprecation_warnings=no-error \
		-Dwerror=true \
		$(PREFIX)

.PHONY: setup/developer
setup/developer: ## Set "developer" options (see Docs for details)
	meson configure build/ \
		-Ddeveloper=true

.PHONY: setup/debug
setup/debug: ## Set debug options
	meson configure build/ \
		-Dbuildtype=debug \
		-Db_ndebug=false

.PHONY: setup/debug/asan
setup/debug/asan: ## Set advanced debug options through Address Sanitizer (see Docs for details)
	meson configure build/
		-Db_sanitize=address,undefined

.PHONY: setup/gles/30
setup/gles/30: setup/gles/angle ## Enable GLES 3.0
	meson configure \
		-Dr_gles30=true \
		-Dshader_transpiler=true \
		-Dr_default=gles30

.PHONY: setup/gles/20
setup/gles/20: setup/gles/angle ## Enable GLES 2.0
	meson configure build/ \
		-Dr_gles20=true \
		-Dshader_transpiler=true \
		-Dr_default=gles20

.PHONY: setup/gles/angle
setup/gles/angle: ## Set ANGLE library locations (necessary for Windows/macOS when using GLES 3.0/2.0)
ifneq ($(and $(LIBGLES),$(LIBEGL)),)
	meson configure build/ \
		-Dinstall_angle=true \
		-Dangle_libgles=$(LIBGLES) \
		-Dangle_libegl=$(LIBEGL)
else
ifneq ($(UNAME), Linux)
	$(error You must set LIBEGL and LIBGLES to ANGLE library binaries for non-Linux systems)
else
	@echo "LIBGLES and LIBEGL not set, building without ANGLE."
endif
endif

.PHONY: setup/submodules
setup/submodules: ## Initialize git submodules
	git submodule update --init

.PHONY: setup/fastbuild
setup/fastbuild: ## Set faster building options (no linking or stripping)
	meson configure build/ \
		-Db_lto=false \
		-Dstrip=false

.PHONY: setup/install-relative
setup/install-relative: ## Set option to install files relatively (needed for Windows)
	meson configure build/ \
 		-Dinstall_relative=true

.PHONY: setup/fallback
setup/fallback: ## Set option to use Taisei's pre-shipped system libraries (for maximum compatability)
	meson configure build/
		--wrap-mode=forcefallback \

.PHONY: setup/all
setup/all: setup setup/fastbuild setup/developer setup/debug ## Setup with fastbuild, developer, and debug settings enabled

#####################
##@ Compile & Install
#####################

.PHONY: compile
compile: ## Compile Taisei from source
	meson compile -C build/

.PHONY: install
install: ## Install to the specified TAISEI_PREFIX (if set)
	meson install -C build/

.PHONY: clean
clean: ## Delete build/ directory
	rm -rf build/

##########
##@ Docker
##########

.PHONY: docker
docker: docker/builder ## Compile Taisei within Docker (build check)
	docker build -t taisei . -f ./Dockerfile

.PHONY: docker/builder
docker/builder: ## Build the "builder" image in Docker (sets up build tools, etc)
	docker build -t taisei_builder . -f ./Dockerfile-builder
