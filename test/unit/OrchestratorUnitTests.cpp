/**
 * @file OrchestratorTests.cpp
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-10-25
 * @copyright Copyright (c) 2020 Hayden McAfee
 * @brief Contains unit tests for the Orchestrator class. 
 */

#include <catch2/catch.hpp>

#include <array>
#include <memory>
#include <sstream>
#include <vector>

#include "../mocks/MockConnectionManager.h"
#include "../mocks/MockConnection.h"

#include "../../src/Orchestrator.h"

/**
 * @brief Test fixture to expose some convenient helpers shared between tests
 */
class OrchestratorUnitTestsFixture
{
public:
    OrchestratorUnitTestsFixture()
    { }

protected:
    static const uint8_t protocolVersionMajor = 0;
    static const uint8_t protocolVersionMinor = 0;
    static const uint8_t protocolVersionRevision = 0;
    std::unique_ptr<Orchestrator<MockConnection>> orchestrator;

    /**
     * @brief Initializes an Orchestrator and associated ConnectionManager
     */
    void init()
    {
        orchestrator = std::make_unique<Orchestrator<MockConnection>>(
            std::make_unique<MockConnectionManager<MockConnection>>());
        orchestrator->Init();
    }

    /**
     * @brief Generates a single mock connection
     * @param hostname hostname to assign the mock connection
     * @return std::shared_ptr<MockConnection> the mock connection
     */
    std::shared_ptr<MockConnection> generateMockConnection(const std::string& hostname)
    {
        return std::make_shared<MockConnection>(hostname);
    }

    /**
     * @brief Generates a set of mock connections based on the given parameters
     * @param hostnamePrefix hostname to give each connection, a count will be appended
     * @param count the number of connections to generate
     * @return std::vector<std::shared_ptr<MockConnection>> connections
     */
    std::vector<std::shared_ptr<MockConnection>> generateMockConnections(
        const std::string& hostnamePrefix,
        const int& count)
    {
        auto returnVal = std::vector<std::shared_ptr<MockConnection>>();
        for (int i = 0; i < count; ++i)
        {
            std::stringstream hostname;
            hostname << hostnamePrefix << "-" << i;
            returnVal.emplace_back(std::make_shared<MockConnection>(hostname.str()));
        }
        return returnVal;
    }

    /**
     * @brief Given a mock connection, connects it to the Orchestrator instance
     * @param connections connection to connect to Orchestrator
     * @param fireIntro whether to send an intro request when connected
     */
    void connectMockConnection(
        const std::shared_ptr<MockConnection>& connection,
        bool fireIntro = true)
    {
        const auto& connectionManager = 
            reinterpret_cast<const std::unique_ptr<MockConnectionManager<MockConnection>>&>(
                orchestrator->GetConnectionManager());
        connectionManager->MockFireNewConnection(connection);
        if (fireIntro)
        {
            connection->MockFireOnIntro(
                {
                    .VersionMajor = protocolVersionMajor,
                    .VersionMinor = protocolVersionMinor,
                    .VersionRevision = protocolVersionRevision,
                    .RelayLayer = 0,
                    .Hostname = connection->GetHostname(),
                });
        }
    }

    /**
     * @brief Given a set of mock connections, connects them to the Orchestrator instance
     * @param connections connections to connect to Orchestrator
     * @param fireIntro whether to send an intro request when connected
     */
    void connectMockConnections(
        const std::vector<std::shared_ptr<MockConnection>>& connections,
        bool fireIntro = true)
    {
        for (const auto& connection : connections)
        {
            connectMockConnection(connection, fireIntro);
        }
    }

    /**
     * @brief
     *  Generates a mock connection based on the given parameters, and connects it
     *  to the Orchestrator instance.
     * 
     * @param hostname hostname to give the connection
     * @param fireIntro whether to send an intro request when connected
     * @return std::shared_ptr<MockConnection> connection
     */
    std::shared_ptr<MockConnection> generateAndConnectMockConnection(
        const std::string& hostname,
        bool fireIntro = true)
    {
        auto connection = generateMockConnection(hostname);
        connectMockConnection(connection, fireIntro);
        return connection;
    }

