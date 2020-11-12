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
    { 
        // TODO
    }

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
        // TODO
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
    std::function<void(std::shared_ptr<Stream>)> mockOnSendStreamAvailable;
    std::function<void(std::shared_ptr<Stream>)> mockOnSendStreamRemoved;
};