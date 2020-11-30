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
        // Stop all incoming connections
        for (const auto& connection : connections)
        {
            connection->Stop();
        }
        if (connectionManager)
        {
            connectionManager->StopListening();
            connectionManagerListenThread.join();
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
        std::promise<void> listeningPromise;
        std::future<void> listeningFuture = listeningPromise.get_future();
        connectionManagerListenThread = std::thread(
            [this, &listeningPromise]()
            {
                this->connectionManager->Listen(std::move(listeningPromise));
            });
        listeningFuture.get();
    }

    /**
     * @brief Waits for a new connection to be reported by ConnectionManager, or times out
     * @param timeout how long to wait before timing out
     * @return std::optional<std::shared_ptr<FtlConnection>> the new connection
     */
    std::optional<std::shared_ptr<FtlConnection>> WaitForNewConnection(
        std::chrono::milliseconds timeout = std::chrono::milliseconds(5000))
    {
        std::unique_lock<std::mutex> lock(connectionManagerMutex);
        if (newConnections.size() <= 0)
        {
            connectionManagerConditionVariable.wait_for(lock, timeout);
        }
        if (newConnections.size() > 0)
        {
            auto returnValue = newConnections.front();
            newConnections.pop_front();
            return returnValue;
        }
        return std::nullopt;
    }

    /**
     * @brief Prepares a new FtlConnection client to connect to local orchestration service
     * @return std::shared_ptr<FtlConnection> new client connection
     */
    std::shared_ptr<FtlConnection> ConnectNewClient(std::string hostname = std::string())
    {
        return FtlOrchestrationClient::Connect("127.0.0.1", preSharedKey, hostname);
    }

    /**
     * @brief Starts a given client connection on a new thread
     * @param client client connection to start
     * @return std::future<void> future returning a value when the connection has started
     */
    std::future<void> StartClientAsync(std::shared_ptr<FtlConnection> client)
    {
        return std::async(std::launch::async, &FtlConnection::Start, client.get());
    }

    /**
     * @brief
     *  Creates a new FtlConnection client, connects it to the local orchestration service,
     *  sends an Intro message with the given parameters, and returns both the local and received
     *  connections
     * @param clientHostname hostname for the new client
     * @param clientRegionCode region code for the new client
     * @return
     *  std::pair<std::shared_ptr<FtlConnection>, std::shared_ptr<FtlConnection>> 
     *  first reference is the client connection, second reference is the received connection.
     */
    std::pair<std::shared_ptr<FtlConnection>, std::shared_ptr<FtlConnection>>
        ConnectAndStartNewClient(
            std::string clientHostname,
            std::string clientRegionCode = "global")
    {
        // Create the client connection, connect it, and get reference to the received connection
        std::shared_ptr<FtlConnection> clientConnection = ConnectNewClient(clientHostname);
        std::future<void> clientConnected = StartClientAsync(clientConnection);
        auto tryRecvConnection = WaitForNewConnection();
        REQUIRE(tryRecvConnection.has_value());
        auto recvConnection = tryRecvConnection.value();

        // When we receive an intro payload, store it away and notify our thread
        std::optional<ConnectionIntroPayload> recvIntroPayload;
        std::mutex introRecvMutex;
        std::condition_variable introRecvCv;
        recvConnection->SetOnIntro(
            [&introRecvCv, &recvIntroPayload](ConnectionIntroPayload introPayload)
            {
                recvIntroPayload = introPayload;
                introRecvCv.notify_all();
                return ConnectionResult
                {
                    .IsSuccess = true
                };
            });

        recvConnection->Start();
        clientConnected.get();

        // Send the intro and wait to receive it
        clientConnection->SendIntro(
            ConnectionIntroPayload
            {
                .VersionMajor = 0,
                .VersionMinor = 0,
                .VersionRevision = 1,
                .RelayLayer = 0,
                .RegionCode = clientRegionCode,
                .Hostname = clientHostname,
            });
        std::unique_lock<std::mutex> lock(introRecvMutex);
        introRecvCv.wait_for(lock, std::chrono::milliseconds(5000));

        REQUIRE(recvIntroPayload.has_value());

        return { clientConnection, recvConnection };
    }

protected:
    std::vector<std::byte> preSharedKey;
    std::mutex connectionManagerMutex;
    std::condition_variable connectionManagerConditionVariable;
    std::shared_ptr<TlsConnectionManager<FtlConnection>> connectionManager;
    std::thread connectionManagerListenThread;
    std::list<std::shared_ptr<FtlConnection>> newConnections;
    std::list<std::shared_ptr<FtlConnection>> connections;
    std::list<std::shared_ptr<FtlConnection>> clients;

    void onNewConnectionManagerConnection(std::shared_ptr<FtlConnection> connection)
    {
        {
            std::lock_guard<std::mutex> lock(connectionManagerMutex);
            connections.push_back(connection);
            newConnections.push_back(connection);
        }
        connectionManagerConditionVariable.notify_all();
    }
};

