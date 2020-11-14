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
    // Set IConnection callbacks
    std::weak_ptr<TConnection> weakConnection(connection);
    connection->SetOnConnectionClosed(
        std::bind(&Orchestrator::connectionClosed, this, weakConnection));
    connection->SetOnIntro(
        std::bind(
            &Orchestrator::connectionIntro,
            this,
            weakConnection,
            std::placeholders::_1,   // versionMajor
            std::placeholders::_2,   // versionMinor
            std::placeholders::_3,   // versionRevision
            std::placeholders::_4)); // hostname
    connection->SetOnOutro(
        std::bind(
            &Orchestrator::connectionOutro,
            this,
            weakConnection,
            std::placeholders::_1)); // reason
    connection->SetOnSubscribeChannel(
        std::bind(
            &Orchestrator::connectionSubscribeChannel,
            this,
            weakConnection,
            std::placeholders::_1)); // channelId
    connection->SetOnUnsubscribeChannel(
        std::bind(
            &Orchestrator::connectionSubscribeChannel,
            this,
            weakConnection,
            std::placeholders::_1)); // channelId
    connection->SetOnStreamAvailable(
        std::bind(
            &Orchestrator::connectionStreamAvailable,
            this,
            weakConnection,
            std::placeholders::_1,   // channelId
            std::placeholders::_2,   // streamId
            std::placeholders::_3)); // hostname
    connection->SetOnStreamRemoved(
        std::bind(
            &Orchestrator::connectionStreamRemoved,
            this,
            weakConnection,
            std::placeholders::_1,   // channelId
            std::placeholders::_2)); // streamId
    connection->SetOnStreamMetadata(
        std::bind(
            &Orchestrator::connectionStreamMetadata,
            this,
            weakConnection,
            std::placeholders::_1,   // channelId
            std::placeholders::_2,   // streamId
            std::placeholders::_3)); // viewerCount

    // Track the connection until we receive the opening intro message
    std::lock_guard<std::mutex> lock(connectionsMutex);
    spdlog::info("New connection, awaiting intro...");
    pendingConnections.insert(connection);
}

