# Janus FTL Orchestrator

This is a work-in-progress service meant to manage multiple instances of [Janus](https://github.com/meetecho/janus-gateway) running the [Janus FTL Plugin](https://github.com/Glimesh/janus-ftl-plugin).

By orchestrating many Janus instances with this service, load from ingest and viewers can be distributed and greater scale can be achieved.

These are early days, so we're still in the process of standing up the architecture and establishing basic connections.

# Protocol

Please see [PROTOCOL.md](/docs/PROTOCOL.md)

# Dependencies

These are available in Ubuntu's package repos:

- `libspdlog-dev`
- `openssl`

### Installing Catch2

Catch2, sadly, is not available in Ubuntu's default repositories.

```sh
git clone https://github.com/catchorg/Catch2.git
cd Catch2
cmake -Bbuild -H. -DBUILD_TESTING=OFF
sudo cmake --build build/ --target install
```

# Building

```sh
meson build/
ninja -C build/
```

# Running

After building, you can fire up `janus-ftl-orchestrator` and connect to it with a pre-shared key using an openssl test client utility.

### Server

```sh
./build/janus-ftl-orchestrator
```

### OpenSSL Test Client

This command provides the default pre-shared key with the `-psk` flag. If you decide to set your own PSK with the `FTL_ORCHESTRATOR_PSK` env var, just provide the same one here.

```sh
openssl s_client -connect 127.0.0.1:8085 -psk 000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f -tls1_3 -ciphersuites TLS_AES_128_GCM_SHA256
```

After you connect, you can send arbitrary ASCII messages and see them reflected on the server. To gracefully disconnect, just type `Q` followed by a line break.

# Configuration

Configuration is achieved through environment variables.

| Environment Variable   | Supported Values | Notes             |
| :--------------------- | :--------------- | :---------------- |
| `FTL_ORCHESTRATOR_PSK` | String of arbitrary hex values (ex. `001122334455ff`) | This is the pre-shared key used to establish a secure TLS1.3 connection. |

# Testing

Tests are written with the help of [Catch2](https://github.com/catchorg/Catch2) and are located in the `test/` directory. They are built into the `janus-ftl-orchestrator-test` binary.

```sh
./build/janus-ftl-orchestrator-test
```