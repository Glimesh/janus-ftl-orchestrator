/**
 * @file Orchestrator.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-10-18
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#pragma once

#include <arpa/inet.h>
#include <memory>
#include <mutex>
#include <set>

// Forward declarations
class Configuration;
class IConnection;
class IConnectionManager;

/**
 * @brief
 *  Orchestrator handles listening for and maintaining incoming
 *  orchestration connections
 */
class Orchestrator
{
public:
    /* Constructor/Destructor */
    Orchestrator(std::shared_ptr<IConnectionManager> connectionManager);

    /* Public methods */
    /**
     * @brief Performs initialization steps
     */
    void Init();

    /**
     * @brief Get connections currently registered with this orchestrator
     * 
     * @return const std::set<std::shared_ptr<IConnection>> set of connections
     */
    const std::set<std::shared_ptr<IConnection>> GetConnections();

private:
    /* Private members */
    const std::shared_ptr<IConnectionManager> connectionManager;
    std::mutex connectionsMutex;
    std::set<std::shared_ptr<IConnection>> connections;

    /* Private methods */
    void newConnection(std::shared_ptr<IConnection> connection);
    void connectionClosed(std::shared_ptr<IConnection> connection);
};