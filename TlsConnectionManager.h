/**
 * @file TlsConnectionManager.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-10-25
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#pragma once

#include "IConnection.h"
#include "IConnectionManager.h"

#include <arpa/inet.h>
#include <functional>
#include <memory>
#include <vector>

/**
 * @brief Class responsible for accepting new TlsConnections
 */
class TlsConnectionManager : public IConnectionManager
{
public:
    /* Constructor/Destructor */
    TlsConnectionManager(
        std::vector<uint8_t> preSharedKey,
        in_port_t listenPort = DEFAULT_LISTEN_PORT);

    /* IConnectionManager */
    void Init() override;
    void Listen() override;
    void SetOnNewConnection(
        std::function<void(std::shared_ptr<IConnection>)> onNewConnection) override;

private:
    static constexpr in_port_t DEFAULT_LISTEN_PORT = 8085;
    static constexpr int SOCKET_LISTEN_QUEUE_LIMIT = 64;
    const std::vector<uint8_t> preSharedKey;
    const in_port_t listenPort;
    int listenSocketHandle;
    std::function<void(std::shared_ptr<IConnection>)> onNewConnection;
};