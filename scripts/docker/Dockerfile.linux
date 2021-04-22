FROM ubuntu:focal as taisei_builder

ENV LANG C.UTF-8
ENV DEBIAN_FRONTEND=noninteractive

RUN apt update && apt install -y -qq build-essential libsdl2-dev libogg-dev libopusfile-dev libpng-dev libzip-dev libx11-dev libwayland-dev python3-docutils libwebp-dev libfreetype6-dev python3-pip libzstd-dev git

RUN pip3 install meson==0.55.3 ninja zstandard

FROM taisei_builder

WORKDIR /usr/src/taisei

COPY . .

RUN make all

RUN mkdir -p /usr/src/taisei

