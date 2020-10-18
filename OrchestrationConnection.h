/**
 * @file OrchestrationConnection.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @version 0.1
 * @date 2020-10-18
 * 
 * @copyright Copyright (c) 2020 Hayden McAfee
 * 
 */

#pragma once

#include <atomic>
#include <filesystem>
#include <openssl/ssl.h>
#include <string>
#include <thread>

/**
 * @brief OrchestrationConnection represents a connection to a single FTL instance.
 */
class OrchestrationConnection
{
public:
    /* Constructor/Destructor */
    /**
     * @brief Construct a new Orchestration Connection object
     * 
     * @param clientSocketHandle the handle to the accepted socket connection for this client
     * @param preSharedKeyHexStr pre-shared key in hex format (00-FF) for TLS PSK encryption
     */
    OrchestrationConnection(
        int clientSocketHandle,
        std::string preSharedKeyHexStr);

    /* Public methods */
    /**
     * @brief Starts the connection in a new thread
     */
    void Start();

    /**
     * @brief Shuts down connection to client and terminates connection thread
     */
    void Stop();

private:
    /* Private members */
    static constexpr const char* SUPPORTED_CIPHER_LIST = "TLSv1.2+PSK:@SECLEVEL=5:@STRENGTH";
    const int clientSocketHandle;
    const std::string preSharedKeyHexStr;
    std::thread connectionThread;
    std::atomic<bool> isStopping = { 0 };
    SSL_psk_find_session_cb_func sslPskCallbackFunc;

    /* Private static methods */
    static int callbackFindSslPsk(SSL *ssl, const unsigned char *identity, size_t identity_len, SSL_SESSION **sess);

    /* Private methods */
    /**
     * @brief Contains the code that runs in the connection thread.
     */
    void startConnectionThread();

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