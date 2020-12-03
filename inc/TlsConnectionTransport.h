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
    void StartAsync() override
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

        // Spin up a new thread to handle I/O
        std::promise<bool> sslConnectedPromise;
        std::future<bool> sslConnectedFuture = sslConnectedPromise.get_future();
        connectionThreadEndedFuture = connectionThreadEndedPromise.get_future();
        connectionThread = std::thread(
            &TlsConnectionTransport::connectionThreadBody,
            this,
            std::move(sslConnectedPromise));
        connectionThread.detach();

        // Wait for SSL to finish connecting
        sslConnectedFuture.get();
    }

    void Stop() override
    {
        spdlog::debug("{} Stop() called", socketHandle);
        if (!isStopping && !isStopped)
        {
            isStopping = true;
            SSL_shutdown(ssl.get());
            shutdown(socketHandle, SHUT_RDWR);
            close(socketHandle);
            spdlog::debug("{} CLOSED: Triggered by local, waiting for thread end...", socketHandle);
            connectionThreadEndedFuture.wait(); // Wait until our connection thread end has ended
            spdlog::debug("{} CLOSED: Triggered by local, thread ended.", socketHandle);
        }
        else if (isStopping && !isStopped)
        {
            spdlog::debug(
                "{} Requested to stop but we're already stopping... waiting until we're closed",
                socketHandle);
            connectionThreadEndedFuture.wait(); // Wait until our connection thread end has ended
            spdlog::debug("{} Thread ended.", socketHandle);
        }
    }

    void Write(const std::vector<std::byte>& bytes) override
    {
        if (!isStopping && !isStopped)
        {
            std::lock_guard<std::mutex> lock(writeMutex);
            spdlog::debug("{} ATTEMPT WRITE {} bytes", socketHandle, bytes.size());
            int writeResult = write(writePipeFds[1], bytes.data(), bytes.size());
            if (static_cast<size_t>(writeResult) != bytes.size())
            {
                spdlog::error(
                    "Write returned {} result, expected {} bytes.",
                    writeResult,
                    bytes.size());
                closeConnection();
            }
        }
    }

    void SetOnBytesReceived(
        std::function<void(const std::vector<std::byte>&)> onBytesReceived) override
    {
        this->onBytesReceived = onBytesReceived;
    }

    void SetOnConnectionClosed(std::function<void(void)> onConnectionClosed) override
    {
        this->onConnectionClosed = onConnectionClosed;
    }

private:
    /* Static members */
    static constexpr int BUFFER_SIZE = 512;
    /* Private members */
    const bool isServer;
    const int socketHandle;
    sockaddr_in targetAddress;
    const std::vector<std::byte> preSharedKey;
    std::atomic<bool> isStopping { false }; // Indicates when SSL has been signaled to shut down
    std::atomic<bool> isStopped { false }; // Indicates when the socket has been closed
    SslPtr ssl;
    SSL_psk_find_session_cb_func sslPskCallbackFunc;
    std::function<void(const std::vector<std::byte>&)> onBytesReceived;
    std::function<void(void)> onConnectionClosed;
    std::promise<void> connectionThreadEndedPromise;
    std::future<void> connectionThreadEndedFuture;
    std::thread connectionThread;
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
    void connectionThreadBody(std::promise<bool>&& sslConnectedPromise)
    {
        // Indicate when we've exited this thread
        connectionThreadEndedPromise.set_value_at_thread_exit();

        // First, we need to connect.
        int connectResult = isServer ? SSL_accept(ssl.get()) : SSL_connect(ssl.get());
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
                sslConnectedPromise.set_value(false);
                closeConnection();
                return;
            }
            
            // Try again
            connectResult = SSL_connect(ssl.get());
        }

        spdlog::debug("{} SSL CONNECTED", socketHandle);
        sslConnectedPromise.set_value(true);

        // We're connected. Now wait for input/output.
        char readBuf[BUFFER_SIZE];
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

            poll(pollFds, 2, 200 /*ms*/);

            // Did the socket get closed?
            if (((pollFds[0].revents & POLLERR) > 0) || 
                ((pollFds[0].revents & POLLHUP) > 0) ||
                ((pollFds[0].revents & POLLNVAL) > 0))
            {
                closeConnection();
                return;
            }

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
                        if (bytesRead > 0)
                        {
                            if (onBytesReceived)
                            {
                                onBytesReceived(
                                    std::vector<std::byte>(
                                        reinterpret_cast<std::byte*>(readBuf),
                                        (reinterpret_cast<std::byte*>(readBuf) + bytesRead)));
                            }
                        }
                        break;
                    case SSL_ERROR_WANT_READ:
                        // Can't read yet - try again later
                        spdlog::debug("{} SSL_ERROR_WANT_READ", socketHandle);
                        break;
                    case SSL_ERROR_WANT_WRITE:
                        // Nothing we can do here, continue to next poll
                        spdlog::debug("{} SSL_ERROR_WANT_WRITE", socketHandle);
                        break;
                    case SSL_ERROR_ZERO_RETURN:
                        // Connection closed
                        closeConnection();
                        return;
                    case SSL_ERROR_SYSCALL:
                    default:
                        // Some other error - close
                        closeConnection();
                        return;
                    }

                    // Check if we're done processing incoming data
                    if (!SSL_pending(ssl.get()))
                    {
                        break;
                    }
                    else
                    {
                        spdlog::debug("{} SSL_PENDING", socketHandle);
                    }
                }
            }

            // Data available for writing?
            if (pollFds[1].revents & POLLIN)
            {
                int readResult;
                std::byte writeBuffer[BUFFER_SIZE];
                {
                    std::lock_guard<std::mutex> lock(writeMutex);
                    readResult = read(writePipeFds[0], writeBuffer, sizeof(writeBuffer));
                }
                if (readResult < 0)
                {
                    int readError = errno;
                    spdlog::error(
                        "Read from write pipe failed with error {}",
                        readError);
                    closeConnection();
                    return;
                }
                else if (readResult > 0)
                {
                    int sslWriteResult = SSL_write(ssl.get(), writeBuffer, readResult);
                    int writeError = SSL_get_error(ssl.get(), sslWriteResult);
                    if ((writeError == SSL_ERROR_NONE) ||
                        (writeError == SSL_ERROR_WANT_READ) ||
                        (writeError == SSL_ERROR_WANT_WRITE))
                    {
                        // Success!
                        spdlog::debug(
                            "{} WROTE {} / {} bytes",
                            socketHandle,
                            sslWriteResult,
                            readResult);
                    }
                    else if (writeError == SSL_ERROR_ZERO_RETURN)
                    {
                        // Connection was closed
                        closeConnection();
                        return;
                    }
                    else
                    {
                        // Some other unknown error...
                        closeConnection();
                        return;
                    }
                }
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
            spdlog::debug("{} CLOSED: Triggered by remote\n", socketHandle);
            if (onConnectionClosed)
            {
                spdlog::debug("{} transport running onConnectionClosed callback...", socketHandle);
                onConnectionClosed();
            }
        }

        if (!isStopped)
        {
            // Once we reach this point, we know the socket has finished closing.
            // Close our write pipes
            spdlog::debug("{} CLOSED WRITE PIPES", socketHandle);
            close(writePipeFds[0]);
            close(writePipeFds[1]);
            isStopped = true;
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
        spdlog::debug(
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
        spdlog::debug(
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