/**
 * @file FtlConnection.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-11-08
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#pragma once

#include "FtlTypes.h"
#include "IConnection.h"
#include "IConnectionTransport.h"
#include "OrchestrationProtocolTypes.h"

#include <atomic>
#include <bit>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <spdlog/spdlog.h>
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
    FtlConnection(
        std::shared_ptr<IConnectionTransport> transport,
        std::string hostname = std::string()) : 
        transport(transport),
        hostname(hostname)
    { }

    ~FtlConnection()
    {
        // If we haven't already stopped, this should handle it.
        Stop();
    }

    /* Static methods */
    /**
     * @brief Attempts to parse an Orchestration Protocol Message Header out of the given bytes
     * @param bytes bytes to parse
     * @return OrchestrationMessageHeader resulting header information
     */
    static OrchestrationMessageHeader ParseMessageHeader(const std::vector<std::byte>& bytes)
    {
        if (bytes.size() < 4)
        {
            throw std::range_error("Attempt to parse message header that is under 4 bytes.");
        }

        std::byte messageDesc = bytes.at(0);
        OrchestrationMessageDirectionKind messageDirection = 
            ((messageDesc & std::byte{0b10000000}) == std::byte{0}) ?
                OrchestrationMessageDirectionKind::Request : 
                OrchestrationMessageDirectionKind::Response;
        bool messageIsFailure = ((messageDesc & std::byte{0b01000000}) != std::byte{0});
        OrchestrationMessageType messageType = 
            static_cast<OrchestrationMessageType>(messageDesc & std::byte{0b00111111});
        uint8_t messageId = static_cast<uint8_t>(bytes.at(1));
        // Determine if we need to flip things around if the host byte ordering is not the same
        // as network byte ordering (big endian)
        uint16_t payloadLength;
        if (std::endian::native != std::endian::big)
        {
            payloadLength = (static_cast<uint16_t>(bytes.at(3)) << 8) | 
                static_cast<uint16_t>(bytes.at(2));
        }
        else
        {
            payloadLength = (static_cast<uint16_t>(bytes.at(2)) << 8) | 
                static_cast<uint16_t>(bytes.at(3));
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

    /**
     * @brief Serializes an Orchestration Protocol Message Header to a byte array
     * @param header header to serialize
     * @return std::vector<std::byte> serialized bytes
     */
    static std::vector<std::byte> SerializeMessageHeader(const OrchestrationMessageHeader& header)
    {
        std::vector<std::byte> headerBytes;
        headerBytes.reserve(4);

        // Convert header to byte payload
        std::byte messageDesc = static_cast<std::byte>(header.MessageType);
        if (header.MessageDirection == OrchestrationMessageDirectionKind::Response)
        {
            messageDesc = (messageDesc | std::byte{0b10000000});
        }
        if (header.MessageFailure)
        {
            messageDesc = (messageDesc | std::byte{0b01000000});
        }
        headerBytes.emplace_back(messageDesc);
        headerBytes.emplace_back(static_cast<std::byte>(header.MessageId));

        // Encode payload length
        std::vector<std::byte> payloadLengthBytes = ConvertToNetworkPayload(header.MessagePayloadLength);
        headerBytes.insert(headerBytes.end(), payloadLengthBytes.begin(), payloadLengthBytes.end());

        return headerBytes;
    }

    /**
     * @brief
     *  Takes a value and converts it to a payload suitable for sending over the network
     *  (swapping byte order if necessary)
     * 
     * @param value The value to convert
     * @return std::vector<std::byte> The resulting payload
     */
    static std::vector<std::byte> ConvertToNetworkPayload(const uint16_t value)
    {
        std::vector<std::byte> payload;
        payload.reserve(2); // 16 bits

        if (std::endian::native != std::endian::big)
        {
            payload.emplace_back(static_cast<std::byte>(value & 0x00FF));
            payload.emplace_back(static_cast<std::byte>((value >> 8) & 0x00FF));
        }
        else
        {
            payload.emplace_back(static_cast<std::byte>((value >> 8) & 0x00FF));
            payload.emplace_back(static_cast<std::byte>(value & 0x00FF));
        }

        return payload;
    }

    /**
     * @brief
     *  Takes a value and converts it to a payload suitable for sending over the network
     *  (swapping byte order if necessary)
     * 
     * @param value The value to convert
     * @return std::vector<std::byte> The resulting payload
     */
    static std::vector<std::byte> ConvertToNetworkPayload(const uint32_t value)
    {
        std::vector<std::byte> payload;
        payload.reserve(4); // 32 bits

        if (std::endian::native != std::endian::big)
        {
            payload.emplace_back(static_cast<std::byte>(value & 0x000000FF));
            payload.emplace_back(static_cast<std::byte>((value >> 8) & 0x000000FF));
            payload.emplace_back(static_cast<std::byte>((value >> 16) & 0x000000FF));
            payload.emplace_back(static_cast<std::byte>((value >> 24) & 0x000000FF));
        }
        else
        {
            payload.emplace_back(static_cast<std::byte>((value >> 24) & 0x000000FF));
            payload.emplace_back(static_cast<std::byte>((value >> 16) & 0x000000FF));
            payload.emplace_back(static_cast<std::byte>((value >> 8) & 0x000000FF));
            payload.emplace_back(static_cast<std::byte>(value & 0x000000FF));
        }

        return payload;
    }

    /**
     * @brief Takes a network encoded byte payload and deserializes it to a host uint16_t value
     * @param payload Payload to deserialize
     * @return uint16_t Resulting value
     */
    static uint16_t DeserializeNetworkUint16(
        const std::vector<std::byte>::const_iterator& begin,
        const std::vector<std::byte>::const_iterator& end)
    {
        if ((end - begin) != 2)
        {
            throw std::range_error("Deserializing uint16 requires a 2 byte payload.");
        }

        if (std::endian::native != std::endian::big)
        {
            return (static_cast<uint16_t>(*(begin + 1)) << 8) | 
                static_cast<uint16_t>(*begin);
        }
        else
        {
            return (static_cast<uint32_t>(*begin) << 8) | 
                static_cast<uint32_t>(*(begin + 1));
        }
    }

    /**
     * @brief Takes a network encoded byte payload and deserializes it to a host uint32_t value
     * @param payload Payload to deserialize
     * @return uint32_t Resulting value
     */
    static uint32_t DeserializeNetworkUint32(
        const std::vector<std::byte>::const_iterator& begin,
        const std::vector<std::byte>::const_iterator& end)
    {
        if ((end - begin) != 4)
        {
            throw std::range_error("Deserializing uint32 requires a 4 byte payload.");
        }

        if (std::endian::native != std::endian::big)
        {
            return (static_cast<uint32_t>(*(begin + 3)) << 24) | 
                (static_cast<uint32_t>(*(begin + 2)) << 16) | 
                (static_cast<uint32_t>(*(begin + 1)) << 8) | 
                static_cast<uint32_t>(*begin);
        }
        else
        {
            return (static_cast<uint32_t>(*begin) << 24) | 
                (static_cast<uint32_t>(*(begin + 1)) << 16) | 
                (static_cast<uint32_t>(*(begin + 2)) << 8) | 
                static_cast<uint32_t>(*(begin + 3));
        }
    }

    /**
     * @brief Given a payload of bytes, append a string to the end. Encapsulates annoying casting.
     * @param payload Payload to append to
     * @param string String to append
     */
    static void AppendStringToPayload(std::vector<std::byte>& payload, const std::string& string)
    {
        payload.insert(
            payload.end(),
            reinterpret_cast<const std::byte*>(string.data()),
            (reinterpret_cast<const std::byte*>(string.data()) + string.size()));
    }

    /* IConnection */
    void Start() override
    {
        // Bind to transport events
        transport->SetOnBytesReceived(
            std::bind(
                &FtlConnection::onTransportBytesReceived,
                this,
                std::placeholders::_1));
        transport->SetOnConnectionClosed(std::bind(&FtlConnection::onTransportConnectionClosed, this));

        // Start the transport
        transport->Start();
    }

    void Stop() override
    {
        // Stop the transport, which should halt our connection thread.
        transport->Stop();
    }

    void SendIntro(const ConnectionIntroPayload& payload) override
    {
        // Construct the binary payload
        std::vector<std::byte> messagePayload
        {
            static_cast<std::byte>(payload.VersionMajor),
            static_cast<std::byte>(payload.VersionMinor),
            static_cast<std::byte>(payload.VersionRevision),
            static_cast<std::byte>(payload.RelayLayer),
        };
        auto regionCodeLength = FtlConnection::ConvertToNetworkPayload(
            static_cast<uint16_t>(payload.RegionCode.size()));
        messagePayload.insert(
            messagePayload.end(),
            regionCodeLength.begin(),
            regionCodeLength.end());
        AppendStringToPayload(messagePayload, payload.RegionCode);
        AppendStringToPayload(messagePayload, payload.Hostname);

        // Construct the message header
        OrchestrationMessageHeader header
        {
            .MessageDirection = OrchestrationMessageDirectionKind::Request,
            .MessageFailure = false,
            .MessageType = OrchestrationMessageType::Intro,
            .MessageId = nextOutgoingMessageId++,
            .MessagePayloadLength = static_cast<uint16_t>(messagePayload.size()),
        };

        // Send it!
        sendMessage(header, messagePayload);
    }
    
    void SendOutro(const ConnectionOutroPayload& payload) override
    {
        std::vector<std::byte> messagePayload;
        AppendStringToPayload(messagePayload, payload.DisconnectReason);
        
        OrchestrationMessageHeader header
        {
            .MessageDirection = OrchestrationMessageDirectionKind::Request,
            .MessageFailure = false,
            .MessageType = OrchestrationMessageType::Outro,
            .MessageId = nextOutgoingMessageId++,
            .MessagePayloadLength = static_cast<uint16_t>(messagePayload.size()),
        };

        sendMessage(header, messagePayload);
    }

    void SendNodeState(const ConnectionNodeStatePayload& payload) override
    {
        std::vector<std::byte> messagePayload;
        messagePayload.reserve(8);
        auto currentLoad = ConvertToNetworkPayload(payload.CurrentLoad);
        messagePayload.insert(messagePayload.end(), currentLoad.begin(), currentLoad.end());
        auto maximumLoad = ConvertToNetworkPayload(payload.MaximumLoad);
        messagePayload.insert(messagePayload.end(), maximumLoad.begin(), maximumLoad.end());

        OrchestrationMessageHeader header
        {
            .MessageDirection = OrchestrationMessageDirectionKind::Request,
            .MessageFailure = false,
            .MessageType = OrchestrationMessageType::NodeState,
            .MessageId = nextOutgoingMessageId++,
            .MessagePayloadLength = static_cast<uint16_t>(messagePayload.size()),
        };

        sendMessage(header, messagePayload);
    }

    void SendChannelSubscription(const ConnectionSubscriptionPayload& payload) override
    {
        std::vector<std::byte> messagePayload
        {
            static_cast<std::byte>(payload.IsSubscribe),
        };

        messagePayload.reserve(5 + payload.StreamKey.size());
        auto channelIdBytes = ConvertToNetworkPayload(payload.ChannelId);
        messagePayload.insert(messagePayload.end(), channelIdBytes.begin(), channelIdBytes.end());
        messagePayload.insert(
            messagePayload.end(),
            payload.StreamKey.begin(),
            payload.StreamKey.end());

        OrchestrationMessageHeader header
        {
            .MessageDirection = OrchestrationMessageDirectionKind::Request,
            .MessageFailure = false,
            .MessageType = OrchestrationMessageType::ChannelSubscription,
            .MessageId = nextOutgoingMessageId++,
            .MessagePayloadLength = static_cast<uint16_t>(messagePayload.size()),
        };

        sendMessage(header, messagePayload);
    }
    
    void SendStreamPublish(const ConnectionPublishPayload& payload) override
    {
        std::vector<std::byte> messagePayload
        {
            static_cast<std::byte>(payload.IsPublish),
        };
        messagePayload.reserve(9);
        auto channelIdBytes = ConvertToNetworkPayload(payload.ChannelId);
        messagePayload.insert(messagePayload.end(), channelIdBytes.begin(), channelIdBytes.end());
        auto streamIdBytes = ConvertToNetworkPayload(payload.StreamId);
        messagePayload.insert(messagePayload.end(), streamIdBytes.begin(), streamIdBytes.end());

        OrchestrationMessageHeader header
        {
            .MessageDirection = OrchestrationMessageDirectionKind::Request,
            .MessageFailure = false,
            .MessageType = OrchestrationMessageType::StreamPublish,
            .MessageId = nextOutgoingMessageId++,
            .MessagePayloadLength = static_cast<uint16_t>(messagePayload.size()),
        };

        sendMessage(header, messagePayload);
    }

    void SendStreamRelay(const ConnectionRelayPayload& payload) override
    {
        std::vector<std::byte> messagePayload
        {
            static_cast<uint8_t>(payload.IsStartRelay),
        };
        messagePayload.reserve(11 + payload.TargetHostname.size() + payload.StreamKey.size());
        auto channelIdBytes = ConvertToNetworkPayload(payload.ChannelId);
        messagePayload.insert(messagePayload.end(), channelIdBytes.begin(), channelIdBytes.end());
        auto streamIdBytes = ConvertToNetworkPayload(payload.StreamId);
        messagePayload.insert(messagePayload.end(), streamIdBytes.begin(), streamIdBytes.end());
        auto hostnameSizeBytes = ConvertToNetworkPayload(
            static_cast<uint16_t>(payload.TargetHostname.size()));
        messagePayload.insert(
            messagePayload.end(),
            hostnameSizeBytes.begin(),
            hostnameSizeBytes.end());
        AppendStringToPayload(messagePayload, payload.TargetHostname);
        messagePayload.insert(
            messagePayload.end(),
            payload.StreamKey.begin(),
            payload.StreamKey.end());

        OrchestrationMessageHeader header
        {
            .MessageDirection = OrchestrationMessageDirectionKind::Request,
            .MessageFailure = false,
            .MessageType = OrchestrationMessageType::StreamRelay,
            .MessageId = nextOutgoingMessageId++,
            .MessagePayloadLength = static_cast<uint16_t>(messagePayload.size()),
        };

        sendMessage(header, messagePayload);
    }

    void SetOnConnectionClosed(std::function<void(void)> onConnectionClosed) override
    {
        this->onConnectionClosed = onConnectionClosed;
    }

    void SetOnIntro(connection_cb_intro_t onIntro) override
    {
        this->onIntro = onIntro;
    }

    void SetOnOutro(connection_cb_outro_t onOutro) override
    {
        this->onOutro = onOutro;
    }

    void SetOnNodeState(connection_cb_nodestate_t onNodeState) override
    {
        this->onNodeState = onNodeState;
    }

    void SetOnChannelSubscription(connection_cb_subscription_t onChannelSubscription) override
    {
        this->onChannelSubscription = onChannelSubscription;
    }

    void SetOnStreamPublish(connection_cb_publishing_t onStreamPublish) override
    {
        this->onStreamPublish = onStreamPublish;
    }

    void SetOnStreamRelay(connection_cb_relay_t onStreamRelay) override
    {
        this->onStreamRelay = onStreamRelay;
    }

    std::string GetHostname() override
    {
        return hostname;
    }

private:
    std::shared_ptr<IConnectionTransport> transport;
    std::vector<std::byte> transportReadBuffer;
    std::optional<OrchestrationMessageHeader> parsedTransportMessageHeader;
    std::function<void(void)> onConnectionClosed;
    connection_cb_intro_t onIntro;
    connection_cb_outro_t onOutro;
    connection_cb_nodestate_t onNodeState;
    connection_cb_subscription_t onChannelSubscription;
    connection_cb_publishing_t onStreamPublish;
    connection_cb_relay_t onStreamRelay;
    std::string hostname;
    std::atomic<uint8_t> nextOutgoingMessageId { 0 };

    /* Private methods */
    /**
     * @brief Called when underlying transport has received new data
     * @param bytes data from transport
     */
    void onTransportBytesReceived(const std::vector<std::byte>& bytes)
    {
        // Add received bytes to our buffer
        spdlog::info("{} received {} bytes ...", GetHostname(), bytes.size());
        transportReadBuffer.insert(transportReadBuffer.end(), bytes.begin(), bytes.end());

        // Parse the header if we haven't already
        if (!parsedTransportMessageHeader.has_value())
        {
            // Do we have enough bytes for a header?
            if (transportReadBuffer.size() >= 4)
            {
                OrchestrationMessageHeader parsedHeader = ParseMessageHeader(transportReadBuffer);
                parsedTransportMessageHeader.emplace(parsedHeader);
            }
            else
            {
                // We need more bytes before we can deal with this message.
                // Wait for our transport to deliver us more juicy data.
                return;
            }
        }
        
        // Do we have all the payload bytes we need to process this message?
        uint16_t messagePayloadLength = parsedTransportMessageHeader.value().MessagePayloadLength;
        if ((transportReadBuffer.size() - 4) >= messagePayloadLength)
        {
            std::vector<std::byte> messagePayload(
                (transportReadBuffer.begin() + 4),
                (transportReadBuffer.begin() + 4 + messagePayloadLength));

            // Process the message, then remove the message from the read buffer
            processMessage(parsedTransportMessageHeader.value(), messagePayload);
            transportReadBuffer.erase(
                transportReadBuffer.begin(),
                (transportReadBuffer.begin() + 4 + messagePayloadLength));
            parsedTransportMessageHeader.reset();
        }
    }

    /**
     * @brief Called when underlying transport has closed.
     */
    void onTransportConnectionClosed()
    {
        if (onConnectionClosed)
        {
            onConnectionClosed();
        }
    }

    /**
     * @brief Processes an Orchestration Protocol Message with a payload.
     * @param header the parsed header
     * @param payload the raw payload data
     */
    void processMessage(
        const OrchestrationMessageHeader& header,
        const std::vector<std::byte>& payload)
    {
        if (header.MessageDirection == OrchestrationMessageDirectionKind::Response)
        {
            // TODO: We don't handle responses yet.
            return;
        }

        switch (header.MessageType)
        {
        case OrchestrationMessageType::Intro:
            processIntroMessage(header, payload);
            break;
        case OrchestrationMessageType::Outro:
            processOutroMessage(header, payload);
            break;
        case OrchestrationMessageType::NodeState:
            processNodeStateMessage(header, payload);
            break;
        case OrchestrationMessageType::ChannelSubscription:
            processChannelSubscriptionMessage(header, payload);
            break;
        case OrchestrationMessageType::StreamPublish:
            processStreamPublishMessage(header, payload);
            break;
        case OrchestrationMessageType::StreamRelay:
            processStreamRelayMessage(header, payload);
            break;
        default:
            break;
        }
    }

    /**
     * @brief Process Orchestration Protocol Message of type Intro
     */
    void processIntroMessage(
        const OrchestrationMessageHeader& header,
        const std::vector<std::byte>& payload)
    {
        if (payload.size() < 6)
        {
            throw std::range_error("Intro payload is too small.");
        }

        // Extract payload data
        uint16_t regionCodeLength = 
            DeserializeNetworkUint16((payload.cbegin() + 4), (payload.cbegin() + 6));

        // Make sure the given region code length doesn't cause us to run off the edge of the payload.
        if ((regionCodeLength + 6ul) > payload.size())
        {
            spdlog::error(
                "FtlConnection: Invalid Intro payload. Region Code of length {} "
                "bytes @ 6 byte offset runs off the edge of {} byte payload.",
                regionCodeLength,
                payload.size());

            // Send an error response
            OrchestrationMessageHeader responseHeader
            {
                .MessageDirection = OrchestrationMessageDirectionKind::Response,
                .MessageFailure = true,
                .MessageType = header.MessageType,
                .MessageId = header.MessageId,
                .MessagePayloadLength = 0,
            };
            sendMessage(responseHeader, std::vector<std::byte>());
            return;
        }

        ConnectionIntroPayload introPayload
        {
            .VersionMajor = static_cast<uint8_t>(payload.at(0)),
            .VersionMinor = static_cast<uint8_t>(payload.at(1)),
            .VersionRevision = static_cast<uint8_t>(payload.at(2)),
            .RelayLayer = static_cast<uint8_t>(payload.at(3)),
            // (bytes 4, 5 are region code length)
            .RegionCode = std::string(
                (reinterpret_cast<const char*>(payload.data()) + 6),
                (reinterpret_cast<const char*>(payload.data()) + 6 + regionCodeLength)),
            .Hostname = std::string(
                (reinterpret_cast<const char*>(payload.data()) + 6 + regionCodeLength),
                (reinterpret_cast<const char*>(payload.data()) + payload.size())),
        };

        // Indicate that we received an intro
        ConnectionResult result
        {
            .IsSuccess = false
        };
        if (onIntro)
        {
            result = onIntro(introPayload);
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
        sendMessage(responseHeader, std::vector<std::byte>());
    }

    /**
     * @brief Process Orchestration Protocol Message of type Outro
     */
    void processOutroMessage(
        const OrchestrationMessageHeader& header,
        const std::vector<std::byte>& payload)
    {
        ConnectionOutroPayload outroPayload
        {
            .DisconnectReason = std::string(
                reinterpret_cast<const char*>(payload.data()),
                payload.size())
        };

        // Indicate that we received an intro
        if (onOutro)
        {
            onOutro(outroPayload);
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
        sendMessage(responseHeader, std::vector<std::byte>());
    }

    /**
     * @brief Process Orchestration Protocol Message of type Node State
     */
    void processNodeStateMessage(
        const OrchestrationMessageHeader& header,
        const std::vector<std::byte>& payload)
    {
        if (payload.size() < 8)
        {
            spdlog::error(
                "FtlConnection: Invalid Node State payload. Expected 8 bytes, got {}.",
                payload.size());

            // Send an error response
            OrchestrationMessageHeader responseHeader
            {
                .MessageDirection = OrchestrationMessageDirectionKind::Response,
                .MessageFailure = true,
                .MessageType = header.MessageType,
                .MessageId = header.MessageId,
                .MessagePayloadLength = 0,
            };
            sendMessage(responseHeader, std::vector<std::byte>());
            return;
        }
        
        ConnectionNodeStatePayload nodeStatePayload
        {
            .CurrentLoad = DeserializeNetworkUint32(payload.cbegin(), (payload.cbegin() + 4)),
            .MaximumLoad = DeserializeNetworkUint32((payload.cbegin() + 4), (payload.cbegin() + 8)),
        };

        // Indicate that we received a node state update
        if (onNodeState)
        {
            onNodeState(nodeStatePayload);
        }

        // Send a response
        OrchestrationMessageHeader responseHeader
        {
            .MessageDirection = OrchestrationMessageDirectionKind::Response,
            .MessageFailure = false,
            .MessageType = header.MessageType,
            .MessageId = header.MessageId,
            .MessagePayloadLength = 0,
        };
        sendMessage(responseHeader, std::vector<std::byte>());
    }

    /**
     * @brief Process Orchestration Protocol Message of type Channel Subscription
     */
    void processChannelSubscriptionMessage(
        const OrchestrationMessageHeader& header,
        const std::vector<std::byte>& payload)
    {
        if (payload.size() < 5)
        {
            throw std::range_error("Subscription payload is too small.");
        }

        // TODO: We should be using std::byte everywhere...
        std::vector<std::byte> streamKey((payload.begin() + 5), payload.end());
        ConnectionSubscriptionPayload subPayload
        {
            .IsSubscribe = (static_cast<uint8_t>(payload.at(0)) == 1),
            .ChannelId = DeserializeNetworkUint32((payload.cbegin() + 1), (payload.cbegin() + 5)),
            .StreamKey = streamKey,
        };

        // Indicate that we received a subscribe
        if (onChannelSubscription)
        {
            onChannelSubscription(subPayload);
        }

        // Send a response
        OrchestrationMessageHeader responseHeader
        {
            .MessageDirection = OrchestrationMessageDirectionKind::Response,
            .MessageFailure = false,
            .MessageType = OrchestrationMessageType::ChannelSubscription,
            .MessageId = header.MessageId,
            .MessagePayloadLength = 0,
        };
        sendMessage(responseHeader, std::vector<std::byte>());
    }

    /**
     * @brief Process Orchestration Protocol Message of type Stream Publishing
     */
    void processStreamPublishMessage(
        const OrchestrationMessageHeader& header,
        const std::vector<std::byte>& payload)
    {
        if (payload.size() < 9)
        {
            throw std::range_error("Stream publish payload is too small.");
        }

        ConnectionPublishPayload publishPayload
        {
            .IsPublish = (static_cast<uint8_t>(payload.at(0)) == 1),
            .ChannelId = DeserializeNetworkUint32((payload.cbegin() + 1), (payload.cbegin() + 5)),
            .StreamId = DeserializeNetworkUint32((payload.cbegin() + 5), (payload.cbegin() + 9)),
        };

        // Indicate that we received a subscribe
        if (onStreamPublish)
        {
            onStreamPublish(publishPayload);
        }

        // Send a response
        OrchestrationMessageHeader responseHeader
        {
            .MessageDirection = OrchestrationMessageDirectionKind::Response,
            .MessageFailure = false,
            .MessageType = OrchestrationMessageType::StreamPublish,
            .MessageId = header.MessageId,
            .MessagePayloadLength = 0,
        };
        sendMessage(responseHeader, std::vector<std::byte>());
    }

    /**
     * @brief Process Orchestration Protocol Message of type Stream Relay
     */
    void processStreamRelayMessage(
        const OrchestrationMessageHeader& header,
        const std::vector<std::byte>& payload)
    {
        if (payload.size() < 11)
        {
            throw std::range_error("Stream relay payload is too small.");
        }

        // Extract some data needed to build the payload
        uint16_t hostnameLength = 
            DeserializeNetworkUint16((payload.cbegin() + 9), (payload.cbegin() + 11));

        // Make sure the given hostname length doesn't cause us to run off the edge of the payload.
        if ((hostnameLength + 11ul) > payload.size())
        {
            spdlog::error(
                "FtlConnection: Invalid Stream Relay payload. Hostname of length {} "
                "bytes @ 11 byte offset runs off the edge of {} byte payload.",
                hostnameLength,
                payload.size());

            // Send an error response
            OrchestrationMessageHeader responseHeader
            {
                .MessageDirection = OrchestrationMessageDirectionKind::Response,
                .MessageFailure = true,
                .MessageType = OrchestrationMessageType::StreamRelay,
                .MessageId = header.MessageId,
                .MessagePayloadLength = 0,
            };
            sendMessage(responseHeader, std::vector<std::byte>());
            return;
        }

        ConnectionRelayPayload relayPayload
        {
            .IsStartRelay = (static_cast<uint8_t>(payload.at(0)) == 1),
            .ChannelId = DeserializeNetworkUint32((payload.cbegin() + 1), (payload.cbegin() + 5)),
            .StreamId = DeserializeNetworkUint32((payload.cbegin() + 5), (payload.cbegin() + 9)),
            // (bytes 10 - 11 are the hostname length)
            .TargetHostname = std::string(
                (reinterpret_cast<const char*>(payload.data()) + 11),
                (reinterpret_cast<const char*>(payload.data()) + 11 + hostnameLength)),
            .StreamKey = std::vector<std::byte>(payload.cbegin() + 11, payload.cend()),
        };

        // Indicate that we received a relay
        if (onStreamRelay)
        {
            onStreamRelay(relayPayload);
        }

        // Send a response
        OrchestrationMessageHeader responseHeader
        {
            .MessageDirection = OrchestrationMessageDirectionKind::Response,
            .MessageFailure = false,
            .MessageType = OrchestrationMessageType::StreamRelay,
            .MessageId = header.MessageId,
            .MessagePayloadLength = 0,
        };
        sendMessage(responseHeader, std::vector<std::byte>());
    }

    /**
     * @brief Sends the given FTL Orchestration Protocol message across the transport
     * @param header Orchestration Protocol Message Header
     * @param payload Orchestration Protocol Message Payload
     */
    void sendMessage(
        const OrchestrationMessageHeader& header,
        const std::vector<std::byte>& payload)
    {
        std::vector<std::byte> sendBuffer = SerializeMessageHeader(header);
        sendBuffer.reserve(4 + payload.size());

        // Append payload
        sendBuffer.insert(sendBuffer.end(), payload.begin(), payload.end());

        // Send it!
        transport->Write(sendBuffer);
    }
};