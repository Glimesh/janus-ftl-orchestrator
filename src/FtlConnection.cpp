/**
 * @file FtlConnection.cpp
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-11-08
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#include "FtlConnection.h"

#include <bit>
#include <optional>

#pragma region Constructor/Destructor
FtlConnection::FtlConnection(std::shared_ptr<IConnectionTransport> transport) : 
    transport(transport)
{ }
#pragma endregion

#pragma region Static methods
OrchestrationMessageHeader FtlConnection::ParseMessageHeader(const std::vector<uint8_t>& bytes)
{
    if (bytes.size() < 4)
    {
        throw std::range_error("Attempt to parse message header that is under 4 bytes.");
    }

    uint8_t messageDesc = bytes.at(0);
    OrchestrationMessageDirectionKind messageDirection = (messageDesc & 0b10000000) == 0 ?
        OrchestrationMessageDirectionKind::Request : OrchestrationMessageDirectionKind::Response;
    bool messageIsFailure = ((messageDesc & 0b01000000) != 0);
    OrchestrationMessageType messageType = 
        static_cast<OrchestrationMessageType>(messageDesc & 0b00111111);
    uint8_t messageId = bytes.at(1);
    // Determine if we need to flip things around if the host byte ordering is not the same
    // as network byte ordering (big endian)
    uint16_t payloadLength;
    if (std::endian::native != std::endian::big)
    {
        payloadLength = (static_cast<uint16_t>(bytes.at(3)) << 8) | bytes.at(2);
    }
    else
    {
        payloadLength = (static_cast<uint16_t>(bytes.at(2)) << 8) | bytes.at(3);
    }

    return OrchestrationMessageHeader
    {
        .MessageDirection = messageDirection,
        .MessageFailure = messageIsFailure,
        .MessageType = messageType,
        .MessageId = messageId,
        .MessagePayloadLength = payloadLength,
    };
}

std::vector<uint8_t> FtlConnection::SerializeMessageHeader(
    const OrchestrationMessageHeader& header)
{
    std::vector<uint8_t> headerBytes;
    headerBytes.reserve(4);

    // Convert header to byte payload
    uint8_t messageDesc = static_cast<uint8_t>(header.MessageType);
    if (header.MessageDirection == OrchestrationMessageDirectionKind::Response)
    {
        messageDesc = (messageDesc | 0b10000000);
    }
    if (header.MessageFailure)
    {
        messageDesc = (messageDesc | 0b01000000);
    }
    headerBytes.emplace_back(messageDesc);
    headerBytes.emplace_back(header.MessageId);

    // Encode payload length
    std::vector<uint8_t> payloadLengthBytes = ConvertToNetworkPayload(header.MessagePayloadLength);
    headerBytes.insert(headerBytes.end(), payloadLengthBytes.begin(), payloadLengthBytes.end());

    return headerBytes;
}

std::vector<uint8_t> FtlConnection::ConvertToNetworkPayload(const uint16_t value)
{
    std::vector<uint8_t> payload;
    payload.reserve(2); // 16 bits

    if (std::endian::native != std::endian::big)
    {
        payload.emplace_back(static_cast<uint8_t>(value & 0x00FF));
        payload.emplace_back(static_cast<uint8_t>((value >> 8) & 0x00FF));
    }
    else
    {
        payload.emplace_back(static_cast<uint8_t>((value >> 8) & 0x00FF));
        payload.emplace_back(static_cast<uint8_t>(value & 0x00FF));
    }

    return payload;
}

std::vector<uint8_t> FtlConnection::ConvertToNetworkPayload(const uint32_t value)
{
    std::vector<uint8_t> payload;
    payload.reserve(4); // 32 bits

    if (std::endian::native != std::endian::big)
    {
        payload.emplace_back(static_cast<uint8_t>(value & 0x000000FF));
        payload.emplace_back(static_cast<uint8_t>((value >> 8) & 0x000000FF));
        payload.emplace_back(static_cast<uint8_t>((value >> 16) & 0x000000FF));
        payload.emplace_back(static_cast<uint8_t>((value >> 24) & 0x000000FF));
    }
    else
    {
        payload.emplace_back(static_cast<uint8_t>((value >> 24) & 0x000000FF));
        payload.emplace_back(static_cast<uint8_t>((value >> 16) & 0x000000FF));
        payload.emplace_back(static_cast<uint8_t>((value >> 8) & 0x000000FF));
        payload.emplace_back(static_cast<uint8_t>(value & 0x000000FF));
    }

    return payload;
}

uint32_t FtlConnection::DeserializeNetworkUint32(const std::vector<uint8_t>& payload)
{
    if (payload.size() != 4)
    {
        throw std::range_error("Deserializing uint32 requires a 4 byte payload.");
    }

    if (std::endian::native != std::endian::big)
    {
        return (static_cast<uint32_t>(payload.at(3)) << 24) | 
            (static_cast<uint32_t>(payload.at(2)) << 16) | 
            (static_cast<uint32_t>(payload.at(1)) << 8) | 
            static_cast<uint32_t>(payload.at(0));
    }
    else
    {
        return (static_cast<uint32_t>(payload.at(0)) << 24) | 
            (static_cast<uint32_t>(payload.at(1)) << 16) | 
            (static_cast<uint32_t>(payload.at(2)) << 8) | 
            static_cast<uint32_t>(payload.at(3));
    }
}
#pragma endregion

#pragma region IConnection
void FtlConnection::Start()
{
    // Bind to transport events
    transport->SetOnConnectionClosed(std::bind(&FtlConnection::onTransportConnectionClosed, this));

    // Start the transport
    transport->Start();

    // Spin up the thread that will listen to data coming from our transport
    connectionThread = std::thread(&FtlConnection::startConnectionThread, this);
    connectionThread.detach();
}

void FtlConnection::Stop()
{
    // Stop the transport, which should halt our connection thread.
    transport->Stop();
    if (connectionThread.joinable())
    {
        connectionThread.join();
    }
}

void FtlConnection::SendOutro(std::string message)
{
    // TODO
}

void FtlConnection::SendStreamAvailable(const Stream& stream)
{
    // TODO
}

void FtlConnection::SendStreamRemoved(const Stream& stream)
{
    // TODO
}

void FtlConnection::SendStreamMetadata(const Stream& stream, uint32_t viewerCount)
{
    // TODO
}

void FtlConnection::SetOnConnectionClosed(std::function<void(void)> onConnectionClosed)
{
    this->onConnectionClosed = onConnectionClosed;
}

void FtlConnection::SetOnIntro(connection_cb_intro_t onIntro)
{
    this->onIntro = onIntro;
}

void FtlConnection::SetOnOutro(connection_cb_outro_t onOutro)
{
    this->onOutro = onOutro;
}

void FtlConnection::SetOnSubscribeChannel(connection_cb_subscribe_t onSubscribeChannel)
{
    this->onSubscribeChannel = onSubscribeChannel;
}

void FtlConnection::SetOnUnsubscribeChannel(connection_cb_unsubscribe_t onUnsubscribeChannel)
{
    this->onUnsubscribeChannel = onUnsubscribeChannel;
}

void FtlConnection::SetOnStreamAvailable(connection_cb_streamavailable_t onStreamAvailable)
{
    this->onStreamAvailable = onStreamAvailable;
}

void FtlConnection::SetOnStreamRemoved(connection_cb_streamremoved_t onStreamRemoved)
{
    this->onStreamRemoved = onStreamRemoved;
}

void FtlConnection::SetOnStreamMetadata(connection_cb_streammetadata_t onStreamMetadata)
{
    this->onStreamMetadata = onStreamMetadata;
}

std::string FtlConnection::GetHostname()
{
    return hostname;
}
#pragma endregion

#pragma region Private methods
void FtlConnection::startConnectionThread()
{
    std::optional<OrchestrationMessageHeader> messageHeader;
    std::vector<uint8_t> buffer;
    while (true)
    {
        if (isStopping)
        {
            break;
        }

        // Try to read in some data
        std::vector<uint8_t> readBytes = transport->Read();
        buffer.insert(buffer.end(), readBytes.begin(), readBytes.end());

        // Parse the header if we haven't already
        if (!messageHeader.has_value())
        {
            // Do we have enough bytes for a header?
            if (buffer.size() >= 4)
            {
                OrchestrationMessageHeader parsedHeader = ParseMessageHeader(buffer);
                messageHeader.emplace(parsedHeader);
            }
            else
            {
                // We need more bytes before we can deal with this message.
                continue;
            }
        }
        
        // Do we have all the payload bytes we need to process this message?
        uint16_t messagePayloadLength = messageHeader.value().MessagePayloadLength;
        if ((buffer.size() - 4) >= messagePayloadLength)
        {
            std::vector<uint8_t> messagePayload = std::vector<uint8_t>(
                (buffer.begin() + 4),
                (buffer.begin() + 4 + messagePayloadLength));

            // Process the message, then remove the message from the read buffer
            processMessage(messageHeader.value(), messagePayload);
            buffer.erase(buffer.begin(), (buffer.begin() + 4 + messagePayloadLength));
            messageHeader.reset();
        }
    }
}

void FtlConnection::onTransportConnectionClosed()
{
    isStopping = true;
}

void FtlConnection::processMessage(
    const OrchestrationMessageHeader& header,
    const std::vector<uint8_t>& payload)
{
    switch (header.MessageType)
    {
    case OrchestrationMessageType::Intro:
        processIntroMessage(header, payload);
        break;
    case OrchestrationMessageType::Outro:
        processOutroMessage(header, payload);
        break;
    case OrchestrationMessageType::SubscribeChannel:
        processSubscribeChannelMessage(header, payload);
        break;
    case OrchestrationMessageType::UnsubscribeChannel:
        processUnsubscribeChannelMessage(header, payload);
        break;
    case OrchestrationMessageType::StreamAvailable:
        processStreamAvailableMessage(header, payload);
        break;
    case OrchestrationMessageType::StreamRemoved:
        processStreamRemovedMessage(header, payload);
        break;
    case OrchestrationMessageType::StreamMetadata:
        processStreamMetadataMessage(header, payload);
        break;
    default:
        break;
    }
}

void FtlConnection::processIntroMessage(
    const OrchestrationMessageHeader& header,
    const std::vector<uint8_t>& payload)
{
    if (payload.size() < 3)
    {
        throw std::range_error("Intro payload is too small.");
    }

    // Extract version information and hostname from payload
    uint8_t versionMajor = payload.at(0);
    uint8_t versionMinor = payload.at(1);
    uint8_t revision = payload.at(2);
    hostname = std::string(payload.begin() + 3, payload.end());

    // Indicate that we received an intro
    ConnectionResult result
    {
        .IsSuccess = false
    };
    if (onIntro)
    {
        result = onIntro(versionMajor, versionMinor, revision, hostname);
    }

    // Send a response
    OrchestrationMessageHeader responseHeader
    {
        .MessageDirection = OrchestrationMessageDirectionKind::Response,
        .MessageFailure = !result.IsSuccess,
        .MessageType = OrchestrationMessageType::Intro,
        .MessageId = header.MessageId,
        .MessagePayloadLength = 0,
    };
    sendMessage(responseHeader, std::vector<uint8_t>());
}

void FtlConnection::processOutroMessage(
    const OrchestrationMessageHeader& header,
    const std::vector<uint8_t>& payload)
{
    // Extract reason from payload (if any)
    auto outroReason = std::string(payload.begin(), payload.end());

    // Indicate that we received an intro
    if (onOutro)
    {
        onOutro(outroReason);
    }

    // Send a response
    OrchestrationMessageHeader responseHeader
    {
        .MessageDirection = OrchestrationMessageDirectionKind::Response,
        .MessageFailure = false,
        .MessageType = OrchestrationMessageType::Outro,
        .MessageId = header.MessageId,
        .MessagePayloadLength = 0,
    };
    sendMessage(responseHeader, std::vector<uint8_t>());
}

void FtlConnection::processSubscribeChannelMessage(
    const OrchestrationMessageHeader& header,
    const std::vector<uint8_t>& payload)
{
    if (payload.size() < 4)
    {
        throw std::range_error("Subscribe payload is too small.");
    }

    // Extract channel ID from payload
    uint32_t channelId = DeserializeNetworkUint32(payload);

    // Indicate that we received a subscribe
    if (onSubscribeChannel)
    {
        onSubscribeChannel(channelId);
    }

    // Send a response
    OrchestrationMessageHeader responseHeader
    {
        .MessageDirection = OrchestrationMessageDirectionKind::Response,
        .MessageFailure = false,
        .MessageType = OrchestrationMessageType::SubscribeChannel,
        .MessageId = header.MessageId,
        .MessagePayloadLength = 0,
    };
    sendMessage(responseHeader, std::vector<uint8_t>());
}

void FtlConnection::processUnsubscribeChannelMessage(
    const OrchestrationMessageHeader& header,
    const std::vector<uint8_t>& payload)
{
    if (payload.size() < 4)
    {
        throw std::range_error("Unsubscribe payload is too small.");
    }

    // Extract channel ID from payload
    uint32_t channelId = DeserializeNetworkUint32(payload);

    // Indicate that we received a subscribe
    if (onUnsubscribeChannel)
    {
        onUnsubscribeChannel(channelId);
    }

    // Send a response
    OrchestrationMessageHeader responseHeader
    {
        .MessageDirection = OrchestrationMessageDirectionKind::Response,
        .MessageFailure = false,
        .MessageType = OrchestrationMessageType::UnsubscribeChannel,
        .MessageId = header.MessageId,
        .MessagePayloadLength = 0,
    };
    sendMessage(responseHeader, std::vector<uint8_t>());
}

void FtlConnection::processStreamAvailableMessage(
    const OrchestrationMessageHeader& header,
    const std::vector<uint8_t>& payload)
{
    if (payload.size() < 8)
    {
        throw std::range_error("Stream available payload is too small.");
    }

    // Extract data from payload
    uint32_t channelId = DeserializeNetworkUint32(
        std::vector<uint8_t>(payload.begin(), payload.begin() + 4));
    uint32_t streamId = DeserializeNetworkUint32(
        std::vector<uint8_t>(payload.begin() + 4, payload.begin() + 8));
    std::string hostname = std::string(payload.begin() + 8, payload.end());

    // Indicate that we received a subscribe
    if (onStreamAvailable)
    {
        onStreamAvailable(channelId, streamId, hostname);
    }

    // Send a response
    OrchestrationMessageHeader responseHeader
    {
        .MessageDirection = OrchestrationMessageDirectionKind::Response,
        .MessageFailure = false,
        .MessageType = OrchestrationMessageType::StreamAvailable,
        .MessageId = header.MessageId,
        .MessagePayloadLength = 0,
    };
    sendMessage(responseHeader, std::vector<uint8_t>());
}

void FtlConnection::processStreamRemovedMessage(
    const OrchestrationMessageHeader& header,
    const std::vector<uint8_t>& payload)
{
    if (payload.size() < 8)
    {
        throw std::range_error("Stream removed payload is too small.");
    }

    // Extract data from payload
    uint32_t channelId = DeserializeNetworkUint32(
        std::vector<uint8_t>(payload.begin(), payload.begin() + 4));
    uint32_t streamId = DeserializeNetworkUint32(
        std::vector<uint8_t>(payload.begin() + 4, payload.begin() + 8));

    // Indicate that we received a subscribe
    if (onStreamRemoved)
    {
        onStreamRemoved(channelId, streamId);
    }

    // Send a response
    OrchestrationMessageHeader responseHeader
    {
        .MessageDirection = OrchestrationMessageDirectionKind::Response,
        .MessageFailure = false,
        .MessageType = OrchestrationMessageType::StreamRemoved,
        .MessageId = header.MessageId,
        .MessagePayloadLength = 0,
    };
    sendMessage(responseHeader, std::vector<uint8_t>());
}

void FtlConnection::processStreamMetadataMessage(
    const OrchestrationMessageHeader& header,
    const std::vector<uint8_t>& payload)
{
    if (payload.size() < 12)
    {
        throw std::range_error("Stream metadata payload is too small.");
    }

    // Extract data from payload
    uint32_t channelId = DeserializeNetworkUint32(
        std::vector<uint8_t>(payload.begin(), payload.begin() + 4));
    uint32_t streamId = DeserializeNetworkUint32(
        std::vector<uint8_t>(payload.begin() + 4, payload.begin() + 8));
    uint32_t viewers = DeserializeNetworkUint32(
        std::vector<uint8_t>(payload.begin() + 8, payload.begin() + 12));

    // Indicate that we received a subscribe
    if (onStreamMetadata)
    {
        onStreamMetadata(channelId, streamId, viewers);
    }

    // Send a response
    OrchestrationMessageHeader responseHeader
    {
        .MessageDirection = OrchestrationMessageDirectionKind::Response,
        .MessageFailure = false,
        .MessageType = OrchestrationMessageType::StreamMetadata,
        .MessageId = header.MessageId,
        .MessagePayloadLength = 0,
    };
    sendMessage(responseHeader, std::vector<uint8_t>());
}

void FtlConnection::sendMessage(
    const OrchestrationMessageHeader& header,
    const std::vector<uint8_t>& payload)
{
    std::vector<uint8_t> sendBuffer = SerializeMessageHeader(header);
    sendBuffer.reserve(4 + payload.size());

    // Append payload
    sendBuffer.insert(sendBuffer.end(), payload.begin(), payload.end());

    // Send it!
    transport->Write(sendBuffer);
}
#pragma endregion