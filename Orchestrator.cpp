/**
 * @file Orchestrator.cpp
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-10-18
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#include "Orchestrator.h"

#include "FtlConnection.h"
#include "IConnection.h"
#include "IConnectionManager.h"
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
    connection->SetOnStreamAvailable(
        std::bind(
            &Orchestrator::streamAvailable,
            this,
            connection,
            std::placeholders::_1,
            std::placeholders::_2));

    std::lock_guard<std::mutex> lock(connectionsMutex);
    connections.insert(connection);
}

template <class TConnection>
void Orchestrator<TConnection>::connectionClosed(std::shared_ptr<TConnection> connection)
{
    spdlog::info("Connection closed to {}", connection->GetHostname());

    std::lock_guard<std::mutex> lock(connectionsMutex);
    connections.erase(connection);
}

template <class TConnection>
void Orchestrator<TConnection>::streamAvailable(
    std::shared_ptr<TConnection> connection,
    ftl_channel_id_t channelId,
    ftl_stream_id_t streamId)
{
    // Create new Stream object to track this stream, then add to stream store
    std::shared_ptr<Stream> stream = std::make_shared<Stream>(connection, channelId, streamId);
    streamStore->AddStream(stream);

    // TODO: Notify other connections that a new stream is available
}

template <class TConnection>
void Orchestrator<TConnection>::streamRemoved(
    std::shared_ptr<TConnection> connection,
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

template <class TConnection>
void Orchestrator<TConnection>::streamMetadata(
    std::shared_ptr<TConnection> connection,
    ftl_channel_id_t channelId,
    ftl_stream_id_t streamId,
    uint32_t viewerCount)
{
    
}
#pragma endregion

#pragma region Template instantiations
// Yeah, this is weird, but necessary.
// See https://stackoverflow.com/questions/495021/why-can-templates-only-be-implemented-in-the-header-file

#include "test/mocks/MockConnection.h"

template class Orchestrator<FtlConnection>;
template class Orchestrator<MockConnection>;
#pragma endregion