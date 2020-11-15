/**
 * @file OrchestratorTests.cpp
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-10-25
 * @copyright Copyright (c) 2020 Hayden McAfee
 * @brief
 *  Contains unit tests for the Orchestrator class. Sort of spaghetti right now - eventually
 *  we should create a test fixture for this and add some helper methods.
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
    std::shared_ptr<MockConnectionManager<MockConnection>> connectionManager;
    std::unique_ptr<Orchestrator<MockConnection>> orchestrator;

    /**
     * @brief Initializes an Orchestrator and associated ConnectionManager
     */
    void init()
    {
        connectionManager = std::make_shared<MockConnectionManager<MockConnection>>();
        connectionManager->Init();
        orchestrator = std::make_unique<Orchestrator<MockConnection>>(connectionManager);
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
        connectionManager->MockFireNewConnection(connection);
        if (fireIntro)
        {
            connection->MockFireOnIntro(
                protocolVersionMajor,
                protocolVersionMinor,
                protocolVersionRevision,
                connection->GetHostname());
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
    mockConnection->MockFireOnIntro(0, 0, 0, "mock");

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
}

TEST_CASE_METHOD(
    OrchestratorUnitTestsFixture,
    "Orchestrator sends stream notifications to subscribed connections",
    "[orchestrator]")
{
    init();

    ftl_channel_id_t channelId = 1234;
    ftl_stream_id_t streamId = 5678;

    // Connect the edge nodes and have them subscribe to updates for this channel
    const size_t numEdgeConnections = 3;
    auto edgeConnections = generateAndConnectMockConnections("edge", numEdgeConnections);

    // Subscribe to updates for a channel
    for (const auto& connection : edgeConnections)
    {
        connection->MockFireOnSubscribeChannel(channelId);
    }

    // Connect the ingest and have it report the stream
    auto ingest = generateAndConnectMockConnection("ingest");
    ingest->MockFireOnStreamAvailable(channelId, streamId, "ingest");

    // Make sure the edge nodes saw the new channel
    for (const auto& connection : edgeConnections)
    {
        REQUIRE(connection->IsStreamAvailable(channelId, streamId) == true);
    }

    // Have ingest report the stream has been removed
    ingest->MockFireOnStreamRemoved(channelId, streamId);
    
    // Make sure edge nodes see the stream has been removed
    for (const auto& connection : edgeConnections)
    {
        REQUIRE(connection->IsStreamAvailable(channelId, streamId) == false);
    }
}

TEST_CASE_METHOD(
    OrchestratorUnitTestsFixture,
    "Orchestrator sends notifications for existing streams to new subscribers",
    "[orchestrator]")
{
    init();

    ftl_channel_id_t channelId = 1234;
    ftl_stream_id_t streamId = 5678;

    // Connect the ingest, and have it indicate that the stream is available.
    auto ingest = generateAndConnectMockConnection("ingest");
    ingest->MockFireOnStreamAvailable(channelId, streamId, "ingest");

    // Connect the edge, subscribe to the stream
    auto edge = generateAndConnectMockConnection("edge");
    edge->MockFireOnSubscribeChannel(channelId);

    // Did we get the stream available notification?
    REQUIRE(edge->IsStreamAvailable(channelId, streamId) == true);
}

TEST_CASE_METHOD(
    OrchestratorUnitTestsFixture,
    "Orchestrator sends unsubscribes for streams when the ingest has disconnected",
    "[orchestrator]")
{
    init();

    ftl_channel_id_t channelId = 1234;
    ftl_stream_id_t streamId = 5678;

    // Connect a few edge nodes and subscribe them
    auto edges = generateAndConnectMockConnections("edge", 3);
    for (const auto& edge : edges)
    {
        edge->MockFireOnSubscribeChannel(channelId);
    }

    // Connect the ingest
    auto ingest = generateAndConnectMockConnection("ingest");

    // Make channel live
    ingest->MockFireOnStreamAvailable(channelId, streamId, "ingest");

    // Disconnect the ingest
    ingest->MockFireOnConnectionClosed();

    // Make sure the streams went away
    for (const auto& edge : edges)
    {
        REQUIRE(edge->IsStreamAvailable(channelId, streamId) == false);
    }
}

TEST_CASE_METHOD(
    OrchestratorUnitTestsFixture,
    "Orchestrator sends metadata updates to original ingest",
    "[orchestrator]")
{
    init();

    ftl_channel_id_t channelId = 1234;
    ftl_stream_id_t streamId = 5678;

    // Connect a few edge nodes and subscribe them
    int numEdges = 3;
    auto edges = generateAndConnectMockConnections("edge", numEdges);
    for (const auto& edge : edges)
    {
        edge->MockFireOnSubscribeChannel(channelId);
    }

    // Connect the ingest
    int viewersReported = 0;
    auto ingest = generateAndConnectMockConnection("ingest");
    ingest->SetMockOnSendStreamMetadata(
        [&viewersReported](Stream stream, uint32_t viewerCount)
        {
            viewersReported += viewerCount;
        });
    ingest->MockFireOnStreamAvailable(channelId, streamId, "ingest");
    
    // Report one viewer per edge
    for (const auto& edge : edges)
    {
        edge->MockFireOnStreamMetadata(channelId, streamId, 1);
    }

    // Confirm we got 1 viewer per edge
    REQUIRE(viewersReported == numEdges);
}

// TODO: Test cases to cover orchestrator logic