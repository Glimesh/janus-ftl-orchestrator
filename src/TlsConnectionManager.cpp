/**
 * @file TlsConnectionManager.cpp
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-10-25
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#include "TlsConnectionManager.h"

#include "FtlConnection.h"
#include "TlsConnectionTransport.h"
#include "Util.h"

#include <openssl/ssl.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stdexcept>

#pragma region Constructor/Destructor
template <class T>
TlsConnectionManager<T>::TlsConnectionManager(
    std::vector<std::byte> preSharedKey,
    in_port_t listenPort
) :
    preSharedKey(preSharedKey),
    listenPort(listenPort)
{ }
#pragma endregion

#pragma region IConnectionManager
template <class T>
void TlsConnectionManager<T>::Init()
{
    // Initialize OpenSSL pieces
    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();
}

template <class T>
void TlsConnectionManager<T>::Listen(std::promise<void>&& readyPromise)
{
    sockaddr_in listenAddr
    {
        .sin_family = AF_INET,
        .sin_port   = htons(listenPort),
        .sin_addr = { .s_addr = htonl(INADDR_ANY) } // TODO: Bind to specific interface
    };

    // Create socket
    listenSocketHandle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocketHandle < 0)
    {
        int error = errno;
        std::stringstream errStr;
        errStr << "Unable to create listen socket! Error "
            << error << ": " << Util::ErrnoToString(error);
        throw std::runtime_error(errStr.str());
    }

    // Bind socket
    if (bind(
        listenSocketHandle,
        reinterpret_cast<const sockaddr*>(&listenAddr),
        sizeof(listenAddr)) < 0)
    {
        int error = errno;
        std::stringstream errStr;
        errStr << "Unable to bind socket! Error "
            << error << ": " << Util::ErrnoToString(error);
        throw std::runtime_error(errStr.str());
    }

    // Listen on socket
    if (listen(listenSocketHandle, SOCKET_LISTEN_QUEUE_LIMIT) < 0)
    {
        int error = errno;
        std::stringstream errStr;
        errStr << "Unable to listen on socket! Error "
            << error << ": " << Util::ErrnoToString(error);
        throw std::runtime_error(errStr.str());
    }

    // Accept new connections
    spdlog::info("Listening on port {}...", listenPort);
    readyPromise.set_value(); // Indicate to promise that we are now listening
    while (true)
    {
        sockaddr_in acceptedAddr { 0 };
        socklen_t acceptedAddrLen = sizeof(acceptedAddr);
        int clientHandle = accept(
            listenSocketHandle,
            reinterpret_cast<sockaddr*>(&acceptedAddr),
            &acceptedAddrLen);

        if (clientHandle < 0)
        {
            int error = errno;
            if (error == EINVAL)
            {
                // This means we've closed the listen handle
                spdlog::info("TLS Connection Manager shutting down...");
                break;
            }
            std::stringstream errStr;
            errStr << "Unable to accept incoming connection! Error "
                << error << ": " << Util::ErrnoToString(error);
            throw std::runtime_error(errStr.str());
        }

        spdlog::info("Accepted new connection...");

        std::shared_ptr<TlsConnectionTransport> transport = 
            std::make_shared<TlsConnectionTransport>(
                true /*isServer*/,
                clientHandle,
                acceptedAddr,
                preSharedKey);

        std::shared_ptr<T> connection = std::make_shared<T>(transport);

        if (onNewConnection)
        {
            onNewConnection(connection);
        }
        else
        {
            spdlog::warn("Accepted a new connection, but nobody was listening. :(");
        }
    }
}

template <class T>
void TlsConnectionManager<T>::StopListening()
{
    shutdown(listenSocketHandle, SHUT_RDWR);
    close(listenSocketHandle);
}

template <class T>
void TlsConnectionManager<T>::SetOnNewConnection(
    std::function<void(std::shared_ptr<T>)> onNewConnection)
{
    this->onNewConnection = onNewConnection;
}

#pragma endregion

#pragma region Template instantiations
// Yeah, this is weird, but necessary.
// See https://stackoverflow.com/questions/495021/why-can-templates-only-be-implemented-in-the-header-file
template class TlsConnectionManager<FtlConnection>;
#pragma endregion