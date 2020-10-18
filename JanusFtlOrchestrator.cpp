/**
 * @file JanusFtlOrchestrator.cpp
 * @author Hayden McAfee (hayden@outlook.com)
 * @version 0.1
 * @date 2020-10-18
 * 
 * @copyright Copyright (c) 2020 Hayden McAfee
 * 
 */

#include "JanusFtlOrchestrator.h"

#include "OrchestrationConnection.h"
#include "Util.h"

#include <openssl/ssl.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stdexcept>
#include <string.h>

#pragma region Constructor/Destructor
JanusFtlOrchestrator::JanusFtlOrchestrator(in_port_t listenPort) : 
    listenPort(listenPort)
{ }
#pragma endregion

#pragma region Public methods
void JanusFtlOrchestrator::Init()
{
    // Initialize OpenSSL pieces
    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();
}

void JanusFtlOrchestrator::Listen()
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
            std::stringstream errStr;
            errStr << "Unable to accept incoming connection! Error "
                << error << ": " << Util::ErrnoToString(error);
            throw std::runtime_error(errStr.str());
        }

        spdlog::info("Accepted new connection...");

        std::shared_ptr<OrchestrationConnection> connection = 
            std::make_shared<OrchestrationConnection>(clientHandle, "FF33FF33");
        pendingConnections.insert(connection);
        connection->Start();
    }
}

void JanusFtlOrchestrator::Stop()
{
    // TODO
}
#pragma endregion