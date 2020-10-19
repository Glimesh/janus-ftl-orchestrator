/**
 * @file OrchestrationConnection.cpp
 * @author Hayden McAfee (hayden@outlook.com)
 * @version 0.1
 * @date 2020-10-18
 * 
 * @copyright Copyright (c) 2020 Hayden McAfee
 * 
 */

#include "OrchestrationConnection.h"

#include <functional>
#include <openssl/err.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>

#pragma region Constructor/Destructor
OrchestrationConnection::OrchestrationConnection(
    int clientSocketHandle,
    std::string preSharedKeyHexStr
) : 
    clientSocketHandle(clientSocketHandle),
    preSharedKeyHexStr(preSharedKeyHexStr)
{ }
#pragma endregion

#pragma region Public methods
void OrchestrationConnection::Start()
{
    connectionThread = std::thread(&OrchestrationConnection::startConnectionThread, this);
    connectionThread.detach();
}

void OrchestrationConnection::Stop()
{
    isStopping = true;
    if (connectionThread.joinable())
    {
        connectionThread.join();
    }
}
#pragma endregion

#pragma region Private static methods
unsigned int OrchestrationConnection::callbackServerPsk(
    SSL *ssl,
    const char *identity,
    unsigned char* psk,
    unsigned int maxPskLen)
{
    // First, pull reference to OrchestrationConnection instance out of SSL context
    OrchestrationConnection* that = reinterpret_cast<OrchestrationConnection*>(SSL_get_ex_data(ssl, 0));
    return that->serverPsk(ssl, identity, psk, maxPskLen);
}

int OrchestrationConnection::callbackFindSslPsk(
    SSL *ssl,
    const unsigned char *identity,
    size_t identity_len,
    SSL_SESSION **sess)
{
    // First, pull reference to OrchestrationConnection instance out of SSL context
    OrchestrationConnection* that = reinterpret_cast<OrchestrationConnection*>(SSL_get_ex_data(ssl, 0));
    return that->findSslPsk(ssl, identity, identity_len, sess);
}
#pragma endregion

#pragma region Private methods
void OrchestrationConnection::startConnectionThread()
{
    // TLS handshake time
    SSL* ssl;
    SSL_CTX* sslContext = SSL_CTX_new(TLS_server_method());

    // Disable old protocols
    SSL_CTX_set_min_proto_version(sslContext, TLS1_3_VERSION);
    SSL_CTX_set_max_proto_version(sslContext, TLS1_3_VERSION);

    // Restrict to secure PSK ciphers
    //SSL_CTX_set_cipher_list(sslContext, SUPPORTED_CIPHER_LIST);


    // Set up callback to locate pre-shared keys
    SSL_CTX_set_psk_server_callback(sslContext, &OrchestrationConnection::callbackServerPsk);
    SSL_CTX_set_psk_find_session_callback(sslContext, &OrchestrationConnection::callbackFindSslPsk);

    ssl = SSL_new(sslContext);
    // Add self-reference to the SSL context so we can get back from callback functions
    SSL_set_ex_data(ssl, 0, this);
    // Bind SSL to our socket file descriptor
    SSL_set_fd(ssl, clientSocketHandle);
    if (SSL_accept(ssl) <= 0)
    {
        char sslErrStr[256];
        unsigned long sslErr = ERR_get_error();
        ERR_error_string_n(sslErr, sslErrStr, sizeof(sslErrStr));
        throw std::runtime_error(sslErrStr);
    }

    spdlog::info("Accepted connection apparently");

    while (true)
    {
        // use SSL_read/SSL_write here
        if (isStopping)
        {
            break;
        }
    }
}

unsigned int OrchestrationConnection::serverPsk(
    SSL *ssl,
    const char *identity,
    unsigned char* psk,
    unsigned int maxPskLen)
{
    long keyLength;
    unsigned char* key = OPENSSL_hexstr2buf(preSharedKeyHexStr.c_str(), &keyLength);

    spdlog::info("serverPsk: Using key {}", spdlog::to_hex(key, key + keyLength));

    if (keyLength > maxPskLen) {
        spdlog::error("PSK is too big!");
        OPENSSL_free(key);
        return 0;
    }

    std::memcpy(psk, key, keyLength);
    OPENSSL_free(key);
    return keyLength;
}

int OrchestrationConnection::findSslPsk(
    SSL *ssl,
    const unsigned char *identity,
    size_t identity_len,
    SSL_SESSION **sess)
{
    spdlog::info("IDENTITY: {}", std::string(identity, identity + identity_len));

    // Convert key hex string to bytes
    long keyLength;
    unsigned char* key = OPENSSL_hexstr2buf(preSharedKeyHexStr.c_str(), &keyLength);
    //HACK
    // unsigned char* key = OPENSSL_hexstr2buf("FF33FF33", &keyLength);
    if (key == nullptr)
    {
        spdlog::error(
            "Could not convert PSK hex string to byte array. String: {}",
            preSharedKeyHexStr);
        return 0;
    }

    spdlog::info("findSslPsk: Using key {}", spdlog::to_hex(key, key + keyLength));

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

    if (!SSL_SESSION_set_cipher(temporarySslSession, cipher))
    {
        spdlog::error("Could not set cipher on new SSL session!");
        OPENSSL_free(key);
        return 0;
    }

    if (!SSL_SESSION_set_protocol_version(temporarySslSession, TLS1_3_VERSION))
    {
        spdlog::error("Could not set version on new SSL session!");
        OPENSSL_free(key);
        return 0;
    }
    

    OPENSSL_free(key);
    *sess = temporarySslSession;
    return 1;
}
#pragma endregion