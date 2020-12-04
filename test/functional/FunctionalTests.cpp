/**
 * @file FunctionalTests.cpp
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-11-25
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#include <catch2/catch.hpp>

#include <FtlOrchestrationClient.h>
#include <Orchestrator.h>
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
            }),
        orchestrator(std::make_unique<Orchestrator<FtlConnection>>(
            std::make_unique<TlsConnectionManager<FtlConnection>>(preSharedKey)))
    {
        orchestrator->Init();

        // Start running on a separate thread,
        // but wait until we're ready to accept new connections
        std::promise<void> readyPromise;
        std::future<void> readyFuture = readyPromise.get_future();
        orchestratorThread = std::thread(
            [this, &readyPromise]()
            {
                this->orchestrator->Run(std::move(readyPromise));
            });
        readyFuture.get();
    }

    ~FunctionalTestsFixture()
    {
        // Stop listening
        orchestrator->Stop();
        orchestratorThread.join();
    }

    /**
     * @brief Prepares a new FtlConnection client to connect to local orchestration service
     * @return std::shared_ptr<FtlConnection> new client connection
     */
    std::shared_ptr<FtlConnection> ConnectNewClient(
        std::string hostname = std::string(),
        bool sendIntro = false,
        std::string regionCode = "global")
    {
        std::shared_ptr<FtlConnection> connection = 
            FtlOrchestrationClient::Connect("127.0.0.1", preSharedKey, hostname);
        if (sendIntro)
        {
            connection->Start();
            connection->SendIntro(
                ConnectionIntroPayload
                {
                    .VersionMajor = 0,
                    .VersionMinor = 0,
                    .VersionRevision = 1,
                    .RelayLayer = 0,
                    .RegionCode = regionCode,
                    .Hostname = hostname,
                });
        }
        return connection;
    }

protected:
    const std::chrono::milliseconds WAIT_TIMEOUT = std::chrono::milliseconds(1000);
    const std::vector<std::byte> preSharedKey;
    const std::shared_ptr<Orchestrator<FtlConnection>> orchestrator;
    std::thread orchestratorThread;
};

TEST_CASE_METHOD(
    FunctionalTestsFixture,
    "Ingest to Edge relaying",
    "[functional][relay]")
{
    ftl_channel_id_t channelId = 1234;
    ftl_stream_id_t streamId = 5678;
    std::vector<std::byte> streamKey
    {
        std::byte(0x0f), std::byte(0x0e), std::byte(0x0d), std::byte(0x0c),
        std::byte(0x0b), std::byte(0x0a), std::byte(0x09), std::byte(0x08), 
        std::byte(0x07), std::byte(0x06), std::byte(0x05), std::byte(0x04), 
        std::byte(0x03), std::byte(0x02), std::byte(0x01), std::byte(0x00), 
    };

    // Connect an ingest node
    auto ingestClient = ConnectNewClient("ingest", true);

    // Connect an edge node
    auto edgeClient = ConnectNewClient("edge", true);
    
    // Track when the ingest receives a relay message
    std::optional<ConnectionRelayPayload> recvRelayPayload;
    std::mutex recvRelayMutex;
    std::condition_variable recvRelayCv;
    ingestClient->SetOnStreamRelay(
        [&recvRelayPayload, &recvRelayCv](ConnectionRelayPayload relayPayload)
        {
            recvRelayPayload = relayPayload;
            recvRelayCv.notify_one();
            return ConnectionResult
            {
                .IsSuccess = true
            };
        });

    // Subscribe the edge node to the channel
    edgeClient->SendChannelSubscription(
        ConnectionSubscriptionPayload
        {
            .IsSubscribe = true,
            .ChannelId = channelId,
            .StreamKey = streamKey,
        });

    // Publish the stream from the ingest
    ingestClient->SendStreamPublish(
        ConnectionPublishPayload
        {
            .IsPublish = true,
            .ChannelId = channelId,
            .StreamId = streamId,
        });

    // Check to see if we've received the relay message
    std::unique_lock<std::mutex> lock(recvRelayMutex);
    recvRelayCv.wait_for(lock, WAIT_TIMEOUT);
    REQUIRE(recvRelayPayload.has_value());
    REQUIRE(recvRelayPayload.value().IsStartRelay == true);
    REQUIRE(recvRelayPayload.value().ChannelId == channelId);
    REQUIRE(recvRelayPayload.value().StreamId == streamId);
    REQUIRE(recvRelayPayload.value().TargetHostname == edgeClient->GetHostname());
    bool streamKeyMatch = (recvRelayPayload.value().StreamKey == streamKey);
    REQUIRE(streamKeyMatch == true);
    lock.unlock();

    // Clear the value and un-subscribe the stream
    recvRelayPayload.reset();
    edgeClient->SendChannelSubscription(
        ConnectionSubscriptionPayload
        {
            .IsSubscribe = false,
            .ChannelId = channelId,
            .StreamKey = std::vector<std::byte>(),
        });
    
    // Check to make sure we've received the stop relay message
    lock.lock();
    recvRelayCv.wait_for(lock, WAIT_TIMEOUT);
    REQUIRE(recvRelayPayload.has_value());
    REQUIRE(recvRelayPayload.value().IsStartRelay == false);
    REQUIRE(recvRelayPayload.value().ChannelId == channelId);
    REQUIRE(recvRelayPayload.value().StreamId == streamId);
    REQUIRE(recvRelayPayload.value().TargetHostname == edgeClient->GetHostname());
    lock.unlock();

    // Stop connections
    ingestClient->Stop();
    edgeClient->Stop();
}