#pragma region Connection callback handlers
template <class TConnection>
void Orchestrator<TConnection>::connectionClosed(std::weak_ptr<TConnection> connection)
{
    if (auto strongConnection = connection.lock())
    {
        spdlog::info("Connection closed to {}", strongConnection->GetHostname());
        std::lock_guard<std::mutex> lock(connectionsMutex);
        pendingConnections.erase(strongConnection);
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
    if (auto strongConnection = connection.lock())
    {
        spdlog::info("FROM {}: Intro v{}.{}.{} @ {}",
            strongConnection->GetHostname(),
            versionMajor,
            versionMinor,
            versionRevision,
            hostname);
        // Move this connection from pending to active
        std::lock_guard<std::mutex> lock(connectionsMutex);
        pendingConnections.erase(strongConnection);
        connections.insert(strongConnection);
        return ConnectionResult
        {
            .IsSuccess = true
        };
    }
    throw std::runtime_error("Lost reference to active connection!");
}

template <class TConnection>
ConnectionResult Orchestrator<TConnection>::connectionOutro(
    std::weak_ptr<TConnection> connection,
    std::string reason)
{
    if (auto strongConnection = connection.lock())
    {
        spdlog::info("FROM {}: Outro '{}'", strongConnection->GetHostname(), reason);
        return ConnectionResult
        {
            .IsSuccess = true
        };
    }
    throw std::runtime_error("Lost reference to active connection!");
}

template <class TConnection>
ConnectionResult Orchestrator<TConnection>::connectionSubscribeChannel(
    std::weak_ptr<TConnection> connection,
    ftl_channel_id_t channelId)
{
    if (auto strongConnection = connection.lock())
    {
        spdlog::info(
            "FROM {}: Subscribe to channel {}",
            strongConnection->GetHostname(),
            channelId);

        // Add the subscription
        bool addResult = subscriptions.AddSubscription(strongConnection, channelId);

        // Check if this stream is already active
        if (auto stream = streamStore.GetStreamByChannelId(channelId))
        {
            spdlog::info(
                "TO {}: Stream available, channel {} / stream {} @ ingest {}",
                stream.value().ChannelId,
                stream.value().StreamId,
                stream.value().IngestConnection->GetHostname());
            strongConnection->SendStreamAvailable(stream.value());
        }

        return ConnectionResult
        {
            .IsSuccess = addResult
        };
    }
    throw std::runtime_error("Lost reference to active connection!");
}

template <class TConnection>
ConnectionResult Orchestrator<TConnection>::connectionUnsubscribeChannel(
    std::weak_ptr<TConnection> connection,
    ftl_channel_id_t channelId)
{
    if (auto strongConnection = connection.lock())
    {
        spdlog::info(
            "FROM {}: Unsubscribe from channel {}",
            strongConnection->GetHostname(),
            channelId);

        // Remove the subscription
        bool removeResult = subscriptions.RemoveSubscription(strongConnection, channelId);

        return ConnectionResult
        {
            .IsSuccess = removeResult
        };
    }
    throw std::runtime_error("Lost reference to active connection!");
}

template <class TConnection>
ConnectionResult Orchestrator<TConnection>::connectionStreamAvailable(
    std::weak_ptr<TConnection> connection,
    ftl_channel_id_t channelId,
    ftl_stream_id_t streamId,
    std::string hostname)
{
    if (auto strongConnection = connection.lock())
    {
        spdlog::info(
            "FROM {}: Stream available, channel {} / stream {} @ ingest {}",
            strongConnection->GetHostname(),
            channelId,
            streamId,
            hostname);

        // Add it to the stream store
        // TODO: Handle existing streams
        Stream newStream
        {
            .IngestConnection = strongConnection,
            .ChannelId = channelId,
            .StreamId = streamId
        };
        streamStore.AddStream(newStream);

        // See if anyone is subscribed to this channel, and let them know
        std::set<std::shared_ptr<IConnection>> subscribedConnections = 
            subscriptions.GetSubscribedConnections(channelId);
        for (const auto& subscribedConnection : subscribedConnections)
        {
            subscribedConnection->SendStreamAvailable(newStream);
        }

        return ConnectionResult
        {
            .IsSuccess = true
        };
    }
    throw std::runtime_error("Lost reference to active connection!");
}

template <class TConnection>
ConnectionResult Orchestrator<TConnection>::connectionStreamRemoved(
    std::weak_ptr<TConnection> connection,
    ftl_channel_id_t channelId,
    ftl_stream_id_t streamId)
{
    if (auto strongConnection = connection.lock())
    {
        spdlog::info(
            "FROM {}: Stream removed, channel {} / stream {}",
            strongConnection->GetHostname(),
            channelId,
            streamId);

        // Attempt to remove it if it exists
        if (auto removedStream = streamStore.RemoveStream(channelId, streamId))
        {
            // Alert any subscribers that this stream has been removed
            std::set<std::shared_ptr<IConnection>> subscribedConnections = 
                subscriptions.GetSubscribedConnections(channelId);
            for (const auto& subscribedConnection : subscribedConnections)
            {
                subscribedConnection->SendStreamRemoved(removedStream.value());
            }

            return ConnectionResult
            {
                .IsSuccess = true
            };
        }

        spdlog::error(
            "{} indicated that stream channel {} / stream {} was removed, "
            "but this stream could not be found.",
            strongConnection->GetHostname(),
            channelId,
            streamId);
        return ConnectionResult
        {
            .IsSuccess = false
        };
    }
    throw std::runtime_error("Lost reference to active connection!");
}

template <class TConnection>
ConnectionResult Orchestrator<TConnection>::connectionStreamMetadata(
    std::weak_ptr<TConnection> connection,
    ftl_channel_id_t channelId,
    ftl_stream_id_t streamId,
    uint32_t viewerCount)
{
    if (auto strongConnection = connection.lock())
    {
        spdlog::info(
            "FROM {}: {} viewers for channel {} / stream {}",
            strongConnection->GetHostname(),
            viewerCount,
            channelId,
            streamId);

        // TODO: We ought to filter based on stream ID as well.
        if (auto stream = streamStore.GetStreamByChannelId(channelId))
        {
            stream.value().IngestConnection->SendStreamMetadata(stream.value());

            return ConnectionResult
            {
                .IsSuccess = true
            };
        }

        spdlog::error(
            "{} provided metadata for stream channel {} / stream {}, "
            "but this stream could not be found.",
            strongConnection->GetHostname(),
            channelId,
            streamId);
        return ConnectionResult
        {
            .IsSuccess = false
        };
    }
    throw std::runtime_error("Lost reference to active connection!");
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