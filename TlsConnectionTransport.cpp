/**
 * @file TlsConnectionTransport.cpp
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-10-18
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#include "TlsConnectionTransport.h"

#include "OpenSslPtr.h"

#include <functional>
#include <openssl/err.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <sys/socket.h>
#include <unistd.h>

#pragma region Constructor/Destructor
TlsConnectionTransport::TlsConnectionTransport(
    int clientSocketHandle,
    sockaddr_in acceptAddress,
    std::vector<uint8_t> preSharedKey
) : 
    clientSocketHandle(clientSocketHandle),
    acceptAddress(acceptAddress),
    preSharedKey(preSharedKey)
{ }
#pragma endregion

#pragma region IConnectionTransport
void TlsConnectionTransport::Start()
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
    SSL_CTX_set_psk_find_session_callback(sslContext.get(), &TlsConnectionTransport::callbackFindSslPsk);

    // Create new SSL instance from context
    ssl = SslPtr(SSL_new(sslContext.get()));

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
        closeConnection();
    }
    else
    {
        spdlog::info("Accepted new TLS connection");
    }
}

void TlsConnectionTransport::Stop()
{
    shutdown(clientSocketHandle, SHUT_RDWR);
    close(clientSocketHandle);
}

std::vector<uint8_t> TlsConnectionTransport::Read()
{
    // Try to read some bytes
    char buffer[512];
    int bytesRead = SSL_read(ssl.get(), buffer, sizeof(buffer));
    if (bytesRead <= 0)
    {
        int sslError = SSL_get_error(ssl.get(), bytesRead);
        switch (sslError)
        {
        // Continuable errors:
        case SSL_ERROR_NONE:
            // Nothing to see here...
        case SSL_ERROR_WANT_READ:
            // Not enough data available; keep trying.
            break;

        // Non-continuable errors:
        case SSL_ERROR_ZERO_RETURN:
            // Client closed the connection.
            spdlog::info("Client closed connection.");
            closeConnection();
            break;
        default:
            // We'll consider other errors fatal.
            spdlog::error("SSL encountered error {}, terminating this connection.", sslError);
            closeConnection();
            break;
        }
    }
    else
    {
        spdlog::info("RECEIVED: {}", std::string(buffer, buffer + bytesRead));
        return std::vector<uint8_t>(buffer, buffer + bytesRead);
    }
}

void TlsConnectionTransport::Write(const std::vector<uint8_t>& bytes)
{
    SSL_write(ssl.get(), bytes.data(), bytes.size());
}

void TlsConnectionTransport::SetOnConnectionClosed(std::function<void(void)> onConnectionClosed)
{
    this->onConnectionClosed = onConnectionClosed;
}
#pragma endregion

#pragma region Private static methods
int TlsConnectionTransport::callbackFindSslPsk(
    SSL *ssl,
    const unsigned char *identity,
    size_t identity_len,
    SSL_SESSION **sess)
{
    // First, pull reference to TlsConnectionTransport instance out of SSL context
    TlsConnectionTransport* that = reinterpret_cast<TlsConnectionTransport*>(SSL_get_ex_data(ssl, 0));
    return that->findSslPsk(ssl, identity, identity_len, sess);
}
#pragma endregion

#pragma region Private methods
void TlsConnectionTransport::closeConnection()
{
    shutdown(clientSocketHandle, SHUT_RDWR);
    close(clientSocketHandle);
    if (onConnectionClosed)
    {
        onConnectionClosed();
    }
}

int TlsConnectionTransport::findSslPsk(
    SSL *ssl,
    const unsigned char *identity,
    size_t identity_len,
    SSL_SESSION **sess)
{
    spdlog::info(
        "findSslPsk: Using key {}",
        spdlog::to_hex(preSharedKey.begin(), preSharedKey.end()));

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
        preSharedKey.data(),
        preSharedKey.size()))
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