TEST_CASE_METHOD(
    FunctionalTestsFixture,
    "Relays stopped when target disconnects",
    "[functional][relay]")
{
    ftl_channel_id_t channelId = 1234;
    ftl_stream_id_t streamId = 5678;
    std::vector<std::byte> streamKey
    {
        std::byte(0x0f), std::byte(0x0e), std::byte(0x0d), std::byte(0x0c),
        std::byte(0x0b), std::byte(0x0a), std::byte(0x09), std::byte(0x08), 
        std::byte(0x07), std::byte(0x06), std::byte(0x05), std::byte(0x04), 
        std::byte(0x03), std::byte(0x02), std::byte(0x01), std::byte(0x00), 
    };

    // Connect an ingest node
    auto ingestClient = ConnectNewClient("ingest", true);

    // Connect an edge node
    auto edgeClient = ConnectNewClient("edge", true);
    
    // Track when the ingest receives a relay message
    std::optional<ConnectionRelayPayload> recvRelayPayload;
    std::mutex recvRelayMutex;
    std::condition_variable recvRelayCv;
    ingestClient->SetOnStreamRelay(
        [&recvRelayPayload, &recvRelayCv](ConnectionRelayPayload relayPayload)
        {
            recvRelayPayload = relayPayload;
            recvRelayCv.notify_one();
            return ConnectionResult
            {
                .IsSuccess = true
            };
        });

    // Subscribe the edge node to the channel
    edgeClient->SendChannelSubscription(
        ConnectionSubscriptionPayload
        {
            .IsSubscribe = true,
            .ChannelId = channelId,
            .StreamKey = streamKey,
        });

    // Publish the stream from the ingest
    ingestClient->SendStreamPublish(
        ConnectionPublishPayload
        {
            .IsPublish = true,
            .ChannelId = channelId,
            .StreamId = streamId,
        });

    // Check to see if we've received the relay message
    std::unique_lock<std::mutex> lock(recvRelayMutex);
    recvRelayCv.wait_for(lock, WAIT_TIMEOUT);
    REQUIRE(recvRelayPayload.has_value());
    REQUIRE(recvRelayPayload.value().IsStartRelay == true);
    REQUIRE(recvRelayPayload.value().ChannelId == channelId);
    REQUIRE(recvRelayPayload.value().StreamId == streamId);
    REQUIRE(recvRelayPayload.value().TargetHostname == edgeClient->GetHostname());
    bool streamKeyMatch = (recvRelayPayload.value().StreamKey == streamKey);
    REQUIRE(streamKeyMatch == true);
    lock.unlock();

    // Clear the value and stop the edge node
    recvRelayPayload.reset();
    edgeClient->Stop();
    
    // Check to make sure we've received the stop relay message
    lock.lock();
    recvRelayCv.wait_for(lock, WAIT_TIMEOUT);
    REQUIRE(recvRelayPayload.has_value());
    REQUIRE(recvRelayPayload.value().IsStartRelay == false);
    REQUIRE(recvRelayPayload.value().ChannelId == channelId);
    REQUIRE(recvRelayPayload.value().StreamId == streamId);
    REQUIRE(recvRelayPayload.value().TargetHostname == edgeClient->GetHostname());
    lock.unlock();

    // Stop connections
    ingestClient->Stop();
}