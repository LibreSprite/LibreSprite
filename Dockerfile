# syntax=docker/dockerfile:1

########## Stage 1: build ##########
FROM debian:bookworm AS build

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    cmake \
    git \
    ninja-build \
    pkg-config \
    libarchive-dev \
    libcurl4-openssl-dev \
    libfreetype6-dev \
    libgif-dev \
    libgtest-dev \
    libjpeg-dev \
    libpixman-1-dev \
    libpng-dev \
    libsdl2-dev \
    libsdl2-image-dev \
    libtinyxml2-dev \
    libwebp-dev \
    libx11-dev \
    libxcursor-dev \
    python3-pip \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/* \
    && pip3 install --break-system-packages --no-cache-dir cmake

WORKDIR /src

COPY . .

RUN git submodule update --init --recursive

RUN mkdir -p /src/build && cd /src/build \
    && cmake -G Ninja -DCMAKE_BUILD_TYPE=Release .. \
    && ninja libresprite \
    && DESTDIR=/staging ninja install

########## Stage 2: runtime ##########
FROM debian:bookworm-slim AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    libarchive13 \
    libcurl4 \
    libdecor-0-0 \
    libfontconfig1 \
    libfreetype6 \
    libgif7 \
    libgl1 \
    libgomp1 \
    libicu72 \
    libjpeg62-turbo \
    libpng16-16 \
    libpixman-1-0 \
    libsdl2-2.0-0 \
    libsdl2-image-2.0-0 \
    libtinyxml2-9 \
    libwayland-client0 \
    libwebp7 \
    libx11-6 \
    libxkbcommon0 \
    libxcursor1 \
    libxext6 \
    libxfixes3 \
    libxi6 \
    libxpm4 \
    libxrandr2 \
    libxrender1 \
    libxxf86vm1 \
    zlib1g \
    fonts-dejavu-core \
    xvfb \
    x11vnc \
    novnc \
    python3-websockify \
    && rm -rf /var/lib/apt/lists/*

# /work and /scripts are the MCP mount points; /tmp/.X11-unix and /home/libre
# are 1777/0777 because MCP calls run with --user $(id -u):$(id -g), which is
# an arbitrary uid with no passwd entry.
RUN useradd --create-home --uid 1000 libre \
    && mkdir -p /workspace /work /scripts /tmp/.X11-unix \
    && chown -R libre:libre /workspace /work /scripts /home/libre \
    && chmod 1777 /tmp/.X11-unix \
    && chmod 0777 /home/libre

COPY --from=build /staging/usr/local/bin/libresprite /opt/libresprite/
COPY --from=build /staging/usr/local/share/libresprite/data /opt/libresprite/data

COPY docker/entrypoint.sh /usr/local/bin/entrypoint.sh
RUN chmod +x /usr/local/bin/entrypoint.sh

USER libre
WORKDIR /workspace

# LS_MODE=batch keeps the VNC stack out of the way; docker-compose overrides it
# to gui for the interactive profile. See docker/entrypoint.sh.
ENV DISPLAY=:99 \
    LS_MODE=batch \
    HOME=/home/libre
EXPOSE 5900 6080

ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