    /**
     * @brief
     *  Generates a set of mock connections based on the given parameters, and connects them
     *  to the Orchestrator instance.
     * 
     * @param hostnamePrefix hostname to give each connection, a count will be appended
     * @param count the number of connections to generate
     * @param fireIntro whether to send an intro request when connected
     * @return std::vector<std::shared_ptr<MockConnection>> connections
     */
    std::vector<std::shared_ptr<MockConnection>> generateAndConnectMockConnections(
        const std::string& hostnamePrefix,
        const int& count,
        bool fireIntro = true)
    {
        auto connections = generateMockConnections(hostnamePrefix, count);
        connectMockConnections(connections, fireIntro);
        return connections;
    }
};

TEST_CASE_METHOD(
    OrchestratorUnitTestsFixture,
    "Orchestrator basic connect/disconnect",
    "[orchestrator]")
{
    init();
    
    // We should start off with zero connections
    REQUIRE(orchestrator->GetConnections().size() == 0);

    // Let's register one
    bool mockConnectionDestructed = false;
    std::shared_ptr<MockConnection> mockConnection = generateMockConnection("mock");
    mockConnection->SetMockOnDestructed(
        [&mockConnectionDestructed]()
        {
            mockConnectionDestructed = true;
        });
    connectMockConnection(mockConnection, false);

    // At this point, we haven't sent an intro message, so we shouldn't be counted
    REQUIRE(orchestrator->GetConnections().count(mockConnection) == 0);

    // Fire mock intro message
    mockConnection->MockFireOnIntro(
        {
            .VersionMajor = protocolVersionMajor,
            .VersionMinor = protocolVersionMinor,
            .VersionRevision = protocolVersionRevision,
            .RelayLayer = 0,
            .Hostname = mockConnection->GetHostname(),
        });

    // Check that our mock connection is now counted
    REQUIRE(orchestrator->GetConnections().count(mockConnection) == 1);

    // TODO: Outro

    // Close the connection and make sure it's removed
    mockConnection->MockFireOnConnectionClosed();
    REQUIRE(orchestrator->GetConnections().count(mockConnection) == 0);
    REQUIRE(orchestrator->GetConnections().size() == 0);

    // Remove our reference and ensure that the connection is destructed
    mockConnection = nullptr;
    REQUIRE(mockConnectionDestructed == true);
}

TEST_CASE_METHOD(
    OrchestratorUnitTestsFixture,
    "Connections that subscribe/unsubscribe are tracked properly",
    "[orchestrator]")
{
    init();

    ftl_channel_id_t channelIds[] = { 1234, 5678, 2468, 3690 };

    // Connect edge nodes
    const size_t numEdgeConnections = 3;
    auto edgeConnections = generateAndConnectMockConnections("edge", numEdgeConnections);

    // Have each node subscribe to all the channels
    for (const auto& connection : edgeConnections)
    {
        for (const auto& channelId : channelIds)
        {
            connection->MockFireOnChannelSubscription(
                {
                    .IsSubscribe = true,
                    .ChannelId = channelId,
                    .StreamKey = std::vector<std::byte>(),
                });
        }
    }

    // Check that all the subscriptions are there
    for (const auto& connection : edgeConnections)
    {
        std::set<ftl_channel_id_t> subs = orchestrator->GetSubscribedChannels(connection);
        for (const auto& channelId : channelIds)
        {
            REQUIRE(subs.contains(channelId));
        }
    }

    // Now unsubscribe
    for (const auto& connection : edgeConnections)
    {
        for (const auto& channelId : channelIds)
        {
            connection->MockFireOnChannelSubscription(
                {
                    .IsSubscribe = false,
                    .ChannelId = channelId,
                    .StreamKey = std::vector<std::byte>(),
                });
        }
    }

    // Make sure the subscriptions are gone
    for (const auto& connection : edgeConnections)
    {
        std::set<ftl_channel_id_t> subs = orchestrator->GetSubscribedChannels(connection);
        REQUIRE(subs.size() == 0);
    }
}

