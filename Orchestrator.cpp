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
#include "StreamStore.h"
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
    // Initialize StreamStore
    streamStore = std::make_unique<StreamStore>();

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
    connection->SetOnIngestNewStream(
        std::bind(
            &Orchestrator::ingestNewStream,
            this,
            connection,
            std::placeholders::_1,
            std::placeholders::_2));

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

void Orchestrator::ingestNewStream(
    std::shared_ptr<IConnection> connection,
    ftl_channel_id_t channelId,
    ftl_stream_id_t streamId)
{
    // Create new Stream object to track this stream, then add to stream store
    std::shared_ptr<Stream> stream = std::make_shared<Stream>(connection, channelId, streamId);
    streamStore->AddStream(stream);

    // TODO: Notify other connections that a new stream is available
}

void Orchestrator::ingestStreamEnded(
    std::shared_ptr<IConnection> connection,
    ftl_channel_id_t channelId,
    ftl_stream_id_t streamId)
{
    // Remove the Stream from the stream store
    if (!streamStore->RemoveStream(channelId, streamId))
    {
        spdlog::error(
            "Couldn't find stream {}/{} indicated removed by {}",
            channelId,
            streamId,
            connection->GetHostname());
    }

    // TODO: Notify other connections that this stream has ended
}

void Orchestrator::streamViewersUpdated(
    std::shared_ptr<IConnection> connection,
    ftl_channel_id_t channelId,
    ftl_stream_id_t streamId,
    uint32_t viewerCount)
{
    
}
#pragma endregion