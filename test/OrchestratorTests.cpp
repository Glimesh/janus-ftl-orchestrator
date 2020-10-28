/**
 * @file OrchestratorTests.cpp
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-10-25
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#include <catch2/catch.hpp>
#include <memory>

#include "mocks/MockConnectionManager.h"
#include "mocks/MockConnection.h"

#include "../Orchestrator.h"

TEST_CASE("Orchestrator keeps track of new connections and closed connections", "[orchestrator]")
{
    std::shared_ptr<MockConnectionManager> mockConnectionManager =
        std::make_shared<MockConnectionManager>();
    std::unique_ptr<Orchestrator> orchestrator =
        std::make_unique<Orchestrator>(mockConnectionManager);
    orchestrator->Init();
    
    // We should start off with zero connections
    REQUIRE(orchestrator->GetConnections().size() == 0);

    // Let's pretend to register a few.
    std::shared_ptr<MockConnection> mockIngestOne = 
        std::make_shared<MockConnection>("mock-ingest-one", FtlServerKind::Ingest);
    mockConnectionManager->MockFireNewConnection(mockIngestOne);
    REQUIRE(orchestrator->GetConnections().count(mockIngestOne) == 1);
    std::shared_ptr<MockConnection> mockEdgeOne = 
        std::make_shared<MockConnection>("mock-edge-one", FtlServerKind::Edge);
    mockConnectionManager->MockFireNewConnection(mockEdgeOne);
    REQUIRE(orchestrator->GetConnections().count(mockEdgeOne) == 1);
    std::shared_ptr<MockConnection> mockIngestTwo = 
        std::make_shared<MockConnection>("mock-ingest-two", FtlServerKind::Ingest);
    mockConnectionManager->MockFireNewConnection(mockIngestTwo);
    REQUIRE(orchestrator->GetConnections().count(mockIngestTwo) == 1);
    std::shared_ptr<MockConnection> mockEdgeTwo = 
        std::make_shared<MockConnection>("mock-edge-two", FtlServerKind::Edge);
    mockConnectionManager->MockFireNewConnection(mockEdgeTwo);
    REQUIRE(orchestrator->GetConnections().count(mockEdgeTwo) == 1);

    REQUIRE(orchestrator->GetConnections().size() == 4);

    // Close mock connections and make sure orchestrator responds correctly.
    mockIngestOne->MockFireOnConnectionClosed();
    REQUIRE(orchestrator->GetConnections().count(mockIngestOne) == 0);
    mockEdgeOne->MockFireOnConnectionClosed();
    REQUIRE(orchestrator->GetConnections().count(mockEdgeOne) == 0);
    mockIngestTwo->MockFireOnConnectionClosed();
    REQUIRE(orchestrator->GetConnections().count(mockIngestTwo) == 0);
    mockEdgeTwo->MockFireOnConnectionClosed();
    REQUIRE(orchestrator->GetConnections().count(mockEdgeTwo) == 0);

    REQUIRE(orchestrator->GetConnections().size() == 0);
}

TEST_CASE("New and ended streams are properly communicated between connections", "[orchestrator]")
{
    // Set up orchestrator instance
    std::shared_ptr<MockConnectionManager> mockConnectionManager = 
        std::make_shared<MockConnectionManager>();
    std::unique_ptr<Orchestrator> orchestrator = 
        std::make_unique<Orchestrator>(mockConnectionManager);
    orchestrator->Init();

    // Set up a few mock ingest connections
    std::shared_ptr<MockConnection> mockIngestAlpha = 
        std::make_shared<MockConnection>("mock-ingest-alpha", FtlServerKind::Ingest);
    std::shared_ptr<MockConnection> mockIngestBravo = 
        std::make_shared<MockConnection>("mock-ingest-bravo", FtlServerKind::Ingest);

    // Set up a few mock edge connections
    std::shared_ptr<MockConnection> mockEdgeAlpha = 
        std::make_shared<MockConnection>("mock-edge-alpha", FtlServerKind::Edge);
    std::shared_ptr<MockConnection> mockEdgeBravo = 
        std::make_shared<MockConnection>("mock-edge-bravo", FtlServerKind::Edge);
    std::shared_ptr<MockConnection> mockEdgeCharlie = 
        std::make_shared<MockConnection>("mock-edge-charlie", FtlServerKind::Edge);

    // Register connections
    mockConnectionManager->MockFireNewConnection(mockIngestAlpha);
    mockConnectionManager->MockFireNewConnection(mockIngestBravo);
    mockConnectionManager->MockFireNewConnection(mockEdgeAlpha);
    mockConnectionManager->MockFireNewConnection(mockEdgeBravo);
    mockConnectionManager->MockFireNewConnection(mockEdgeCharlie);

    // Have ingest alpha report a new ingest, and make sure this is reported to the edge connections
    ftl_channel_id_t newChannelId = 1234;
    ftl_stream_id_t newStreamId = 5678;
    bool mockEdgeAlphaSawNewStream = false;
    bool mockEdgeBravoSawNewStream = false;
    bool mockEdgeCharlieSawNewStream = false;
    mockEdgeAlpha->SetOnIngestNewStream(
        [&mockEdgeAlphaSawNewStream, newChannelId, newStreamId]
        (ftl_channel_id_t channelId, ftl_stream_id_t streamId)
        {
            mockEdgeAlphaSawNewStream = (channelId == newChannelId && streamId == newStreamId);
        });
    mockEdgeBravo->SetOnIngestNewStream(
        [&mockEdgeBravoSawNewStream, newChannelId, newStreamId]
        (ftl_channel_id_t channelId, ftl_stream_id_t streamId)
        {
            mockEdgeBravoSawNewStream = (channelId == newChannelId && streamId == newStreamId);
        });
    mockEdgeCharlie->SetOnIngestNewStream(
        [&mockEdgeCharlieSawNewStream, newChannelId, newStreamId]
        (ftl_channel_id_t channelId, ftl_stream_id_t streamId)
        {
            mockEdgeCharlieSawNewStream = (channelId == newChannelId && streamId == newStreamId);
        });
    mockIngestAlpha->MockFireOnIngestNewStream(newChannelId, newStreamId);

    REQUIRE(mockEdgeAlphaSawNewStream);
    REQUIRE(mockEdgeBravoSawNewStream);
    REQUIRE(mockEdgeCharlieSawNewStream);
}