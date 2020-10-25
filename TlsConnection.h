/**
 * @file TlsConnection.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @version 0.1
 * @date 2020-10-18
 * 
 * @copyright Copyright (c) 2020 Hayden McAfee
 * 
 */

#pragma once

#include "IConnection.h"

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
 *  OrchestrationConnection represents a connection to a single FTL instance
 *  over a TCP socket secured by TLS.
 */
class TlsConnection : public IConnection
{
public:
    /* Constructor/Destructor */
    /**
     * @brief Construct a new Orchestration Connection object
     * 
     * @param clientSocketHandle the handle to the accepted socket connection for this client
     * @param preSharedKeyHexStr pre-shared key in hex format (00-FF) for TLS PSK encryption
     */
    TlsConnection(
        int clientSocketHandle,
        sockaddr_in acceptAddress,
        std::vector<uint8_t> preSharedKey);

    /* IOrchestrationConnection */
    void Start() override;
    void Stop() override;
    void SetOnConnectionClosed(std::function<void(void)> onConnectionClosed) override;
    std::string GetHostname() override;
    FtlServerKind GetServerKind() override;

private:
    /* Private members */
    const int clientSocketHandle;
    sockaddr_in acceptAddress;
    const std::vector<uint8_t> preSharedKey;
    std::string hostname;
    FtlServerKind serverKind;
    std::function<void(void)> onConnectionClosed;
    std::thread connectionThread;
    std::atomic<bool> isStopping = { 0 };
    SSL_psk_find_session_cb_func sslPskCallbackFunc;

    /* Private static methods */
    static unsigned int callbackServerPsk(SSL *ssl, const char *identity, unsigned char* psk, unsigned int maxPskLen);
    static int callbackFindSslPsk(SSL *ssl, const unsigned char *identity, size_t identity_len, SSL_SESSION **sess);

    /* Private methods */
    /**
     * @brief Contains the code that runs in the connection thread.
     */
    void startConnectionThread();

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