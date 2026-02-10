FROM alpine:3.18 AS build

RUN apk add --no-cache \
    cargo \
    cmake \
    curl-dev \
    g++ \
    gcc \
    libpng-dev \
    libwebsockets-dev \
    mariadb-dev \
    ninja \
    python3 \
    sqlite-dev \
    git \
    make \
    pkgconfig

COPY . /tw/sources

RUN cd /tw/sources && git submodule update --init --recursive

WORKDIR /tw/build

RUN cmake -S /tw/sources -B /tw/build \
    -Wno-dev \
    -DCMAKE_INSTALL_PREFIX=/tw/install \
    -DCMAKE_BUILD_TYPE=Release \
    -DANTIBOT=OFF \
    -DAUTOUPDATE=OFF \
    -DCLIENT=OFF \
    -DDEV=OFF \
    -DIPO=ON \
    -DMYSQL=ON \
    -DPREFER_BUNDLED_LIBS=OFF \
    -DVIDEORECORDER=OFF \
    -DVULKAN=OFF \
    -DWEBSOCKETS=ON \
    -GNinja

RUN cmake --build /tw/build --target install -- -j"$(nproc)"

FROM alpine:3.18 AS ddnet

RUN apk add --no-cache \
    libcurl \
    libstdc++ \
    libwebsockets \
    mariadb-connector-c \
    sqlite-libs \
    ca-certificates

RUN addgroup -S -g 5120 ddnet-insta && adduser -S -G ddnet-insta -u 5120 ddnet-insta

COPY --from=build /tw/install/bin /tw/bin
COPY --from=build /tw/install/share/ddnet/data/maps /tw/data/maps

RUN chown -R ddnet-insta:ddnet-insta /tw /entrypoint.sh || true && \
    mkdir -p /home/ddnet-insta && chown -R ddnet-insta:ddnet-insta /home/ddnet-insta

ENV HOME=/home/ddnet-insta

USER ddnet-insta

WORKDIR /tw/data
ENTRYPOINT ["/tw/bin/DDNet-Server"]
