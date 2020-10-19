/**
 * @file test-client.cpp
 * @author Hayden McAfee (hayden@outlook.com)
 * @version 0.1
 * @date 2020-10-18
 * 
 * @copyright Copyright (c) 2020 Hayden McAfee
 * 
 */

#include "Util.h"

#include <arpa/inet.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sstream>
#include <string>

int psk(SSL *ssl, const EVP_MD *md, const unsigned char **id, size_t *idlen, SSL_SESSION **sess)
{
    long keyLength = 0;
    unsigned char* key = OPENSSL_hexstr2buf("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f", &keyLength);

    spdlog::info("PSK processing with key {} / {}...", key, keyLength);
    spdlog::info("Using key {}", spdlog::to_hex(key, key + keyLength));

    spdlog::info("MD: {:p}", reinterpret_cast<const void*>(md));

    // Find the cipher we'll be using
    // identified by IANA mapping: https://testssl.sh/openssl-iana.mapping.html
    const unsigned char tls13_aes128gcmsha256_id[] = { 0x13, 0x01 };
    const SSL_CIPHER* cipher = SSL_CIPHER_find(ssl, tls13_aes128gcmsha256_id);
    if (cipher == nullptr)
    {
        spdlog::error("OpenSSL could not find cipher suite!");
        OPENSSL_free(key);
        return 0;
    }

    // Create an SSL session and set some parameters on it
    SSL_SESSION* temporarySslSession = SSL_SESSION_new();
    if (temporarySslSession == nullptr)
    {
        spdlog::error("Could not create new SSL session!");
        OPENSSL_free(key);
        return 0;
    }

    if (!SSL_SESSION_set1_master_key(temporarySslSession, key, keyLength))
    {
        spdlog::error("Could not set key on new SSL session!");
        OPENSSL_free(key);
        return 0;
    }
    OPENSSL_free(key);

    if (!SSL_SESSION_set_cipher(temporarySslSession, cipher))
    {
        spdlog::error("Could not set cipher on new SSL session!");
        return 0;
    }

    if (!SSL_SESSION_set_protocol_version(temporarySslSession, TLS1_3_VERSION))
    {
        spdlog::error("Could not set version on new SSL session!");
        return 0;
    }

    
    *sess = temporarySslSession;
    *id = reinterpret_cast<const unsigned char*>("test");
    *idlen = 4;

    return 1;
}

int main()
{
    uint16_t port = 8085;

    spdlog::info("Hello world!");

    // Let's try connecting...
    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();

    int clientSocketHandle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in serverAddr
    {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = { .s_addr = htonl(INADDR_LOOPBACK) }
    };

    spdlog::info("Connecting to port {}...", port);

    if (connect(clientSocketHandle, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0)
    {
        int error = errno;
        std::stringstream errStr;
        errStr << "Unable to connect! Error "
            << error << ": " << Util::ErrnoToString(error);
        throw std::runtime_error(errStr.str());
    }

    spdlog::info("Connected!");

    SSL_CTX* sslContext = SSL_CTX_new(TLS_client_method());
    if (sslContext == nullptr)
    {
        throw std::runtime_error("Error creating SSL context");
    }

    // Set up PSK
    SSL_CTX_set_psk_use_session_callback(sslContext, psk);

    SSL* ssl = SSL_new(sslContext);
    if (ssl == nullptr)
    {
        throw std::runtime_error("Error creating SSL instance");
    }
    
    spdlog::info("Setting SSL fd...");

    SSL_set_fd(ssl, clientSocketHandle);

    spdlog::info("SSL Connecting...");
    const int status = SSL_connect(ssl);
    if (status != 1)
    {
        char sslErrStr[256];
        unsigned long sslErr = ERR_get_error();
        ERR_error_string_n(sslErr, sslErrStr, sizeof(sslErrStr));
        throw std::runtime_error(sslErrStr);
    }

    spdlog::info("Connected apparently");
}