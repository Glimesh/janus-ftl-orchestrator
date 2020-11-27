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

#include <list>

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

template <class TConnection>
std::set<ftl_channel_id_t> Orchestrator<TConnection>::GetSubscribedChannels(
    std::shared_ptr<TConnection> connection)
{
    std::set<ftl_channel_id_t> returnVal;
    for (const auto& sub : subscriptions.GetSubscriptions(connection))
    {
        returnVal.insert(sub->ChannelId);
    }
    return returnVal;
}
#pragma endregion

#pragma region Private methods
template <class TConnection>
void Orchestrator<TConnection>::openRoute(
    Stream<TConnection> stream,
    std::shared_ptr<TConnection> edgeConnection,
    std::vector<std::byte> streamKey)
{
    // TODO: Handle relays, generate real routes.
    // for now, we just tell the ingest to start relaying directly to the edge node.
    stream.IngestConnection->SendStreamRelay(ConnectionRelayPayload
        {
            .IsStartRelay = true,
            .ChannelId = stream.ChannelId,
            .StreamId = stream.StreamId,
            .TargetHostname = edgeConnection->GetHostname(),
            .StreamKey = streamKey,
        });
}

template <class TConnection>
void Orchestrator<TConnection>::closeRoute(
    Stream<TConnection> stream,
    std::shared_ptr<TConnection> edgeConnection)
{
    // TODO: Handle relays, generate real routes.
    // for now, we just tell the ingest to stop relaying directly to the edge node.
    stream.IngestConnection->SendStreamRelay(ConnectionRelayPayload
        {
            .IsStartRelay = false,
            .ChannelId = stream.ChannelId,
            .StreamId = stream.StreamId,
            .TargetHostname = edgeConnection->GetHostname(),
            .StreamKey = std::vector<std::byte>(),
        });
}

template <class TConnection>
void Orchestrator<TConnection>::newConnection(std::shared_ptr<TConnection> connection)
{
    // Set IConnection callbacks
    // We use weak references to the connection to avoid circular references since these get captured
    // as part of the callback that's stored on the connection. Otherwise, the ref count would never
    // hit 0, and the connection would never be destructed.
    std::weak_ptr<TConnection> weakConnection(connection);
    connection->SetOnConnectionClosed(
        std::bind(&Orchestrator::connectionClosed, this, weakConnection));
    connection->SetOnIntro(
        std::bind(
            &Orchestrator::connectionIntro,
            this,
            weakConnection,
            std::placeholders::_1));
    connection->SetOnOutro(
        std::bind(
            &Orchestrator::connectionOutro,
            this,
            weakConnection,
            std::placeholders::_1));
    connection->SetOnNodeState(
        std::bind(
            &Orchestrator::connectionNodeState,
            this,
            weakConnection,
            std::placeholders::_1));
    connection->SetOnChannelSubscription(
        std::bind(
            &Orchestrator::connectionChannelSubscription,
            this,
            weakConnection,
            std::placeholders::_1));
    connection->SetOnStreamPublish(
        std::bind(
            &Orchestrator::connectionStreamPublish,
            this,
            weakConnection,
            std::placeholders::_1));
    connection->SetOnStreamRelay(
        std::bind(
            &Orchestrator::connectionStreamRelay,
            this,
            weakConnection,
            std::placeholders::_1));

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

        // First, clear any active routes to this connection
        for (const auto& sub : subscriptions.GetSubscriptions(strongConnection))
        {
            if (auto stream = streamStore.GetStreamByChannelId(sub->ChannelId))
            {
                closeRoute(stream.value(), strongConnection);
            }
        }

        // Remove all streams associated with this connection
        streamStore.RemoveAllConnectionStreams(strongConnection);
        // Remove all subscriptions associated with this connetion
        subscriptions.ClearSubscriptions(strongConnection);

        std::lock_guard<std::mutex> lock(connectionsMutex);
        pendingConnections.erase(strongConnection);
        connections.erase(strongConnection);
    }
}

