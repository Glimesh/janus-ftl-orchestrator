/**
 * @file FtlConnectionUnitTests.cpp
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-11-09
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#include <catch2/catch.hpp>

#include "../mocks/MockConnectionTransport.h"
#include "../../src/FtlConnection.h"

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
    std::string hostname = "test";

    // Store values received by the connection
    std::optional<uint8_t> recvMajor;
    std::optional<uint8_t> recvMinor;
    std::optional<uint8_t> recvRevision;
    std::optional<std::string> recvHost;
    ftlConnection->SetOnIntro(
        [&recvMajor, &recvMinor, &recvRevision, &recvHost]
        (uint8_t major, uint8_t minor, uint8_t revision, std::string host)
        {
            recvMajor = major;
            recvMinor = minor;
            recvRevision = revision;
            recvHost = host;

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
    };
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
    REQUIRE(recvMajor.value() == versionMajor);
    REQUIRE(recvMinor.value() == versionMinor);
    REQUIRE(recvRevision.value() == versionRevision);
    REQUIRE(recvHost.value() == hostname);

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
    std::optional<std::string> recvReason;
    ftlConnection->SetOnOutro(
        [&recvReason](std::string reason)
        {
            recvReason = reason;

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
    REQUIRE(recvReason.value() == sendReason);

    ftlConnection->Stop();
}

TEST_CASE("Subscribe channel requests are recognized", "[connection]")
{
    auto mockTransport = std::make_shared<MockConnectionTransport>();
    auto ftlConnection = std::make_shared<FtlConnection>(mockTransport);

    // Start ftl connection thread
    ftlConnection->Start();

    // Our payload value
    uint8_t messageId = 123;
    uint32_t sendChannelId = 123456789;

    // Keep track of what we receive
    std::optional<uint32_t> recvChannelId;
    ftlConnection->SetOnSubscribeChannel(
        [&recvChannelId](uint32_t channelId)
        {
            recvChannelId = channelId;

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
            .MessageType = OrchestrationMessageType::SubscribeChannel,
            .MessageId = messageId,
            .MessagePayloadLength = 4,
        });
    // Add the channel ID payload
    std::vector<uint8_t> channelIdBytes = FtlConnection::ConvertToNetworkPayload(sendChannelId);
    messageBuffer.insert(messageBuffer.end(), channelIdBytes.begin(), channelIdBytes.end());

    // Send to the FtlConnection
    mockTransport->MockSetReadBuffer(messageBuffer);

    // Verify response
    std::optional<std::vector<uint8_t>> response = mockTransport->WaitForWrite();
    REQUIRE(response.has_value());
    OrchestrationMessageHeader responseHeader = FtlConnection::ParseMessageHeader(response.value());
    REQUIRE(responseHeader.MessageDirection == OrchestrationMessageDirectionKind::Response);
    REQUIRE(responseHeader.MessageFailure == false);
    REQUIRE(responseHeader.MessageType == OrchestrationMessageType::SubscribeChannel);
    REQUIRE(responseHeader.MessageId == messageId);

    // Verify the FtlConnection received the expected value
    REQUIRE(recvChannelId.value() == sendChannelId);

    ftlConnection->Stop();
}

TEST_CASE("Unsubscribe channel requests are recognized", "[connection]")
{
    auto mockTransport = std::make_shared<MockConnectionTransport>();
    auto ftlConnection = std::make_shared<FtlConnection>(mockTransport);

    // Start ftl connection thread
    ftlConnection->Start();

    // Our payload value
    uint8_t messageId = 123;
    uint32_t sendChannelId = 123456789;

    // Keep track of what we receive
    std::optional<uint32_t> recvChannelId;
    ftlConnection->SetOnUnsubscribeChannel(
        [&recvChannelId](uint32_t channelId)
        {
            recvChannelId = channelId;

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
            .MessageType = OrchestrationMessageType::UnsubscribeChannel,
            .MessageId = messageId,
            .MessagePayloadLength = 4,
        });
    // Add the channel ID payload
    std::vector<uint8_t> channelIdBytes = FtlConnection::ConvertToNetworkPayload(sendChannelId);
    messageBuffer.insert(messageBuffer.end(), channelIdBytes.begin(), channelIdBytes.end());

    // Send to the FtlConnection
    mockTransport->MockSetReadBuffer(messageBuffer);

    // Verify response
    std::optional<std::vector<uint8_t>> response = mockTransport->WaitForWrite();
    REQUIRE(response.has_value());
    OrchestrationMessageHeader responseHeader = FtlConnection::ParseMessageHeader(response.value());
    REQUIRE(responseHeader.MessageDirection == OrchestrationMessageDirectionKind::Response);
    REQUIRE(responseHeader.MessageFailure == false);
    REQUIRE(responseHeader.MessageType == OrchestrationMessageType::UnsubscribeChannel);
    REQUIRE(responseHeader.MessageId == messageId);

    // Verify the FtlConnection received the expected value
    REQUIRE(recvChannelId.value() == sendChannelId);

    ftlConnection->Stop();
}

TEST_CASE("Stream available requests are recognized", "[connection]")
{
    auto mockTransport = std::make_shared<MockConnectionTransport>();
    auto ftlConnection = std::make_shared<FtlConnection>(mockTransport);

    // Start ftl connection thread
    ftlConnection->Start();

    // Our payload value
    uint8_t messageId = 123;
    uint32_t sendChannelId = 123456789;
    uint32_t sendStreamId = 987654321;
    std::string sendHost = "test-ingest";

    // Keep track of what we receive
    std::optional<uint32_t> recvChannelId;
    std::optional<uint32_t> recvStreamId;
    std::optional<std::string> recvHost;
    ftlConnection->SetOnStreamAvailable(
        [&recvChannelId, &recvStreamId, &recvHost]
        (uint32_t channelId, uint32_t streamId, std::string host)
        {
            recvChannelId = channelId;
            recvStreamId = streamId;
            recvHost = host;

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
            .MessageType = OrchestrationMessageType::StreamAvailable,
            .MessageId = messageId,
            .MessagePayloadLength = static_cast<uint16_t>(8 + sendHost.size()),
        });
    // Add the payload
    std::vector<uint8_t> channelIdBytes = FtlConnection::ConvertToNetworkPayload(sendChannelId);
    messageBuffer.insert(messageBuffer.end(), channelIdBytes.begin(), channelIdBytes.end());
    std::vector<uint8_t> streamIdBytes = FtlConnection::ConvertToNetworkPayload(sendStreamId);
    messageBuffer.insert(messageBuffer.end(), streamIdBytes.begin(), streamIdBytes.end());
    messageBuffer.insert(messageBuffer.end(), sendHost.begin(), sendHost.end());

    // Send to the FtlConnection
    mockTransport->MockSetReadBuffer(messageBuffer);

    // Verify response
    std::optional<std::vector<uint8_t>> response = mockTransport->WaitForWrite();
    REQUIRE(response.has_value());
    OrchestrationMessageHeader responseHeader = FtlConnection::ParseMessageHeader(response.value());
    REQUIRE(responseHeader.MessageDirection == OrchestrationMessageDirectionKind::Response);
    REQUIRE(responseHeader.MessageFailure == false);
    REQUIRE(responseHeader.MessageType == OrchestrationMessageType::StreamAvailable);
    REQUIRE(responseHeader.MessageId == messageId);

    // Verify the FtlConnection received the expected value
    REQUIRE(recvChannelId.value() == sendChannelId);
    REQUIRE(recvStreamId.value() == sendStreamId);
    REQUIRE(recvHost.value() == sendHost);

    ftlConnection->Stop();
}

TEST_CASE("Stream removed requests are recognized", "[connection]")
{
    auto mockTransport = std::make_shared<MockConnectionTransport>();
    auto ftlConnection = std::make_shared<FtlConnection>(mockTransport);

    // Start ftl connection thread
    ftlConnection->Start();

    // Our payload value
    uint8_t messageId = 123;
    uint32_t sendChannelId = 123456789;
    uint32_t sendStreamId = 987654321;

    // Keep track of what we receive
    std::optional<uint32_t> recvChannelId;
    std::optional<uint32_t> recvStreamId;
    ftlConnection->SetOnStreamRemoved(
        [&recvChannelId, &recvStreamId]
        (uint32_t channelId, uint32_t streamId)
        {
            recvChannelId = channelId;
            recvStreamId = streamId;

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
            .MessageType = OrchestrationMessageType::StreamRemoved,
            .MessageId = messageId,
            .MessagePayloadLength = 8,
        });
    // Add the payload
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
    REQUIRE(responseHeader.MessageType == OrchestrationMessageType::StreamRemoved);
    REQUIRE(responseHeader.MessageId == messageId);

    // Verify the FtlConnection received the expected value
    REQUIRE(recvChannelId.value() == sendChannelId);
    REQUIRE(recvStreamId.value() == sendStreamId);

    ftlConnection->Stop();
}

TEST_CASE("Stream metadata requests are recognized", "[connection]")
{
    auto mockTransport = std::make_shared<MockConnectionTransport>();
    auto ftlConnection = std::make_shared<FtlConnection>(mockTransport);

    // Start ftl connection thread
    ftlConnection->Start();

    // Our payload value
    uint8_t messageId = 123;
    uint32_t sendChannelId = 123456789;
    uint32_t sendStreamId = 987654321;
    uint32_t sendViewers = 111111111;

    // Keep track of what we receive
    std::optional<uint32_t> recvChannelId;
    std::optional<uint32_t> recvStreamId;
    std::optional<uint32_t> recvViewers;
    ftlConnection->SetOnStreamMetadata(
        [&recvChannelId, &recvStreamId, &recvViewers]
        (uint32_t channelId, uint32_t streamId, uint32_t viewers)
        {
            recvChannelId = channelId;
            recvStreamId = streamId;
            recvViewers = viewers;

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
            .MessageType = OrchestrationMessageType::StreamMetadata,
            .MessageId = messageId,
            .MessagePayloadLength = 12,
        });
    // Add the payload
    std::vector<uint8_t> channelIdBytes = FtlConnection::ConvertToNetworkPayload(sendChannelId);
    messageBuffer.insert(messageBuffer.end(), channelIdBytes.begin(), channelIdBytes.end());
    std::vector<uint8_t> streamIdBytes = FtlConnection::ConvertToNetworkPayload(sendStreamId);
    messageBuffer.insert(messageBuffer.end(), streamIdBytes.begin(), streamIdBytes.end());
    std::vector<uint8_t> viewerBytes = FtlConnection::ConvertToNetworkPayload(sendViewers);
    messageBuffer.insert(messageBuffer.end(), viewerBytes.begin(), viewerBytes.end());

    // Send to the FtlConnection
    mockTransport->MockSetReadBuffer(messageBuffer);

    // Verify response
    std::optional<std::vector<uint8_t>> response = mockTransport->WaitForWrite();
    REQUIRE(response.has_value());
    OrchestrationMessageHeader responseHeader = FtlConnection::ParseMessageHeader(response.value());
    REQUIRE(responseHeader.MessageDirection == OrchestrationMessageDirectionKind::Response);
    REQUIRE(responseHeader.MessageFailure == false);
    REQUIRE(responseHeader.MessageType == OrchestrationMessageType::StreamMetadata);
    REQUIRE(responseHeader.MessageId == messageId);

    // Verify the FtlConnection received the expected value
    REQUIRE(recvChannelId.value() == sendChannelId);
    REQUIRE(recvStreamId.value() == sendStreamId);
    REQUIRE(recvViewers.value() == sendViewers);

    ftlConnection->Stop();
}