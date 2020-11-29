/**
 * @file FtlOrchestrationClient.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-11-25
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#pragma once

#include "FtlConnection.h"
#include "TlsConnectionTransport.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <optional>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>

/**
 * @brief
 *  This is a class that allows you to easily connect to and communicate with the FTL Orchestration
 *  Service via the FTL Orchestration protocol.
 *  This is meant to be consumed by applications as a header-only library.
 */
class FtlOrchestrationClient
{
public:
    static std::shared_ptr<FtlConnection> Connect(
        std::string hostname,
        std::vector<std::byte> preSharedKey,
        uint16_t port = DEFAULT_PORT)
    {
        // Load SSL stuff
        SSL_load_error_strings();
        SSL_library_init();
        OpenSSL_add_all_algorithms();

        // Look up hostname
        addrinfo addrHints { 0 };
        addrHints.ai_family = AF_INET; // TODO: IPV6 support
        addrHints.ai_socktype = SOCK_STREAM;
        addrHints.ai_protocol = IPPROTO_TCP;
        addrinfo* addrLookup = nullptr;
        int lookupErr = 
            getaddrinfo(hostname.c_str(), std::to_string(port).c_str(), &addrHints, &addrLookup);
        if (lookupErr != 0)
        {
            throw std::invalid_argument("Error looking up hostname");
        }
        
        // Attempt to open TCP connection
        // TODO: Loop through additional addresses on failure. For now, only try the first one.
        addrinfo targetAddr = *addrLookup;
        int socketHandle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        int connectErr = connect(socketHandle, targetAddr.ai_addr, targetAddr.ai_addrlen);
        if (connectErr != 0)
        {
            throw std::runtime_error("Could not connect to Orchestration service on given host");
        }
        printf("%d CONNECT\n", socketHandle);

        // Fire up a TlsConnectionTransport to handle TLS on this socket
        std::shared_ptr<TlsConnectionTransport> transport = 
            std::make_shared<TlsConnectionTransport>(
                false /*isServer*/,
                socketHandle,
                *(reinterpret_cast<sockaddr_in*>(targetAddr.ai_addr)),
                preSharedKey);

        // Create an FtlConnection on top of this transport
        std::shared_ptr<FtlConnection> ftlConnection = std::make_shared<FtlConnection>(transport);
        return ftlConnection;
    }

private:
    static constexpr in_port_t DEFAULT_PORT = 8085;
};