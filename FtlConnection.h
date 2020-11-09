/**
 * @file FtlConnection.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-11-08
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#pragma once

#include "IConnection.h"

#include "FtlTypes.h"
#include "IConnectionTransport.h"
#include "OrchestrationProtocolTypes.h"

#include <atomic>
#include <functional>
#include <memory>
#include <thread>

/**
 * @brief
 *  FtlConnection translates FTL Orchestration Protocol binary data to/from an IConnectionTransport
 *  to discrete FTL commands and events.
 * 
 */
class FtlConnection : public IConnection
{
public:
    /* Constructor/Destructor */
    FtlConnection(std::shared_ptr<IConnectionTransport> transport);

    /* IConnection */
    void Start() override;
    void Stop() override;
    void SendStreamAvailable(std::shared_ptr<Stream> stream) override;
    void SendStreamEnded(std::shared_ptr<Stream> stream) override;
    void SetOnConnectionClosed(std::function<void(void)> onConnectionClosed) override;
    void SetOnIngestNewStream(
        std::function<void(ftl_channel_id_t, ftl_stream_id_t)> onIngestNewStream) override;
    void SetOnIngestStreamEnded(
        std::function<void(ftl_channel_id_t, ftl_stream_id_t)> onIngestStreamEnded) override;
    void SetOnStreamViewersUpdated(
        std::function<void(ftl_channel_id_t, ftl_stream_id_t, uint32_t)>
        onStreamViewersUpdated) override;
    std::string GetHostname() override;

private:
    std::shared_ptr<IConnectionTransport> transport;
    std::atomic<bool> isStopping { 0 };
    std::thread connectionThread;
    std::function<void(void)> onConnectionClosed;
    std::function<void(ftl_channel_id_t, ftl_stream_id_t)> onIngestNewStream;
    std::function<void(ftl_channel_id_t, ftl_stream_id_t)> onIngestStreamEnded;
    std::function<void(ftl_channel_id_t, ftl_stream_id_t, uint32_t)> onStreamViewersUpdated;
    std::string hostname;

    /* Private methods */
    /**
     * @brief
     *  Method body that runs on a different thread started by Start(), handles incoming connection
     *  data.
     */
    void startConnectionThread();

    /**
     * @brief Called when underlying transport has closed.
     */
    void onTransportConnectionClosed();

    /**
     * @brief Attempts to parse an Orchestration Protocol Message Header out of the given bytes
     * @param bytes bytes to parse
     * @return OrchestrationMessageHeader resulting header information
     */
    OrchestrationMessageHeader parseMessageHeader(const std::vector<uint8_t>& bytes);

    /**
     * @brief Processes an Orchestration Protocol Message with a payload.
     * @param header the parsed header
     * @param payload the raw payload data
     */
    void processMessage(
        const OrchestrationMessageHeader& header,
        const std::vector<uint8_t>& payload);

    /**
     * @brief Process Orchestration Protocol Nessage of type Intro
     */
    void processIntroMessage(
        const OrchestrationMessageHeader& header,
        const std::vector<uint8_t>& payload);

    /**
     * @brief Process Orchestration Protocol Nessage of type Outro
     */
    void processOutroMessage(
        const OrchestrationMessageHeader& header,
        const std::vector<uint8_t>& payload);

    /**
     * @brief Process Orchestration Protocol Nessage of type SubscribeChannel
     */
    void processSubscribeChannelMessage(
        const OrchestrationMessageHeader& header,
        const std::vector<uint8_t>& payload);

    /**
     * @brief Process Orchestration Protocol Nessage of type UnsubscribeChannel
     */
    void processUnsubscribeChannelMessage(
        const OrchestrationMessageHeader& header,
        const std::vector<uint8_t>& payload);

    /**
     * @brief Process Orchestration Protocol Nessage of type StreamAvailable
     */
    void processStreamAvailableMessage(
        const OrchestrationMessageHeader& header,
        const std::vector<uint8_t>& payload);

    /**
     * @brief Process Orchestration Protocol Nessage of type StreamRemoved
     */
    void processStreamRemovedMessage(
        const OrchestrationMessageHeader& header,
        const std::vector<uint8_t>& payload);

    /**
     * @brief Process Orchestration Protocol Nessage of type StreamMetadata
     */
    void processStreamMetadataMessage(
        const OrchestrationMessageHeader& header,
        const std::vector<uint8_t>& payload);

    /**
     * @brief Sends the given FTL Orchestration Protocol message across the transport
     * @param header Orchestration Protocol Message Header
     * @param payload Orchestration Protocol Message Payload
     */
    void sendMessage(
        const OrchestrationMessageHeader& header,
        const std::vector<uint8_t>& payload);
};