template <class TConnection>
ConnectionResult Orchestrator<TConnection>::connectionIntro(
    std::weak_ptr<TConnection> connection,
    ConnectionIntroPayload payload)
{
    if (auto strongConnection = connection.lock())
    {
        spdlog::info("FROM {}: Intro from {} v{}.{}.{}, Layer {}, Region {}",
            strongConnection->GetHostname(),
            payload.Hostname,
            payload.VersionMajor,
            payload.VersionMinor,
            payload.VersionRevision,
            payload.RelayLayer,
            payload.RegionCode);

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
    ConnectionOutroPayload payload)
{
    if (auto strongConnection = connection.lock())
    {
        spdlog::info(
            "FROM {}: Outro '{}'",
            strongConnection->GetHostname(),
            payload.DisconnectReason);
        return ConnectionResult
        {
            .IsSuccess = true
        };
    }
    throw std::runtime_error("Lost reference to active connection!");
}

template <class TConnection>
ConnectionResult Orchestrator<TConnection>::connectionNodeState(
    std::weak_ptr<TConnection> connection,
    ConnectionNodeStatePayload payload)
{
    if (auto strongConnection = connection.lock())
    {
        spdlog::info(
            "FROM {}: Node State, load: {} / {}",
            strongConnection->GetHostname(),
            payload.CurrentLoad,
            payload.MaximumLoad);
        return ConnectionResult
        {
            .IsSuccess = true
        };
    }
    throw std::runtime_error("Lost reference to active connection!");
}

template <class TConnection>
ConnectionResult Orchestrator<TConnection>::connectionChannelSubscription(
    std::weak_ptr<TConnection> connection,
    ConnectionSubscriptionPayload payload)
{
    if (auto strongConnection = connection.lock())
    {
        if (payload.IsSubscribe)
        {
            spdlog::info(
                "FROM {}: Subscribe to channel {}",
                strongConnection->GetHostname(),
                payload.ChannelId);

            // Add the subscription
            bool addResult = subscriptions.AddSubscription(
                strongConnection,
                payload.ChannelId,
                payload.StreamKey);
            if (!addResult)
            {
                return ConnectionResult
                {
                    .IsSuccess = false
                };
            }

            // Check if this stream is already active
            if (auto stream = streamStore.GetStreamByChannelId(payload.ChannelId))
            {
                // Establish a route to this edge node
                openRoute(stream.value(), strongConnection, payload.StreamKey);
            }

            return ConnectionResult
            {
                .IsSuccess = addResult
            };
        }
        else
        {
            spdlog::info(
                "FROM {}: Unsubscribe from channel {}",
                strongConnection->GetHostname(),
                payload.ChannelId);

            // Check if this stream is currently active
            if (auto stream = streamStore.GetStreamByChannelId(payload.ChannelId))
            {
                // Close any existing route
                closeRoute(stream.value(), strongConnection);
            }

            // Remove the subscription
            bool removeResult = 
                subscriptions.RemoveSubscription(strongConnection, payload.ChannelId);

            return ConnectionResult
            {
                .IsSuccess = removeResult
            };
        }
    }
    throw std::runtime_error("Lost reference to active connection!");
}

template <class TConnection>
ConnectionResult Orchestrator<TConnection>::connectionStreamPublish(
    std::weak_ptr<TConnection> connection,
    ConnectionPublishPayload payload)
{
    if (auto strongConnection = connection.lock())
    {
        if (payload.IsPublish)
        {
            spdlog::info(
                "FROM {}: Stream published, channel {} / stream {}",
                strongConnection->GetHostname(),
                payload.ChannelId,
                payload.StreamId);

            // Add it to the stream store
            // TODO: Handle existing streams
            Stream newStream
            {
                .IngestConnection = strongConnection,
                .ChannelId = payload.ChannelId,
                .StreamId = payload.StreamId,
            };
            streamStore.AddStream(newStream);

            // Start opening relays to any subscribed connections
            for (const auto& subscription : subscriptions.GetSubscriptions(payload.ChannelId))
            {
                openRoute(newStream, subscription->SubscribedConnection, subscription->StreamKey);
            }

            return ConnectionResult
            {
                .IsSuccess = true
            };
        }
        else
        {
            spdlog::info(
                "FROM {}: Stream unpublished, channel {} / stream {}",
                strongConnection->GetHostname(),
                payload.ChannelId,
                payload.StreamId);

            // Attempt to remove it if it exists
            if (auto removedStream = streamStore.RemoveStream(payload.ChannelId, payload.StreamId))
            {
                return ConnectionResult
                {
                    .IsSuccess = true
                };
            }

            spdlog::error(
                "{} indicated that stream channel {} / stream {} was removed, "
                "but this stream could not be found.",
                strongConnection->GetHostname(),
                payload.ChannelId,
                payload.StreamId);
            return ConnectionResult
            {
                .IsSuccess = false
            };
        }
    }
    throw std::runtime_error("Lost reference to active connection!");
}

template <class TConnection>
ConnectionResult Orchestrator<TConnection>::connectionStreamRelay(
    std::weak_ptr<TConnection> connection,
    ConnectionRelayPayload payload)
{
    if (auto strongConnection = connection.lock())
    {
        // TODO
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