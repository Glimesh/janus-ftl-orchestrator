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
#include <mutex>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <poll.h>
#include <string>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <thread>
#include <unistd.h>
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
     * @param isServer true if this is a server connection, false if this is a client connection
     * @param socketHandle the handle to the socket connection for this connection
     * @param targetAddress the address that this connection is communicating with
     * @param preSharedKey pre-shared key for TLS PSK encryption
     */
    TlsConnectionTransport(
        bool isServer,
        int socketHandle,
        sockaddr_in targetAddress,
        std::vector<std::byte> preSharedKey
    ) : 
        isServer(isServer),
        socketHandle(socketHandle),
        targetAddress(targetAddress),
        preSharedKey(preSharedKey)
    { }


    /* IConnectionTransport */
    void Start() override
    {
        // First, set the socket to non-blocking IO mode
        int socketFlags = fcntl(socketHandle, F_GETFL, 0);
        socketFlags = socketFlags | O_NONBLOCK;
        if (fcntl(socketHandle, F_SETFL, socketFlags) != 0)
        {
            throw std::runtime_error("Could not set socket to non-blocking mode.");
        }

        SslCtxPtr sslContext(SSL_CTX_new(isServer ? TLS_server_method() : TLS_client_method()));

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
        if (isServer)
        {
            SSL_CTX_set_psk_find_session_callback(
                sslContext.get(),
                &TlsConnectionTransport::sslPskFindSessionCallback);
        }
        else
        {
            SSL_CTX_set_psk_use_session_callback(
                sslContext.get(),
                &TlsConnectionTransport::sslPskUseSessionCallback);
        }

        // Create new SSL instance from context
        ssl = SslPtr(SSL_new(sslContext.get()));

        // Add self-reference to the SSL instance so we can get back from callback functions
        SSL_set_ex_data(ssl.get(), 0, this);

        // Bind SSL to our socket file descriptor and attempt to accept/connect
        SSL_set_fd(ssl.get(), socketHandle);

        // Open pipe used for writing to SSL
        if (pipe2(writePipeFds, O_NONBLOCK) != 0)
        {
            throw std::runtime_error("Could not open SSL write pipe!");
        }

        if (isServer)
        {
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
        else
        {
            if (SSL_connect(ssl.get()) <= 0)
            {
                char sslErrStr[256];
                unsigned long sslErr = ERR_get_error();
                ERR_error_string_n(sslErr, sslErrStr, sizeof(sslErrStr));
                throw std::runtime_error(sslErrStr);
            }
            else
            {
                spdlog::info("Connected to TLS server");
            }
        }
        
    }

    void Stop() override
    {
        if (!isStopping && !isStopped)
        {
            isStopping = true;
            SSL_shutdown(ssl.get());
            shutdown(socketHandle, SHUT_RDWR);
            close(socketHandle);
            printf("%d CLOSED: Triggered by local\n", socketHandle);
        }
    }

    std::vector<uint8_t> Read() override
    {
        if (isStopped)
        {
            return std::vector<uint8_t>();
        }

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
                spdlog::warn("SSL encountered error {}, terminating this connection.", sslError);
                closeConnection();
                break;
            }
        }
        else
        {
            spdlog::info("RECEIVED: {} bytes", bytesRead);
            return std::vector<uint8_t>(buffer, buffer + bytesRead);
        }

        return std::vector<uint8_t>();
    }

    void Write(const std::vector<uint8_t>& bytes) override
    {
        if (!isStopping && !isStopped)
        {
            std::lock_guard<std::mutex> lock(writeMutex);
            int writeResult = SSL_write(ssl.get(), bytes.data(), bytes.size());
            if (writeResult <= 0)
            {
                spdlog::warn("ERROR {} WRITING {} BYTES", writeResult, bytes.size());
            }
            else
            {
                spdlog::info("WROTE {} BYTES", writeResult);
            }
        }
    }

    void SetOnConnectionClosed(std::function<void(void)> onConnectionClosed) override
    {
        this->onConnectionClosed = onConnectionClosed;
    }

