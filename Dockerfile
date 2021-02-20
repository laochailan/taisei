FROM taisei_builder

WORKDIR /usr/src/taisei

COPY . .

RUN make all

RUN mkdir -p /usr/src/taisei

