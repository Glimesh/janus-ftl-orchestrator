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

void FtlConnection::SendStreamAvailable(std::shared_ptr<Stream> stream)
{

}

void FtlConnection::SendStreamEnded(std::shared_ptr<Stream> stream)
{
    
}

void FtlConnection::SetOnConnectionClosed(std::function<void(void)> onConnectionClosed)
{
    this->onConnectionClosed = onConnectionClosed;
}

void FtlConnection::SetOnIngestNewStream(
    std::function<void(ftl_channel_id_t, ftl_stream_id_t)> onIngestNewStream)
{
    this->onIngestNewStream = onIngestNewStream;
}

void FtlConnection::SetOnIngestStreamEnded(
    std::function<void(ftl_channel_id_t, ftl_stream_id_t)> onIngestStreamEnded)
{
    this->onIngestStreamEnded = onIngestStreamEnded;
}

void FtlConnection::SetOnStreamViewersUpdated(
    std::function<void(ftl_channel_id_t, ftl_stream_id_t, uint32_t)> onStreamViewersUpdated)
{
    this->onStreamViewersUpdated = onStreamViewersUpdated;
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
    while (true)
    {
        if (isStopping)
        {
            break;
        }

        // Try to read in some data
        std::vector<uint8_t> buffer;
        std::vector<uint8_t> readBytes = transport->Read();
        buffer.insert(buffer.end(), readBytes.begin(), readBytes.end());

        // Parse the header if we haven't already
        if (!messageHeader.has_value())
        {
            // Do we have enough bytes for a header?
            if (buffer.size() >= 4)
            {
                OrchestrationMessageHeader parsedHeader = parseMessageHeader(buffer);
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

            
        }
    }

}

void FtlConnection::onTransportConnectionClosed()
{
    isStopping = true;
}

OrchestrationMessageHeader FtlConnection::parseMessageHeader(const std::vector<uint8_t>& bytes)
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
    // Payload contains the hostname of the client
    hostname = std::string(payload.begin(), payload.end());

    // Send a response
    OrchestrationMessageHeader responseHeader
    {
        .MessageDirection = OrchestrationMessageDirectionKind::Response,
        .MessageFailure = false,
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
    // TODO
}

void FtlConnection::processSubscribeChannelMessage(
    const OrchestrationMessageHeader& header,
    const std::vector<uint8_t>& payload)
{
    // TODO
}

void FtlConnection::processUnsubscribeChannelMessage(
    const OrchestrationMessageHeader& header,
    const std::vector<uint8_t>& payload)
{
    // TODO
}

void FtlConnection::processStreamAvailableMessage(
    const OrchestrationMessageHeader& header,
    const std::vector<uint8_t>& payload)
{
    // TODO
}

void FtlConnection::processStreamRemovedMessage(
    const OrchestrationMessageHeader& header,
    const std::vector<uint8_t>& payload)
{
    // TODO
}

void FtlConnection::processStreamMetadataMessage(
    const OrchestrationMessageHeader& header,
    const std::vector<uint8_t>& payload)
{
    // TODO
}

void FtlConnection::sendMessage(
    const OrchestrationMessageHeader& header,
    const std::vector<uint8_t>& payload)
{
    std::vector<uint8_t> sendBuffer;
    sendBuffer.reserve(4 + payload.size());

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
    sendBuffer.emplace_back(messageDesc);
    sendBuffer.emplace_back(header.MessageId);

    // Append payload size
    // Do we need to flip to network byte order (big-endian) ?
    if (std::endian::native != std::endian::big)
    {
        sendBuffer.emplace_back(static_cast<uint8_t>((header.MessagePayloadLength & 0xFF00) >> 8));
        sendBuffer.emplace_back(static_cast<uint8_t>(header.MessagePayloadLength & 0x00FF));
    }
    else
    {
        sendBuffer.emplace_back(static_cast<uint8_t>(header.MessagePayloadLength & 0x00FF));
        sendBuffer.emplace_back(static_cast<uint8_t>((header.MessagePayloadLength & 0xFF00) >> 8));
    }

    // Append payload
    sendBuffer.insert(sendBuffer.end(), payload.begin(), payload.end());

    // Send it!
    transport->Write(sendBuffer);
}
#pragma endregion