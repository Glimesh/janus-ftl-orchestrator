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

#include "OpenSslPtr.h"

#include <functional>
#include <openssl/err.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <sys/socket.h>
#include <unistd.h>

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
    SslCtxPtr sslContext(SSL_CTX_new(TLS_server_method()));

    // Disable old protocols
    SSL_CTX_set_min_proto_version(sslContext.get(), TLS1_3_VERSION);
    SSL_CTX_set_max_proto_version(sslContext.get(), TLS1_3_VERSION);

    // Restrict to secure PSK ciphers
    if (!SSL_CTX_set_ciphersuites(sslContext.get(), "TLS_AES_128_GCM_SHA256"))
    {
        char sslErrStr[256];
        unsigned long sslErr = ERR_get_error();
        ERR_error_string_n(sslErr, sslErrStr, sizeof(sslErrStr));
        throw std::runtime_error(sslErrStr);
    }

    // Set up callback to locate pre-shared key
    SSL_CTX_set_psk_find_session_callback(sslContext.get(), &OrchestrationConnection::callbackFindSslPsk);

    // Create new SSL instance from context
    SslPtr ssl(SSL_new(sslContext.get()));

    // Add self-reference to the SSL instance so we can get back from callback functions
    SSL_set_ex_data(ssl.get(), 0, this);

    // Bind SSL to our socket file descriptor and attempt to accept incoming connection
    SSL_set_fd(ssl.get(), clientSocketHandle);
    if (SSL_accept(ssl.get()) <= 0)
    {
        char sslErrStr[256];
        unsigned long sslErr = ERR_get_error();
        ERR_error_string_n(sslErr, sslErrStr, sizeof(sslErrStr));
        spdlog::warn("Failure accepting TLS connection: {}", sslErrStr);
        shutdown(clientSocketHandle, SHUT_RDWR);
        close(clientSocketHandle);
        return;
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

int OrchestrationConnection::findSslPsk(
    SSL *ssl,
    const unsigned char *identity,
    size_t identity_len,
    SSL_SESSION **sess)
{
    spdlog::info("IDENTITY: {}", std::string(identity, identity + identity_len));

    // Convert key hex string to bytes
    long keyLength;
    OpenSslPtr keyPtr(OPENSSL_hexstr2buf(preSharedKeyHexStr.c_str(), &keyLength));
    if (keyPtr.get() == nullptr)
    {
        spdlog::error(
            "Could not convert PSK hex string to byte array. String: {}",
            preSharedKeyHexStr);
        return 0;
    }

    spdlog::info(
        "findSslPsk: Using key {}",
        spdlog::to_hex(
            reinterpret_cast<const unsigned char*>(keyPtr.get()),
            reinterpret_cast<const unsigned char*>(keyPtr.get()) + keyLength));

    // Find the cipher we'll be using
    // identified by IANA mapping: https://testssl.sh/openssl-iana.mapping.html
    const unsigned char tls13_aes128gcmsha256_id[] = { 0x13, 0x01 };
    const SSL_CIPHER* cipher = SSL_CIPHER_find(ssl, tls13_aes128gcmsha256_id);
    if (cipher == nullptr)
    {
        spdlog::error("OpenSSL could not find cipher suite!");
        return 0;
    }

    // Create an SSL session and set some parameters on it
    SslSessionPtr temporarySslSession(SSL_SESSION_new());
    if (temporarySslSession == nullptr)
    {
        spdlog::error("Could not create new SSL session!");
        return 0;
    }

    if (!SSL_SESSION_set1_master_key(
        temporarySslSession.get(),
        reinterpret_cast<const unsigned char*>(keyPtr.get()),
        keyLength))
    {
        spdlog::error("Could not set key on new SSL session!");
        return 0;
    }

    if (!SSL_SESSION_set_cipher(temporarySslSession.get(), cipher))
    {
        spdlog::error("Could not set cipher on new SSL session!");
        return 0;
    }

    if (!SSL_SESSION_set_protocol_version(temporarySslSession.get(), TLS1_3_VERSION))
    {
        spdlog::error("Could not set version on new SSL session!");
        return 0;
    }
    
    // Release ownership since we are no longer responsible for this session
    *sess = temporarySslSession.release(); 
    return 1;
}
#pragma endregion