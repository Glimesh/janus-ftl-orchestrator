/**
 * @file FtlConnectionUnitTests.cpp
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-11-09
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#include <catch2/catch.hpp>

#include <FtlConnection.h>

#include "../mocks/MockConnectionTransport.h"

TEST_CASE("Intro requests are recognized", "[connection]")
{
    auto mockTransport = std::make_shared<MockConnectionTransport>();
    auto ftlConnection = std::make_shared<FtlConnection>(mockTransport);

    // Start ftl connection thread
    ftlConnection->Start();

    // Our payload values
    uint8_t versionMajor = 0;
    uint8_t versionMinor = 0;
    uint8_t versionRevision = 0;
    uint8_t relayLayer = 0;
    std::string regionCode = "sea";
    std::string hostname = "test";

    // Store values received by the connection
    std::optional<ConnectionIntroPayload> receivedPayload;
    ftlConnection->SetOnIntro(
        [&receivedPayload]
        (ConnectionIntroPayload payload)
        {
            receivedPayload = payload;

            return ConnectionResult
            {
                .IsSuccess = true
            };
        });

    // Write an intro connection message
    std::vector<uint8_t> payloadBuffer = {
        // Version Major
        versionMajor,
        // Version Minor
        versionMinor,
        // Revision
        versionRevision,
        // Relay layer
        relayLayer,
    };
    auto regionCodeLength = 
        FtlConnection::ConvertToNetworkPayload(static_cast<uint16_t>(regionCode.size()));
    payloadBuffer.insert(payloadBuffer.end(), regionCodeLength.begin(), regionCodeLength.end());
    // Add the region code to the payload
    payloadBuffer.insert(payloadBuffer.end(), regionCode.begin(), regionCode.end());
    // Add the hostname to the payload
    payloadBuffer.insert(payloadBuffer.end(), hostname.begin(), hostname.end());

    // Construct the message
    std::vector<uint8_t> messageBuffer = FtlConnection::SerializeMessageHeader(
        {
            .MessageDirection = OrchestrationMessageDirectionKind::Request,
            .MessageFailure = false,
            .MessageType = OrchestrationMessageType::Intro,
            .MessageId = 1,
            .MessagePayloadLength = static_cast<uint16_t>(payloadBuffer.size()),
        });
    messageBuffer.insert(messageBuffer.end(), payloadBuffer.begin(), payloadBuffer.end());
    mockTransport->MockSetReadBuffer(messageBuffer);

    // Verify response
    std::optional<std::vector<uint8_t>> response = mockTransport->WaitForWrite();
    REQUIRE(response.has_value());
    OrchestrationMessageHeader responseHeader = FtlConnection::ParseMessageHeader(response.value());
    REQUIRE(responseHeader.MessageDirection == OrchestrationMessageDirectionKind::Response);
    REQUIRE(responseHeader.MessageFailure == false);
    REQUIRE(responseHeader.MessageType == OrchestrationMessageType::Intro);
    REQUIRE(responseHeader.MessageId == 1);

    // Verify our values were received by the connection
    REQUIRE(receivedPayload.has_value());
    REQUIRE(receivedPayload.value().VersionMajor == versionMajor);
    REQUIRE(receivedPayload.value().VersionMinor == versionMinor);
    REQUIRE(receivedPayload.value().VersionRevision == versionRevision);
    REQUIRE(receivedPayload.value().RegionCode == regionCode);
    REQUIRE(receivedPayload.value().Hostname == hostname);

    ftlConnection->Stop();
}

TEST_CASE("Outro requests are recognized", "[connection]")
{
    auto mockTransport = std::make_shared<MockConnectionTransport>();
    auto ftlConnection = std::make_shared<FtlConnection>(mockTransport);

    // Start ftl connection thread
    ftlConnection->Start();

    // Our payload value
    uint8_t messageId = 123;
    std::string sendReason = "testing";

    // Keep track of what we receive
    std::optional<ConnectionOutroPayload> recvPayload;
    ftlConnection->SetOnOutro(
        [&recvPayload](ConnectionOutroPayload payload)
        {
            recvPayload = payload;

            return ConnectionResult
            {
                .IsSuccess = true
            };
        });

    // Construct the message
    std::vector<uint8_t> messageBuffer = FtlConnection::SerializeMessageHeader(
        {
            .MessageDirection = OrchestrationMessageDirectionKind::Request,
            .MessageFailure = false,
            .MessageType = OrchestrationMessageType::Outro,
            .MessageId = messageId,
            .MessagePayloadLength = static_cast<uint16_t>(sendReason.size()),
        });
    // Add the reason to the message payload
    messageBuffer.insert(messageBuffer.end(), sendReason.begin(), sendReason.end());

    // Send to the FtlConnection
    mockTransport->MockSetReadBuffer(messageBuffer);

    // Verify response
    std::optional<std::vector<uint8_t>> response = mockTransport->WaitForWrite();
    REQUIRE(response.has_value());
    OrchestrationMessageHeader responseHeader = FtlConnection::ParseMessageHeader(response.value());
    REQUIRE(responseHeader.MessageDirection == OrchestrationMessageDirectionKind::Response);
    REQUIRE(responseHeader.MessageFailure == false);
    REQUIRE(responseHeader.MessageType == OrchestrationMessageType::Outro);
    REQUIRE(responseHeader.MessageId == messageId);

    // Verify the FtlConnection received the expected value
    REQUIRE(recvPayload.has_value());
    REQUIRE(recvPayload.value().DisconnectReason == sendReason);

    ftlConnection->Stop();
}

// TODO test node state messages

TEST_CASE("Channel subscription requests are recognized", "[connection]")
{
    auto mockTransport = std::make_shared<MockConnectionTransport>();
    auto ftlConnection = std::make_shared<FtlConnection>(mockTransport);

    // Start ftl connection thread
    ftlConnection->Start();

    // Our payload value
    uint8_t sendMessageId = 123;
    bool sendIsSubscribe = true;
    uint32_t sendChannelId = 123456789;
    std::vector<std::byte> sendStreamKey
    {
        std::byte(0x00),
        std::byte(0x01),
        std::byte(0x02),
        std::byte(0x03),
        std::byte(0x04),
        std::byte(0x05),
        std::byte(0x06),
        std::byte(0x07),
    };

    // Keep track of what we receive
    std::optional<ConnectionSubscriptionPayload> recvPayload;
    ftlConnection->SetOnChannelSubscription(
        [&recvPayload](ConnectionSubscriptionPayload payload)
        {
            recvPayload = payload;

            return ConnectionResult
            {
                .IsSuccess = true
            };
        });

    // Construct the message
    std::vector<uint8_t> messageBuffer = FtlConnection::SerializeMessageHeader(
        {
            .MessageDirection = OrchestrationMessageDirectionKind::Request,
            .MessageFailure = false,
            .MessageType = OrchestrationMessageType::ChannelSubscription,
            .MessageId = sendMessageId,
            .MessagePayloadLength = static_cast<uint16_t>(5 + sendStreamKey.size()),
        });
    // Add the payload
    messageBuffer.push_back(static_cast<uint8_t>(sendIsSubscribe));
    std::vector<uint8_t> channelIdBytes = FtlConnection::ConvertToNetworkPayload(sendChannelId);
    messageBuffer.insert(messageBuffer.end(), channelIdBytes.begin(), channelIdBytes.end());
    // TODO: use std::byte everywhere to avoid these annoying casts...
    messageBuffer.reserve(messageBuffer.size() + sendStreamKey.size());
    for (const auto& keyByte : sendStreamKey)
    {
        messageBuffer.push_back(static_cast<uint8_t>(keyByte));
    }

    // Send to the FtlConnection
    mockTransport->MockSetReadBuffer(messageBuffer);

    // Verify response
    std::optional<std::vector<uint8_t>> response = mockTransport->WaitForWrite();
    REQUIRE(response.has_value());
    OrchestrationMessageHeader responseHeader = FtlConnection::ParseMessageHeader(response.value());
    REQUIRE(responseHeader.MessageDirection == OrchestrationMessageDirectionKind::Response);
    REQUIRE(responseHeader.MessageFailure == false);
    REQUIRE(responseHeader.MessageType == OrchestrationMessageType::ChannelSubscription);
    REQUIRE(responseHeader.MessageId == sendMessageId);

    // Verify the FtlConnection received the expected value
    REQUIRE(recvPayload.has_value());
    REQUIRE(recvPayload.value().IsSubscribe == sendIsSubscribe);
    REQUIRE(recvPayload.value().ChannelId == sendChannelId);
    REQUIRE(recvPayload.value().StreamKey == sendStreamKey);

    ftlConnection->Stop();
}

TEST_CASE("Stream publish requests are recognized", "[connection]")
{
    auto mockTransport = std::make_shared<MockConnectionTransport>();
    auto ftlConnection = std::make_shared<FtlConnection>(mockTransport);

    // Start ftl connection thread
    ftlConnection->Start();

    // Our payload value
    uint8_t messageId = 123;
    bool sendIsPublish = true;
    uint32_t sendChannelId = 123456789;
    uint32_t sendStreamId = 987654321;

    // Keep track of what we receive
    std::optional<ConnectionPublishPayload> recvPayload;
    ftlConnection->SetOnStreamPublish(
        [&recvPayload](ConnectionPublishPayload payload)
        {
            recvPayload = payload;

            return ConnectionResult
            {
                .IsSuccess = true
            };
        });

    // Construct the message
    std::vector<uint8_t> messageBuffer = FtlConnection::SerializeMessageHeader(
        {
            .MessageDirection = OrchestrationMessageDirectionKind::Request,
            .MessageFailure = false,
            .MessageType = OrchestrationMessageType::StreamPublish,
            .MessageId = messageId,
            .MessagePayloadLength = 9,
        });
    // Add the payload
    messageBuffer.push_back(static_cast<uint8_t>(sendIsPublish));
    std::vector<uint8_t> channelIdBytes = FtlConnection::ConvertToNetworkPayload(sendChannelId);
    messageBuffer.insert(messageBuffer.end(), channelIdBytes.begin(), channelIdBytes.end());
    std::vector<uint8_t> streamIdBytes = FtlConnection::ConvertToNetworkPayload(sendStreamId);
    messageBuffer.insert(messageBuffer.end(), streamIdBytes.begin(), streamIdBytes.end());

    // Send to the FtlConnection
    mockTransport->MockSetReadBuffer(messageBuffer);

    // Verify response
    std::optional<std::vector<uint8_t>> response = mockTransport->WaitForWrite();
    REQUIRE(response.has_value());
    OrchestrationMessageHeader responseHeader = FtlConnection::ParseMessageHeader(response.value());
    REQUIRE(responseHeader.MessageDirection == OrchestrationMessageDirectionKind::Response);
    REQUIRE(responseHeader.MessageFailure == false);
    REQUIRE(responseHeader.MessageType == OrchestrationMessageType::StreamPublish);
    REQUIRE(responseHeader.MessageId == messageId);

    // Verify the FtlConnection received the expected value
    REQUIRE(recvPayload.has_value());
    REQUIRE((recvPayload.value().IsPublish == 1) == sendIsPublish);
    REQUIRE(recvPayload.value().ChannelId == sendChannelId);
    REQUIRE(recvPayload.value().StreamId == sendStreamId);

    ftlConnection->Stop();
}

// TODO stream relay messages