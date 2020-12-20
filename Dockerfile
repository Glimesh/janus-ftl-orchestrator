FROM ubuntu:20.04

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get -y install build-essential g++-10 git libssl-dev pkg-config python3-pip ninja-build && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 100 --slave /usr/bin/g++ g++ /usr/bin/g++-10 --slave /usr/bin/gcov gcov /usr/bin/gcov-10

RUN pip3 install meson

WORKDIR /app

COPY . /app

RUN meson build/ && \
    ninja -C build/

# Orchestrator SSL API
EXPOSE 8085/tcp

CMD exec ./build/janus-ftl-orchestrator