TEST_CASE_METHOD(
    FunctionalTestsFixture,
    "Orchestration client connect and disconnect",
    "[functional]")
{
    InitConnectionManager();
    StartConnectionManagerListening();
    
    // Now connect a client
    std::shared_ptr<FtlConnection> clientConnection = ConnectNewClient();

    // We need to start the client in a separate thread so we can accept the connection
    // in this thread without deadlocking.
    std::future<void> clientStarted = StartClientAsync(clientConnection);

    // Make sure the server successfully received the connection
    auto tryReceivedConnection = WaitForNewConnection();
    REQUIRE(tryReceivedConnection.has_value());
    auto receivedConnection = tryReceivedConnection.value();

    // Record when we see the received connection disconnect
    std::promise<void> receivedConnClosedPromise;
    std::future<void> receivedConnClosedFuture = receivedConnClosedPromise.get_future();
    receivedConnection->SetOnConnectionClosed(
        [&receivedConnClosedPromise]()
        {
            receivedConnClosedPromise.set_value_at_thread_exit();
        });

    // Start the received connection
    receivedConnection->Start();

    // Make sure our client finished starting
    clientStarted.get();

    // Shut down the client
    clientConnection->Stop();

    // Wait for the received connection to close
    receivedConnClosedFuture.get();
}

TEST_CASE_METHOD(
    FunctionalTestsFixture,
    "Intro messages sent by the client are received",
    "[functional]")
{
    InitConnectionManager();
    StartConnectionManagerListening();

    std::shared_ptr<FtlConnection> clientConnection = ConnectNewClient();
    std::future<void> clientConnected = StartClientAsync(clientConnection);
    
    auto tryReceivedConnection = WaitForNewConnection();
    REQUIRE(tryReceivedConnection.has_value());
    auto receivedConnection = tryReceivedConnection.value();

    // When we receive an intro payload, store it away and notify our thread
    std::optional<ConnectionIntroPayload> recvIntroPayload;
    std::mutex introRecvMutex;
    std::condition_variable introRecvCv;
    receivedConnection->SetOnIntro(
        [&introRecvCv, &recvIntroPayload](ConnectionIntroPayload introPayload) -> ConnectionResult
        {
            recvIntroPayload = introPayload;
            introRecvCv.notify_all();
            return ConnectionResult
            {
                .IsSuccess = true
            };
        });

    // Start received connection and ensure client has started
    receivedConnection->Start();
    clientConnected.get();

    // Send an intro from the client
    ConnectionIntroPayload introPayload
        {
            // TODO: Store version info in some common header...
            .VersionMajor = 0,
            .VersionMinor = 0,
            .VersionRevision = 1,
            .RelayLayer = 0,
            .RegionCode = "test",
            .Hostname = "test-host",
        };
    clientConnection->SendIntro(introPayload);

    // Wait to see if we got the intro...
    std::unique_lock<std::mutex> lock(introRecvMutex);
    introRecvCv.wait_for(lock, std::chrono::milliseconds(5000));

    REQUIRE(recvIntroPayload.has_value());
    bool payloadIsEqual = (recvIntroPayload.value() == introPayload);
    REQUIRE(payloadIsEqual == true);
}

// TEST_CASE_METHOD(
//     FunctionalTestsFixture,
//     "Ingest to Edge relaying",
//     "[functional][relay]")
// {
//     // TODO: ORCHESTRATOR INIT!
//     InitConnectionManager();
//     StartConnectionManagerListening();

//     ftl_channel_id_t channelId = 1234;
//     ftl_stream_id_t streamId = 5678;
//     std::vector<std::byte> streamKey
//     {
//         std::byte(0x0f), std::byte(0x0e), std::byte(0x0d), std::byte(0x0c),
//         std::byte(0x0b), std::byte(0x0a), std::byte(0x09), std::byte(0x08), 
//         std::byte(0x07), std::byte(0x06), std::byte(0x05), std::byte(0x04), 
//         std::byte(0x03), std::byte(0x02), std::byte(0x01), std::byte(0x00), 
//     };

//     // Connect an ingest node
//     auto ingestConnections = ConnectAndStartNewClient("ingest");
//     auto ingestClient = ingestConnections.first;
//     auto ingestRecv = ingestConnections.second;

//     // Connect an edge node
//     auto edgeConnections = ConnectAndStartNewClient("edge");
//     auto edgeClient = ingestConnections.first;
//     auto edgeRecv = ingestConnections.second;
    
//     // Track when the ingest receives a relay message
//     std::optional<ConnectionRelayPayload> recvRelayPayload;
//     std::mutex recvRelayMutex;
//     std::condition_variable recvRelayCv;
//     ingestClient->SetOnStreamRelay(
//         [&recvRelayPayload, &recvRelayCv](ConnectionRelayPayload relayPayload)
//         {
//             recvRelayPayload = relayPayload;
//             recvRelayCv.notify_all();
//             return ConnectionResult
//             {
//                 .IsSuccess = true
//             };
//         });

//     // Subscribe the edge node to the channel
//     edgeClient->SendChannelSubscription(
//         ConnectionSubscriptionPayload
//         {
//             .IsSubscribe = true,
//             .ChannelId = channelId,
//             .StreamKey = streamKey,
//         });

//     // Publish the stream from the ingest
//     ingestClient->SendStreamPublish(
//         ConnectionPublishPayload
//         {
//             .IsPublish = true,
//             .ChannelId = channelId,
//             .StreamId = streamId,
//         });

//     // Check to see if we've received the relay message
//     std::unique_lock<std::mutex> lock(recvRelayMutex);
//     recvRelayCv.wait_for(lock, std::chrono::milliseconds(5000));
//     REQUIRE(recvRelayPayload.has_value());
//     REQUIRE(recvRelayPayload.value().ChannelId == channelId);
//     REQUIRE(recvRelayPayload.value().StreamId == streamId);
//     bool streamKeyMatch = (recvRelayPayload.value().StreamKey == streamKey);
//     REQUIRE(streamKeyMatch == true);
// }