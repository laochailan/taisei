# Taisei Docker Makefile

tag:
ifndef TAG
	@echo "tag not defined, please define one"
	@echo "i.e: TAG=20210630 make docker/switch"
	@exit 1
endif

##########
# docker #
##########

docker/windows: tag
	docker build . -m 8GB -t taiseiproject/windows-toolkit:$(TAG) -f "Dockerfile.windows"
	docker push taisei/windows-toolkit:$(TAG)

docker/switch: tag
	docker build . -m 4GB -t taiseiproject/switch-toolkit:$(TAG) -f "Dockerfile.switch"
	docker push taiseiproject/switch-toolkit:$(TAG)

docker/linux: tag
	docker build . -m 4GB -t taiseiproject/linux-toolkit:$(TAG) -f "Dockerfile.linux"
	docker push taiseiproject/linux-toolkit:$(TAG)

# cursed, and only here to preserve the madness for future generations
# builds ANGLE for Windows... eventually
docker/angle:
	@echo "To build this on Windows, you must have '"storage-opt": ["size=130GB"]' in your daemon.json for Docker"
	docker build misc/ci/ -m 12GB -t taisei-angle-builder -f "misc/ci/Dockerfile.angle" --build-arg ANGLE_VERSION=4484 # build the image, this can take 2 hours so please wait warmly
	docker run -m 16GB --name taisei-temp taisei-angle-builder:latest # start up a container that dies immediately because of no entrypoint
	docker cp taisei-temp:"C:\\GOOGLE\\angle\\out\\Release\\libGLESv2.dll" ./ # copy locally
	docker cp taisei-temp:"C:\\GOOGLE\\angle\\out\\Release\\libEGL.dll" ./ # copy locally
	docker rm taisei-temp # remove unneeded container (the image is still available as taisei-angle-builder)
