/**
 * @file OrchestratorTests.cpp
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-10-25
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#include <catch2/catch.hpp>
#include <memory>

#include "../mocks/MockConnectionManager.h"
#include "../mocks/MockConnection.h"

#include "../../src/Orchestrator.h"

TEST_CASE("Orchestrator basic connect/disconnect", "[orchestrator]")
{
    auto mockConnectionManager = std::make_shared<MockConnectionManager<MockConnection>>();
    mockConnectionManager->Init();
    auto orchestrator = std::make_unique<Orchestrator<MockConnection>>(mockConnectionManager);
    orchestrator->Init();
    
    // We should start off with zero connections
    REQUIRE(orchestrator->GetConnections().size() == 0);

    // Let's register one
    bool mockConnectionDestructed = false;
    std::shared_ptr<MockConnection> mockConnection = std::make_shared<MockConnection>("mock");
    mockConnection->SetMockOnDestructed(
        [&mockConnectionDestructed]()
        {
            mockConnectionDestructed = true;
        });
    mockConnectionManager->MockFireNewConnection(mockConnection);

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

TEST_CASE("Orchestrator sends stream notifications immediately on subscribe", "[orchestrator]")
{
    auto mockConnectionManager = std::make_shared<MockConnectionManager<MockConnection>>();
    mockConnectionManager->Init();
    auto orchestrator = std::make_unique<Orchestrator<MockConnection>>(mockConnectionManager);
    orchestrator->Init();

    ftl_channel_id_t channelId = 1234;
    ftl_stream_id_t streamId = 5678;

    // Connect the ingest, and have it indicate that the stream is available.
    auto mockIngest = std::make_shared<MockConnection>("ingest");
    mockConnectionManager->MockFireNewConnection(mockIngest);
    mockIngest->MockFireOnIntro(0, 0, 0, "ingest");
    mockIngest->MockFireOnStreamAvailable(channelId, streamId, "ingest");

    // Connect the edge, subscribe to the stream
    auto mockEdge = std::make_shared<MockConnection>("edge");
    bool edgeReceivedStream = false;
    mockEdge->SetMockOnSendStreamAvailable(
        [&edgeReceivedStream, &channelId, &streamId](Stream stream)
        {
            if ((stream.ChannelId == channelId) && (stream.StreamId == streamId))
            {
                edgeReceivedStream = true;
            }
        });
    mockConnectionManager->MockFireNewConnection(mockEdge);
    mockEdge->MockFireOnIntro(0, 0, 0, "edge");
    mockEdge->MockFireOnSubscribeChannel(channelId);

    // Did we get the stream available notification?
    REQUIRE(edgeReceivedStream == true);
}

// TODO: Test cases to cover orchestrator logic