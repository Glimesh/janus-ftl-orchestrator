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

    /* Static methods */
    /**
     * @brief Attempts to parse an Orchestration Protocol Message Header out of the given bytes
     * @param bytes bytes to parse
     * @return OrchestrationMessageHeader resulting header information
     */
    static OrchestrationMessageHeader ParseMessageHeader(const std::vector<uint8_t>& bytes);

    /**
     * @brief Serializes an Orchestration Protocol Message Header to a byte array
     * @param header header to serialize
     * @return std::vector<uint8_t> serialized bytes
     */
    static std::vector<uint8_t> SerializeMessageHeader(const OrchestrationMessageHeader& header);

    /**
     * @brief
     *  Takes a value and converts it to a payload suitable for sending over the network
     *  (swapping byte order if necessary)
     * 
     * @param value The value to convert
     * @return std::vector<uint8_t> The resulting payload
     */
    static std::vector<uint8_t> ConvertToNetworkPayload(const uint16_t value);

    /**
     * @brief
     *  Takes a value and converts it to a payload suitable for sending over the network
     *  (swapping byte order if necessary)
     * 
     * @param value The value to convert
     * @return std::vector<uint8_t> The resulting payload
     */
    static std::vector<uint8_t> ConvertToNetworkPayload(const uint32_t value);

    /**
     * @brief Takes a network encoded payload and deserializes it to a host uint32_t value
     * 
     * @param payload Payload to deserialize
     * @return uint32_t Resulting value
     */
    static uint32_t DeserializeNetworkUint32(const std::vector<uint8_t>& payload);

    /* IConnection */
    void Start() override;
    void Stop() override;
    void SendIntro(const ConnectionIntroPayload& payload) override;
    void SendOutro(const ConnectionOutroPayload& payload) override;
    void SendNodeState(const ConnectionNodeStatePayload& payload) override;
    void SendChannelSubscription(const ConnectionSubscriptionPayload& payload) override;
    void SendStreamPublish(const ConnectionPublishPayload& payload) override;
    void SendStreamRelay(const ConnectionRelayPayload& payload) override;
    void SetOnConnectionClosed(std::function<void(void)> onConnectionClosed) override;
    void SetOnIntro(connection_cb_intro_t onIntro) override;
    void SetOnOutro(connection_cb_outro_t onOutro) override;
    void SetOnNodeState(connection_cb_nodestate_t onNodeState) override;
    void SetOnChannelSubscription(connection_cb_subscription_t onChannelSubscription) override;
    void SetOnStreamPublish(connection_cb_publishing_t onStreamPublish) override;
    void SetOnStreamRelay(connection_cb_relay_t onStreamRelay) override;
    std::string GetHostname() override;

private:
    std::shared_ptr<IConnectionTransport> transport;
    std::atomic<bool> isStopping { 0 };
    std::thread connectionThread;
    std::function<void(void)> onConnectionClosed;
    connection_cb_intro_t onIntro;
    connection_cb_outro_t onOutro;
    connection_cb_nodestate_t onNodeState;
    connection_cb_subscription_t onChannelSubscription;
    connection_cb_publishing_t onStreamPublish;
    connection_cb_relay_t onStreamRelay;
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
     * @brief Processes an Orchestration Protocol Message with a payload.
     * @param header the parsed header
     * @param payload the raw payload data
     */
    void processMessage(
        const OrchestrationMessageHeader& header,
        const std::vector<uint8_t>& payload);

    /**
     * @brief Process Orchestration Protocol Message of type Intro
     */
    void processIntroMessage(
        const OrchestrationMessageHeader& header,
        const std::vector<uint8_t>& payload);

    /**
     * @brief Process Orchestration Protocol Message of type Outro
     */
    void processOutroMessage(
        const OrchestrationMessageHeader& header,
        const std::vector<uint8_t>& payload);

    /**
     * @brief Process Orchestration Protocol Message of type Node State
     */
    void processNodeStateMessage(
        const OrchestrationMessageHeader& header,
        const std::vector<uint8_t>& payload);

    /**
     * @brief Process Orchestration Protocol Message of type Channel Subscription
     */
    void processChannelSubscriptionMessage(
        const OrchestrationMessageHeader& header,
        const std::vector<uint8_t>& payload);

    /**
     * @brief Process Orchestration Protocol Message of type Stream Publishing
     */
    void processStreamPublishMessage(
        const OrchestrationMessageHeader& header,
        const std::vector<uint8_t>& payload);

    /**
     * @brief Process Orchestration Protocol Message of type Stream Relay
     */
    void processStreamRelayMessage(
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