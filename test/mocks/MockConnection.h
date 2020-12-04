/**
 * @file MockConnection.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-10-25
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#pragma once

#include <FtlTypes.h>
#include <IConnection.h>

#include "../../src/Stream.h" // TODO: Replace with generic structure

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

    void MockFireOnIntro(ConnectionIntroPayload payload)
    {
        if (onIntro)
        {
            onIntro(payload);
        }
    }

    void MockFireOnOutro(ConnectionOutroPayload payload)
    {
        if (onOutro)
        {
            onOutro(payload);
        }
    }

    void MockFireOnNodeState(ConnectionNodeStatePayload payload)
    {
        if (onNodeState)
        {
            onNodeState(payload);
        }
    }

    void MockFireOnChannelSubscription(ConnectionSubscriptionPayload payload)
    {
        if (onChannelSubscription)
        {
            onChannelSubscription(payload);
        }
    }

    void MockFireOnStreamPublish(ConnectionPublishPayload payload)
    {
        if (onStreamPublish)
        {
            onStreamPublish(payload);
        }
    }

    void MockFireOnStreamRelay(ConnectionRelayPayload payload)
    {
        if (onStreamRelay)
        {
            onStreamRelay(payload);
        }
    }

    void SetMockOnDestructed(std::function<void(void)> onDestructed)
    {
        this->onDestructed = onDestructed;
    }

    void SetMockOnSendStreamPublish(
        std::function<void(ConnectionPublishPayload)> mockOnSendStreamPublish)
    {
        this->mockOnSendStreamPublish = mockOnSendStreamPublish;
    }

    // IConnection
    void Start() override
    { }

    void Stop() override
    { }

    void SendIntro(const ConnectionIntroPayload& payload) override
    {
        if (onIntro)
        {
            onIntro(payload);
        }
    }

    void SendOutro(const ConnectionOutroPayload& payload) override
    { 
        if (onOutro)
        {
            onOutro(payload);
        }
    }

    void SendNodeState(const ConnectionNodeStatePayload& payload) override
    {
        if (onNodeState)
        {
            onNodeState(payload);
        }
    }

    void SendChannelSubscription(const ConnectionSubscriptionPayload& payload) override
    {
        if (onChannelSubscription)
        {
            onChannelSubscription(payload);
        }
    }

    void SendStreamPublish(const ConnectionPublishPayload& payload) override
    {
        if (payload.IsPublish)
        {
            Stream<MockConnection> newStream
            {
                .ChannelId = payload.ChannelId,
                .StreamId = payload.StreamId,
            };
            availableStreams.push_back(newStream);
        }
        else
        {
            availableStreams.erase(
            std::remove_if(
                availableStreams.begin(),
                availableStreams.end(),
                [&payload](auto availableStream)
                {
                    return (
                        (availableStream.ChannelId == payload.ChannelId) && 
                        (availableStream.StreamId == payload.StreamId));
                }),
            availableStreams.end());
        }

        if (mockOnSendStreamPublish)
        {
            mockOnSendStreamPublish(payload);
        }
    }

    void SendStreamRelay(const ConnectionRelayPayload& payload) override
    {
        if (onStreamRelay)
        {
            onStreamRelay(payload);
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

    void SetHostname(std::string hostname) override
    {
        this->hostname = hostname;
    }

private:
    std::function<void(void)> onConnectionClosed;
    connection_cb_intro_t onIntro;
    connection_cb_outro_t onOutro;
    connection_cb_nodestate_t onNodeState;
    connection_cb_subscription_t onChannelSubscription;
    connection_cb_publishing_t onStreamPublish;
    connection_cb_relay_t onStreamRelay;
    std::string hostname;

    // Mock callbacks
    std::function<void(void)> onDestructed;
    std::function<void(ConnectionPublishPayload)> mockOnSendStreamPublish;

    // Mock data
    std::vector<Stream<MockConnection>> availableStreams;
};