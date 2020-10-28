/**
 * @file TlsConnection.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-10-18
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#pragma once

#include "IConnectionTransport.h"

#include "OpenSslPtr.h"
#include "FtlTypes.h"

#include <arpa/inet.h>
#include <atomic>
#include <filesystem>
#include <functional>
#include <openssl/ssl.h>
#include <string>
#include <thread>
#include <vector>

/**
 * @brief
 *  TlsConnectionTransport represents a connection to a single FTL instance
 *  over a TCP socket secured by TLS.
 */
class TlsConnectionTransport : public IConnectionTransport
{
public:
    /* Constructor/Destructor */
    /**
     * @brief Construct a new TlsConnectionTransport object
     * @param clientSocketHandle the handle to the accepted socket connection for this client
     * @param preSharedKeyHexStr pre-shared key in hex format (00-FF) for TLS PSK encryption
     */
    TlsConnectionTransport(
        int clientSocketHandle,
        sockaddr_in acceptAddress,
        std::vector<uint8_t> preSharedKey);

    /* IConnectionTransport */
    void Start() override;
    void Stop() override;
    std::vector<uint8_t> Read() override;
    void Write(std::vector<uint8_t> bytes) override;
    void SetOnConnectionClosed(std::function<void(void)> onConnectionClosed) override;

private:
    /* Private members */
    const int clientSocketHandle;
    sockaddr_in acceptAddress;
    const std::vector<uint8_t> preSharedKey;
    SslPtr ssl;
    SSL_psk_find_session_cb_func sslPskCallbackFunc;
    std::function<void(void)> onConnectionClosed;

    /* Private static methods */
    static int callbackFindSslPsk(SSL *ssl, const unsigned char *identity, size_t identity_len, SSL_SESSION **sess);

    /* Private methods */
    /**
     * @brief Closes the socket and fires connection closed callback
     */
    void closeConnection();

    /**
     * @brief A callback function passed to OpenSSL used to find pre-shared keys
     * 
     * @param ssl 
     * @param identity 
     * @param identity_len 
     * @param sess 
     */
    int findSslPsk(SSL *ssl, const unsigned char *identity, size_t identity_len, SSL_SESSION **sess);
};