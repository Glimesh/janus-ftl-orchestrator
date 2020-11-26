/**
 * @file FunctionalTests.cpp
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-11-25
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#include <catch2/catch.hpp>

#include <FtlOrchestrationClient.h>
#include <TlsConnectionManager.h>

#include <chrono>
#include <condition_variable>
#include <list>
#include <mutex>
#include <thread>
#include <vector>

/**
 * @brief
 *  A class providing a variety of tools for managing functional testing of Orchestrator
 *  application classes.
 */
class FunctionalTestsFixture
{
public:
    FunctionalTestsFixture() : 
        preSharedKey(
            {
                std::byte(0x00), std::byte(0x01), std::byte(0x02), std::byte(0x03), 
                std::byte(0x04), std::byte(0x05), std::byte(0x06), std::byte(0x07), 
                std::byte(0x08), std::byte(0x09), std::byte(0x0a), std::byte(0x0b), 
                std::byte(0x0c), std::byte(0x0d), std::byte(0x0e), std::byte(0x0f), 
            })
    { }

    ~FunctionalTestsFixture()
    {
        if (connectionManager)
        {
            connectionManager->StopListening();
            if (connectionManagerListenThread.joinable())
            {
                connectionManagerListenThread.join();
            }
        }
    }

    /**
     * @brief Instantiates a new ConnectionManager and sets up callbacks
     */
    void InitConnectionManager()
    {
        connectionManager = std::make_shared<TlsConnectionManager<FtlConnection>>(preSharedKey);
        connectionManager->SetOnNewConnection(
            std::bind(
                &FunctionalTestsFixture::onNewConnectionManagerConnection,
                this,
                std::placeholders::_1));
        connectionManager->Init();
    }

    /**
     * @brief Starts listening for connections on a separate thread
     */
    void StartConnectionManagerListening()
    {
        connectionManagerListenThread = std::thread(
            [this]()
            {
                this->connectionManager->Listen();
            });
        connectionManagerListenThread.detach();
    }

    /**
     * @brief Waits for a new connection to be reported by ConnectionManager, or times out
     * @param timeout how long to wait before timing out
     * @return std::optional<std::shared_ptr<FtlConnection>> the new connection
     */
    std::optional<std::shared_ptr<FtlConnection>> WaitForNewConnection(
        std::chrono::milliseconds timeout = std::chrono::milliseconds(1000))
    {
        std::unique_lock<std::mutex> lock(connectionManagerMutex);
        connectionManagerConditionVariable.wait_for(lock, timeout);
        if (newConnections.size() > 0)
        {
            auto returnValue = newConnections.front();
            newConnections.pop_front();
            return returnValue;
        }
        return std::nullopt;
    }

protected:
    std::vector<std::byte> preSharedKey;
    std::mutex connectionManagerMutex;
    std::condition_variable connectionManagerConditionVariable;
    std::shared_ptr<TlsConnectionManager<FtlConnection>> connectionManager;
    std::thread connectionManagerListenThread;
    std::list<std::shared_ptr<FtlConnection>> newConnections;

    void onNewConnectionManagerConnection(std::shared_ptr<FtlConnection> connection)
    {
        {
            std::lock_guard<std::mutex> lock(connectionManagerMutex);
            newConnections.push_back(connection);
        }
        connectionManagerConditionVariable.notify_all();
    }
};

TEST_CASE_METHOD(
    FunctionalTestsFixture,
    "Orchestration client can connect to Orchestration server",
    "[functional]")
{
    InitConnectionManager();
    StartConnectionManagerListening();

    // TODO: need to wait for server to be ready for new connections
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Now connect a client
    std::shared_ptr<FtlConnection> clientConnection = 
        FtlOrchestrationClient::Connect("127.0.0.1", preSharedKey);
    auto clientThread = std::thread(
        [&clientConnection]()
        {
            clientConnection->Start();
        });
    clientThread.detach();

    // Make sure the client successfully connected
    auto receivedConnection = WaitForNewConnection();
    REQUIRE(receivedConnection.has_value());
    if (receivedConnection)
    {
        receivedConnection.value()->Start();
    }

    if (clientThread.joinable())
    {
        clientThread.join();
    }

    // Shut down the client
    clientConnection->Stop();
}