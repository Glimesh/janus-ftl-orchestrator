/**
 * @file MockConnection.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-10-25
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#pragma once

#include "../../IConnection.h"

#include <functional>
#include <string>

class MockConnection : public IConnection
{
public:
    // Mock constructor
    MockConnection(std::string hostname) : 
        hostname(hostname)
    { }

    // Mock utilities
    void MockFireOnConnectionClosed()
    {
        onConnectionClosed();
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

    void SetMockOnSendStreamAvailable(
        std::function<void(std::shared_ptr<Stream>)> mockOnSendStreamAvailable)
    {
        this->mockOnSendStreamAvailable = mockOnSendStreamAvailable;
    }

    void SetMockOnSendStreamRemoved(
        std::function<void(std::shared_ptr<Stream>)> mockOnSendStreamRemoved)
    {
        this->mockOnSendStreamRemoved = mockOnSendStreamRemoved;
    }

    // IConnection
    void Start() override
    { }

    void Stop() override
    { }

    void SendOutro(std::string message) override
    { }

    void SendStreamAvailable(std::shared_ptr<Stream> stream) override
    {
        if (mockOnSendStreamAvailable)
        {
            mockOnSendStreamAvailable(stream);
        }
    }

    void SendStreamRemoved(std::shared_ptr<Stream> stream) override
    {
        if (mockOnSendStreamRemoved)
        {
            mockOnSendStreamRemoved(stream);
        }
    }

    void SendStreamMetadata(std::shared_ptr<Stream> stream) override
    {

    }

    void SetOnConnectionClosed(std::function<void(void)> onConnectionClosed) override
    {
        this->onConnectionClosed = onConnectionClosed;
    }

    void SetOnIntro(std::function<void(uint8_t, uint8_t, uint8_t, std::string)> onIntro) override
    {
        
    }

    void SetOnOutro(std::function<void(std::string)> onOutro)
    {

    }

    void SetOnSubscribeChannel(std::function<void(ftl_channel_id_t)> onSubscribeChannel) override
    {

    }

    void SetOnUnsubscribeChannel(
        std::function<void(ftl_channel_id_t)> onUnsubscribeChannel) override
    {
        
    }

    void SetOnStreamAvailable(
        std::function<void(ftl_channel_id_t, ftl_stream_id_t, std::string)> onStreamAvailable) override
    {
        this->onStreamAvailable = onStreamAvailable;
    }

    void SetOnStreamRemoved(
        std::function<void(ftl_channel_id_t, ftl_stream_id_t)> onStreamRemoved) override
    {
        this->onStreamRemoved = onStreamRemoved;
    }

    void SetOnStreamMetadata(
        std::function<void(ftl_channel_id_t, ftl_stream_id_t, uint32_t)>
        onStreamMetadata) override
    {
        this->onStreamMetadata = onStreamMetadata;
    }

    std::string GetHostname() override
    {
        return hostname;
    }

private:
    std::function<void(void)> onConnectionClosed;
    std::function<void(uint8_t, uint8_t, uint8_t, std::string)> onIntro;
    std::function<void(ftl_channel_id_t)> onSubscribeChannel;
    std::function<void(ftl_channel_id_t)> onUnsubscribeChannel;
    std::function<void(ftl_channel_id_t, ftl_stream_id_t, std::string)> onStreamAvailable;
    std::function<void(ftl_channel_id_t, ftl_stream_id_t)> onStreamRemoved;
    std::function<void(ftl_channel_id_t, ftl_stream_id_t, uint32_t)> onStreamMetadata;
    std::string hostname;

    // Mock callbacks
    std::function<void(std::shared_ptr<Stream>)> mockOnSendStreamAvailable;
    std::function<void(std::shared_ptr<Stream>)> mockOnSendStreamRemoved;
};