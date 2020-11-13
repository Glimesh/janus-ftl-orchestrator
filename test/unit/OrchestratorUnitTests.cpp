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
    auto orchestrator = std::make_unique<Orchestrator<MockConnection>>(mockConnectionManager);
    orchestrator->Init();
    
    // We should start off with zero connections
    REQUIRE(orchestrator->GetConnections().size() == 0);

    // Let's register one
    std::shared_ptr<MockConnection> mockConnection = std::make_shared<MockConnection>("mock");
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
}

// TODO: Test cases to cover orchestrator logic