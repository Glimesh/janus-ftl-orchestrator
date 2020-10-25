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