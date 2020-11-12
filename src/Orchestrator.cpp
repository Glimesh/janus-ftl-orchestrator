/**
 * @file Orchestrator.cpp
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-10-18
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#include "Orchestrator.h"

#include "FtlConnection.h"
#include "Configuration.h"
#include "StreamStore.h"
#include "Util.h"

#include <spdlog/spdlog.h>

#pragma region Constructor/Destructor
template <class TConnection>
Orchestrator<TConnection>::Orchestrator(
    std::shared_ptr<IConnectionManager<TConnection>> connectionManager
) : 
    connectionManager(connectionManager)
{ }
#pragma endregion

#pragma region Public methods
template <class TConnection>
void Orchestrator<TConnection>::Init()
{
    // Initialize StreamStore
    streamStore = std::make_unique<StreamStore>();

    // Set IConnectionManager callbacks
    connectionManager->SetOnNewConnection(
        std::bind(&Orchestrator::newConnection, this, std::placeholders::_1));
}

template <class TConnection>
const std::set<std::shared_ptr<TConnection>> Orchestrator<TConnection>::GetConnections()
{
    return connections;
}
#pragma endregion

#pragma region Private methods
template <class TConnection>
void Orchestrator<TConnection>::newConnection(std::shared_ptr<TConnection> connection)
{
    spdlog::info("New connection");

    // Set IConnection callbacks
    connection->SetOnConnectionClosed(
        std::bind(&Orchestrator::connectionClosed, this, connection));
    connection->SetOnIntro(
        std::bind(
            &Orchestrator::connectionIntro,
            this,
            connection,
            std::placeholders::_1,   // versionMajor
            std::placeholders::_2,   // versionMinor
            std::placeholders::_3,   // versionRevision
            std::placeholders::_4)); // hostname
    connection->SetOnOutro(
        std::bind(
            &Orchestrator::connectionOutro,
            this,
            connection,
            std::placeholders::_1)); // reason
    connection->SetOnSubscribeChannel(
        std::bind(
            &Orchestrator::connectionSubscribeChannel,
            this,
            connection,
            std::placeholders::_1)); // channelId
    connection->SetOnUnsubscribeChannel(
        std::bind(
            &Orchestrator::connectionSubscribeChannel,
            this,
            connection,
            std::placeholders::_1)); // channelId
    connection->SetOnStreamAvailable(
        std::bind(
            &Orchestrator::connectionStreamAvailable,
            this,
            connection,
            std::placeholders::_1,   // channelId
            std::placeholders::_2,   // streamId
            std::placeholders::_3)); // hostname
    connection->SetOnStreamRemoved(
        std::bind(
            &Orchestrator::connectionStreamRemoved,
            this,
            connection,
            std::placeholders::_1,   // channelId
            std::placeholders::_2)); // streamId
    connection->SetOnStreamMetadata(
        std::bind(
            &Orchestrator::connectionStreamMetadata,
            this,
            connection,
            std::placeholders::_1,   // channelId
            std::placeholders::_2,   // streamId
            std::placeholders::_3)); // viewerCount

    std::lock_guard<std::mutex> lock(connectionsMutex);
    connections.insert(connection);
}

#pragma region Connection callback handlers
template <class TConnection>
void Orchestrator<TConnection>::connectionClosed(std::weak_ptr<TConnection> connection)
{
    if (auto strongConnection = connection.lock())
    {
        spdlog::info("Connection closed to {}", strongConnection->GetHostname());

        std::lock_guard<std::mutex> lock(connectionsMutex);
        connections.erase(strongConnection);
    }
}

template <class TConnection>
ConnectionResult Orchestrator<TConnection>::connectionIntro(
    std::weak_ptr<TConnection> connection,
    uint8_t versionMajor,
    uint8_t versionMinor,
    uint8_t versionRevision,
    std::string hostname)
{
    // TODO
    return ConnectionResult
    {
        .IsSuccess = true
    };
}

template <class TConnection>
ConnectionResult Orchestrator<TConnection>::connectionOutro(
    std::weak_ptr<TConnection> connection,
    std::string reason)
{
    // TODO
    return ConnectionResult
    {
        .IsSuccess = true
    };
}

template <class TConnection>
ConnectionResult Orchestrator<TConnection>::connectionSubscribeChannel(
    std::weak_ptr<TConnection> connection,
    ftl_channel_id_t channelId)
{
    // TODO
    return ConnectionResult
    {
        .IsSuccess = true
    };
}

template <class TConnection>
ConnectionResult Orchestrator<TConnection>::connectionUnsubscribeChannel(
    std::weak_ptr<TConnection> connection,
    ftl_channel_id_t channelId)
{
    // TODO
    return ConnectionResult
    {
        .IsSuccess = true
    };
}

template <class TConnection>
ConnectionResult Orchestrator<TConnection>::connectionStreamAvailable(
    std::weak_ptr<TConnection> connection,
    ftl_channel_id_t channelId,
    ftl_stream_id_t streamId,
    std::string hostname)
{
    // TODO
    return ConnectionResult
    {
        .IsSuccess = true
    };
}

template <class TConnection>
ConnectionResult Orchestrator<TConnection>::connectionStreamRemoved(
    std::weak_ptr<TConnection> connection,
    ftl_channel_id_t channelId,
    ftl_stream_id_t streamId)
{
    // TODO
    return ConnectionResult
    {
        .IsSuccess = true
    };
}

template <class TConnection>
ConnectionResult Orchestrator<TConnection>::connectionStreamMetadata(
    std::weak_ptr<TConnection> connection,
    ftl_channel_id_t channelId,
    ftl_stream_id_t streamId,
    uint32_t viewerCount)
{
    // TODO
    return ConnectionResult
    {
        .IsSuccess = true
    };
}
#pragma endregion /Connection callback handlers
#pragma endregion /Private methods

#pragma region Template instantiations
// Yeah, this is weird, but necessary.
// See https://stackoverflow.com/questions/495021/why-can-templates-only-be-implemented-in-the-header-file

#include "test/mocks/MockConnection.h"

template class Orchestrator<FtlConnection>;
template class Orchestrator<MockConnection>;
#pragma endregion