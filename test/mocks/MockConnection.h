/**
 * @file MockConnection.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-10-25
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#pragma once

#include "../../src/IConnection.h"

#include <functional>
#include <string>
#include <tuple>
#include <vector>

class MockConnection : public IConnection
{
public:
    // Mock constructor
    MockConnection(std::string hostname) : 
        hostname(hostname)
    { }

    // Mock destructor
    ~MockConnection() override
    {
        if (onDestructed)
        {
            onDestructed();
        }
    }

    // Mock utilities
    bool IsStreamAvailable(ftl_channel_id_t channelId)
    {
        for (const auto& stream : availableStreams)
        {
            if (stream.ChannelId == channelId)
            {
                return true;
            }
        }
        return false;
    }

    bool IsStreamAvailable(ftl_channel_id_t channelId, ftl_stream_id_t streamId)
    {
        for (const auto& stream : availableStreams)
        {
            if ((stream.ChannelId == channelId) && (stream.StreamId == streamId))
            {
                return true;
            }
        }
        return false;
    }

    void MockFireOnConnectionClosed()
    {
        onConnectionClosed();
    }

    void MockFireOnIntro(
        uint8_t versionMajor,
        uint8_t versionMinor,
        uint8_t versionRevision,
        std::string host)
    {
        if (onIntro)
        {
            onIntro(versionMajor, versionMinor, versionRevision, host);
        }
    }

    void MockFireOnSubscribeChannel(ftl_channel_id_t channelId)
    {
        if (onSubscribeChannel)
        {
            onSubscribeChannel(channelId);
        }
    }

    void MockFireOnStreamAvailable(
        ftl_channel_id_t channelId,
        ftl_stream_id_t streamId,
        std::string hostname)
    {
        if (onStreamAvailable)
        {
            onStreamAvailable(channelId, streamId, hostname);
        }
    }

    void MockFireOnStreamRemoved(ftl_channel_id_t channelId, ftl_stream_id_t streamId)
    {
        if (onStreamRemoved)
        {
            onStreamRemoved(channelId, streamId);
        }
    }

    void MockFireOnStreamMetadata(
        ftl_channel_id_t channelId,
        ftl_stream_id_t streamId,
        uint32_t viewerCount)
    {
        if (onStreamMetadata)
        {
            onStreamMetadata(channelId, streamId, viewerCount);
        }
    }

    void SetMockOnDestructed(std::function<void(void)> onDestructed)
    {
        this->onDestructed = onDestructed;
    }

    void SetMockOnSendStreamAvailable(
        std::function<void(Stream)> mockOnSendStreamAvailable)
    {
        this->mockOnSendStreamAvailable = mockOnSendStreamAvailable;
    }

    void SetMockOnSendStreamRemoved(
        std::function<void(Stream)> mockOnSendStreamRemoved)
    {
        this->mockOnSendStreamRemoved = mockOnSendStreamRemoved;
    }

    void SetMockOnSendStreamMetadata(
        std::function<void(Stream, uint32_t)> mockOnSendStreamMetadata)
    {
        this->mockOnSendStreamMetadata = mockOnSendStreamMetadata;
    }

    // IConnection
    void Start() override
    { }

    void Stop() override
    { }

    void SendOutro(std::string message) override
    { 
        // TODO
    }

    void SendStreamAvailable(const Stream& stream) override
    {
        availableStreams.push_back(stream);

        if (mockOnSendStreamAvailable)
        {
            mockOnSendStreamAvailable(stream);
        }
    }

    void SendStreamRemoved(const Stream& stream) override
    {
        availableStreams.erase(
            std::remove_if(
                availableStreams.begin(),
                availableStreams.end(),
                [&stream](auto availableStream)
                {
                    return (
                        (stream.ChannelId == availableStream.ChannelId) && 
                        (stream.StreamId == availableStream.StreamId));
                }),
            availableStreams.end());

        if (mockOnSendStreamRemoved)
        {
            mockOnSendStreamRemoved(stream);
        }
    }

    void SendStreamMetadata(const Stream& stream, uint32_t viewerCount) override
    {
        if (mockOnSendStreamMetadata)
        {
            mockOnSendStreamMetadata(stream, viewerCount);
        }
    }

    void SetOnConnectionClosed(std::function<void(void)> onConnectionClosed) override
    {
        this->onConnectionClosed = onConnectionClosed;
    }

    void SetOnIntro(connection_cb_intro_t onIntro) override
    {
        this->onIntro = onIntro;
    }

    void SetOnOutro(connection_cb_outro_t onOutro)
    {
        this->onOutro = onOutro;
    }

    void SetOnSubscribeChannel(connection_cb_subscribe_t onSubscribeChannel) override
    {
        this->onSubscribeChannel = onSubscribeChannel;
    }

    void SetOnUnsubscribeChannel(connection_cb_unsubscribe_t onUnsubscribeChannel) override
    {
        this->onUnsubscribeChannel = onUnsubscribeChannel;
    }

    void SetOnStreamAvailable(connection_cb_streamavailable_t onStreamAvailable) override
    {
        this->onStreamAvailable = onStreamAvailable;
    }

    void SetOnStreamRemoved(connection_cb_streamremoved_t onStreamRemoved) override
    {
        this->onStreamRemoved = onStreamRemoved;
    }

    void SetOnStreamMetadata(connection_cb_streammetadata_t onStreamMetadata) override
    {
        this->onStreamMetadata = onStreamMetadata;
    }

    std::string GetHostname() override
    {
        return hostname;
    }

private:
    std::function<void(void)> onConnectionClosed;
    connection_cb_intro_t onIntro;
    connection_cb_outro_t onOutro;
    connection_cb_subscribe_t onSubscribeChannel;
    connection_cb_unsubscribe_t onUnsubscribeChannel;
    connection_cb_streamavailable_t onStreamAvailable;
    connection_cb_streamremoved_t onStreamRemoved;
    connection_cb_streammetadata_t onStreamMetadata;
    std::string hostname;

    // Mock callbacks
    std::function<void(void)> onDestructed;
    std::function<void(Stream, uint32_t)> mockOnSendStreamMetadata;
    std::function<void(Stream)> mockOnSendStreamAvailable;
    std::function<void(Stream)> mockOnSendStreamRemoved;

    // Mock data
    std::vector<Stream> availableStreams;
};