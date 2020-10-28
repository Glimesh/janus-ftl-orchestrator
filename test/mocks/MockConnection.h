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
    MockConnection(std::string hostname, FtlServerKind serverKind) : 
        hostname(hostname),
        serverKind(serverKind)
    { }

    // Mock utilities
    void MockFireOnConnectionClosed()
    {
        onConnectionClosed();
    }

    void MockFireOnIngestNewStream(ftl_channel_id_t channelId, ftl_stream_id_t streamId)
    {
        if (onIngestNewStream)
        {
            onIngestNewStream(channelId, streamId);
        }
    }

    void MockFireOnIngestStreamEnded(ftl_channel_id_t channelId, ftl_stream_id_t streamId)
    {
        if (onIngestStreamEnded)
        {
            onIngestStreamEnded(channelId, streamId);
        }
    }

    void SetMockOnSendStreamAvailable(
        std::function<void(std::shared_ptr<Stream>)> mockOnSendStreamAvailable)
    {
        this->mockOnSendStreamAvailable = mockOnSendStreamAvailable;
    }

    void SetMockOnSendStreamEnded(
        std::function<void(std::shared_ptr<Stream>)> mockOnSendStreamEnded)
    {
        this->mockOnSendStreamEnded = mockOnSendStreamEnded;
    }

    // IConnection
    void Start() override
    { }

    void Stop() override
    { }

    void SendStreamAvailable(std::shared_ptr<Stream> stream) override
    {
        if (mockOnSendStreamAvailable)
        {
            mockOnSendStreamAvailable(stream);
        }
    }

    void SendStreamEnded(std::shared_ptr<Stream> stream) override
    {
        if (mockOnSendStreamEnded)
        {
            mockOnSendStreamEnded(stream);
        }
    }

    void SetOnConnectionClosed(std::function<void(void)> onConnectionClosed) override
    {
        this->onConnectionClosed = onConnectionClosed;
    }

    void SetOnIngestNewStream(
        std::function<void(ftl_channel_id_t, ftl_stream_id_t)> onIngestNewStream) override
    {
        this->onIngestNewStream = onIngestNewStream;
    }

    void SetOnIngestStreamEnded(
        std::function<void(ftl_channel_id_t, ftl_stream_id_t)> onIngestStreamEnded) override
    {
        this->onIngestStreamEnded = onIngestStreamEnded;
    }

    void SetOnStreamViewersUpdated(
        std::function<void(ftl_channel_id_t, ftl_stream_id_t, uint32_t)>
        onStreamViewersUpdated) override
    {
        this->onStreamViewersUpdated = onStreamViewersUpdated;
    }

    std::string GetHostname() override
    {
        return hostname;
    }

    FtlServerKind GetServerKind() override
    {
        return serverKind;
    }

private:
    std::function<void(void)> onConnectionClosed;
    std::function<void(ftl_channel_id_t, ftl_stream_id_t)> onIngestNewStream;
    std::function<void(ftl_channel_id_t, ftl_stream_id_t)> onIngestStreamEnded;
    std::function<void(ftl_channel_id_t, ftl_stream_id_t, uint32_t)> onStreamViewersUpdated;
    std::string hostname;
    FtlServerKind serverKind;

    // Mock callbacks
    std::function<void(std::shared_ptr<Stream>)> mockOnSendStreamAvailable;
    std::function<void(std::shared_ptr<Stream>)> mockOnSendStreamEnded;
};