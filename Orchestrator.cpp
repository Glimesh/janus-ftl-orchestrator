/**
 * @file Orchestrator.cpp
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-10-18
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#include "Orchestrator.h"

#include "IConnection.h"
#include "IConnectionManager.h"
#include "Configuration.h"
#include "Util.h"

#include <spdlog/spdlog.h>

#pragma region Constructor/Destructor
Orchestrator::Orchestrator(std::shared_ptr<IConnectionManager> connectionManager) : 
    connectionManager(connectionManager)
{ }
#pragma endregion

#pragma region Public methods
void Orchestrator::Init()
{
    // Set IConnectionManager callbacks
    connectionManager->SetOnNewConnection(
        std::bind(&Orchestrator::newConnection, this, std::placeholders::_1));
}

const std::set<std::shared_ptr<IConnection>> Orchestrator::GetConnections()
{
    return connections;
}
#pragma endregion

#pragma region Private methods
void Orchestrator::newConnection(std::shared_ptr<IConnection> connection)
{
    spdlog::info(
        "New connection established to {} @ {}",
        IConnection::FtlServerKindToString(connection->GetServerKind()),
        connection->GetHostname());

    // Set IConnection callbacks
    connection->SetOnConnectionClosed(
        std::bind(&Orchestrator::connectionClosed, this, connection));

    std::lock_guard<std::mutex> lock(connectionsMutex);
    connections.insert(connection);
}

void Orchestrator::connectionClosed(std::shared_ptr<IConnection> connection)
{
    spdlog::info(
        "Connection closed to {} @ {}",
        IConnection::FtlServerKindToString(connection->GetServerKind()),
        connection->GetHostname());

    std::lock_guard<std::mutex> lock(connectionsMutex);
    connections.erase(connection);
}
#pragma endregion