TEST_CASE_METHOD(
    OrchestratorUnitTestsFixture,
    "Orchestrator relays streams from Ingest nodes to Edge nodes when there are no Relay nodes",
    "[orchestrator]")
{
    init();

    ftl_channel_id_t channelId = 1234;
    ftl_stream_id_t streamId = 5678;
    std::vector<std::byte> streamKey = 
        {
            std::byte{0x00}, std::byte{0x01}, std::byte{0x02}, std::byte{0x03},
            std::byte{0x04}, std::byte{0x05}, std::byte{0x06}, std::byte{0x07},
        };

    // Connect the edge nodes and have them subscribe to updates for this channel
    const size_t numEdgeConnections = 3;
    auto edgeConnections = generateAndConnectMockConnections("edge", numEdgeConnections);

    // Subscribe to updates for a channel
    for (const auto& connection : edgeConnections)
    {
        connection->MockFireOnChannelSubscription(
            {
                .IsSubscribe = true,
                .ChannelId = channelId,
                .StreamKey = streamKey,
            });
    }

    // Connect the ingest and have it report the stream
    auto ingest = generateAndConnectMockConnection("ingest");
    std::vector<ConnectionRelayPayload> recvRelayPayloads;
    ingest->SetOnStreamRelay(
        [&recvRelayPayloads](ConnectionRelayPayload payload)
        {
            recvRelayPayloads.push_back(payload);
            return ConnectionResult
            {
                .IsSuccess = true
            };
        });
    ingest->MockFireOnStreamPublish(
        {
            .IsPublish = true,
            .ChannelId = channelId,
            .StreamId = streamId,
        });

    // Ensure the ingest has been instructed to relay the stream to all of the subscribing edges
    for (const auto& connection : edgeConnections)
    {
        bool connectionWasRelayedTo = std::any_of(
            recvRelayPayloads.begin(),
            recvRelayPayloads.end(),
            [&connection, &channelId, &streamId, &streamKey](ConnectionRelayPayload payload)
            {
                return (payload.IsStartRelay) &&
                    (payload.TargetHostname == connection->GetHostname()) &&
                    (payload.ChannelId == channelId) &&
                    (payload.StreamId == streamId) && 
                    (payload.StreamKey == streamKey);
            });
        REQUIRE(connectionWasRelayedTo == true);
    }
    recvRelayPayloads.clear();

    // Unsubscribe the first edge node and make sure the ingest is instructed to stop relaying
    const auto& unSubEdge = edgeConnections.at(0);
    unSubEdge->MockFireOnChannelSubscription(
        {
            .IsSubscribe = false,
            .ChannelId = channelId,
            .StreamKey = std::vector<std::byte>(),
        });
    REQUIRE(recvRelayPayloads.size() == 1);
    const auto& unSubPayload = recvRelayPayloads.at(0);
    REQUIRE(unSubPayload.TargetHostname == unSubEdge->GetHostname());
    REQUIRE(unSubPayload.IsStartRelay == false);
    REQUIRE(unSubPayload.ChannelId == channelId);
    recvRelayPayloads.clear();

    // Disconnect the second edge node and make sure the ingest is instructed to stop relaying
    const auto& disconnectEdge = edgeConnections.at(1);
    disconnectEdge->MockFireOnConnectionClosed();
    REQUIRE(recvRelayPayloads.size() == 1);
    const auto& disconnectPayload = recvRelayPayloads.at(0);
    REQUIRE(disconnectPayload.TargetHostname == disconnectEdge->GetHostname());
    REQUIRE(disconnectPayload.IsStartRelay == false);
    REQUIRE(disconnectPayload.ChannelId == channelId);
    recvRelayPayloads.clear();
}

// TODO: Test cases to cover orchestrator/routing logic