private:
    /* Private members */
    const bool isServer;
    const int socketHandle;
    sockaddr_in targetAddress;
    const std::vector<std::byte> preSharedKey;
    std::atomic<bool> isStopping { false }; // Indicates when SSL has been signaled to shut down
    std::atomic<bool> isStopped { false }; // Indicates when the socket has been closed
    SslPtr ssl;
    SSL_psk_find_session_cb_func sslPskCallbackFunc;
    std::function<void(void)> onConnectionClosed;
    std::mutex writeMutex;
    int writePipeFds[2]; // Pipe used to write to the SSL socket

    /* Private static methods */
    /**
     * @brief
     *  This is a static callback method for SSL. It will pull a reference to the
     *  TlsConnectionTransport instance out of SSL object's data storage, and use that to
     *  call the sslPskFindSession member function.
     */
    static int sslPskFindSessionCallback(
        SSL* ssl,
        const unsigned char *identity,
        size_t identity_len,
        SSL_SESSION **sess)
    {
        TlsConnectionTransport* that = 
            reinterpret_cast<TlsConnectionTransport*>(SSL_get_ex_data(ssl, 0));
        return that->sslPskFindSession(ssl, identity, identity_len, sess);
    }

    /**
     * @brief
     *  This is a static callback method for SSL. It will pull a reference to the
     *  TlsConnectionTransport instance out of SSL object's data storage, and use that to
     *  call the sslPskUseSession member function.
     */
    static int sslPskUseSessionCallback(
        SSL* ssl,
        const EVP_MD* md,
        const unsigned char** id,
        size_t* idlen,
        SSL_SESSION** sess)
    {
        TlsConnectionTransport* that = 
            reinterpret_cast<TlsConnectionTransport*>(SSL_get_ex_data(ssl, 0));
        return that->sslPskUseSession(ssl, md, id, idlen, sess);
    }

    /**
     * @brief Thread body for processing SSL socket input/output
     */
    void connectionThreadBody()
    {
        // First, we need to connect.
        int connectResult = SSL_connect(ssl.get());
        while (connectResult == -1)
        {
            // We're not done connecting yet - figure out what we're waiting on
            int connectError = SSL_get_error(ssl.get(), connectResult);
            if (connectError == SSL_ERROR_WANT_READ)
            {
                // OpenSSL wants to read, but the socket can't yet, so wait for it.
                pollfd readPollFd
                {
                    .fd = socketHandle,
                    .events = POLLIN,
                    .revents = 0,
                };
                poll(&readPollFd, 1, 100 /*ms*/);
            }
            else if (connectError == SSL_ERROR_WANT_WRITE)
            {
                // OpenSSL wants to write, but the socket can't yet, so wait for it.
                pollfd writePollFd
                {
                    .fd = socketHandle,
                    .events = POLLOUT,
                    .revents = 0,
                };
                poll(&writePollFd, 1, 100 /*ms*/);
            }
            else
            {
                // Unexpected error - close this connection.
                closeConnection();
                return;
            }
            
            // Try again
            connectResult = SSL_connect(ssl.get());
        }

        // We're connected. Now wait for input/output.
        char readBuf[512];
        while (true)
        {
            pollfd pollFds[]
            {
                // OpenSSL socket read
                {
                    .fd = socketHandle,
                    .events = POLLIN,
                    .revents = 0,
                },
                // Pending writes
                {
                    .fd = writePipeFds[0],
                    .events = POLLIN,
                    .revents = 0,
                },
            };

            poll(pollFds, 2, 100 /*ms*/);

            // Data available for reading?
            if (pollFds[0].revents & POLLIN)
            {
                while (true)
                {
                    int bytesRead = SSL_read(ssl.get(), readBuf, sizeof(readBuf));
                    int readError = SSL_get_error(ssl.get(), bytesRead);
                    switch (readError)
                    {
                    case SSL_ERROR_NONE:
                        // Successfully read!
                        break;
                    case SSL_ERROR_ZERO_RETURN:
                        // Connection closed
                        break;
                    case SSL_ERROR_WANT_READ:
                        // Didn't finish reading - try again
                        continue;
                        break;
                    case SSL_ERROR_WANT_WRITE:
                        // Nothing we can do here
                        break;
                    case SSL_ERROR_SYSCALL:
                    default:
                        // Some other error - close
                        break;
                    }

                    // Check if we're done processing incoming data
                    if (!SSL_pending(ssl.get()))
                    {
                        break;
                    }
                }
            }

            // Data available for writing?
            if (pollFds[1].revents & POLLIN)
            {

            }
        }
    }

    /* Private methods */
    /**
     * @brief Closes the socket and fires connection closed callback
     */
    void closeConnection()
    {
        // Avoid closing the socket twice (in case we were the ones who closed it)
        if (!isStopping)
        {
            isStopping = true;
            shutdown(socketHandle, SHUT_RDWR);
            close(socketHandle);
            printf("%d CLOSED: Triggered by remote\n", socketHandle);
        }

        // Avoid firing the closed callback twice
        if (!isStopped)
        {
            isStopped = true;
            if (onConnectionClosed)
            {
                onConnectionClosed();
            }
        }
    }

    /**
     * @brief
     *  This function handles a callback by SSL server connections to provide the pre-shared key
     */
    int sslPskFindSession(
        SSL *ssl,
        const unsigned char *identity,
        size_t identity_len,
        SSL_SESSION **sess)
    {
        spdlog::info(
            "sslPskFindSession: Using key {}",
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
            reinterpret_cast<const unsigned char*>(preSharedKey.data()),
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

    /**
     * @brief 
     *  This function handles a callback by SSL client connections to provide the pre-shared key
     */
    int sslPskUseSession(
        SSL* ssl,
        const EVP_MD* md,
        const unsigned char** id,
        size_t* idlen,
        SSL_SESSION** sess)
    {
        spdlog::info(
            "sslPskUseSession: Using key {}",
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
            reinterpret_cast<const unsigned char*>(preSharedKey.data()),
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
        *id = reinterpret_cast<const unsigned char*>("orchestrator");
        *idlen = 12;

        return 1;
    }
};