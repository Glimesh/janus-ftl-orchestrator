/**
 * @file IConnection.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-10-25
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#pragma once

#include <cstddef>
#include <functional>
#include <string>

/**
 * @brief
 *  ConnectionResult is a struct used to report information on the result of a request.
 *  Listeners such as Orchestrator use this to indicate to the IConnection whether a callback
 *  was successfully handled or not.
 * 
 */
struct ConnectionResult
{
    bool IsSuccess;
};

/* Connection Message Payload Types */
struct ConnectionIntroPayload
{
    uint8_t VersionMajor;
    uint8_t VersionMinor;
    uint8_t VersionRevision;
    uint8_t RelayLayer;
    std::string RegionCode;
    std::string Hostname;

    bool operator==(const ConnectionIntroPayload& c)
    {
        return (
            (VersionMajor == c.VersionMajor) &&
            (VersionMinor == c.VersionMinor) &&
            (VersionRevision == c.VersionRevision) &&
            (RelayLayer == c.RelayLayer) &&
            (RegionCode == c.RegionCode) && 
            (Hostname == c.Hostname));
    }
};

struct ConnectionOutroPayload
{
    std::string DisconnectReason;
};

struct ConnectionNodeStatePayload
{
    uint32_t CurrentLoad;
    uint32_t MaximumLoad;
};

struct ConnectionSubscriptionPayload
{
    bool IsSubscribe;
    uint32_t ChannelId;
    std::vector<std::byte> StreamKey;
};

struct ConnectionPublishPayload
{
    bool IsPublish;
    uint32_t ChannelId;
    uint32_t StreamId;
};

struct ConnectionRelayPayload
{
    bool IsStartRelay;
    uint32_t ChannelId;
    uint32_t StreamId;
    std::string TargetHostname;
    std::vector<std::byte> StreamKey;
};

/* Callback types */
typedef 
    std::function<ConnectionResult(ConnectionIntroPayload)>
    connection_cb_intro_t;
typedef
    std::function<ConnectionResult(ConnectionOutroPayload)>
    connection_cb_outro_t;
typedef
    std::function<ConnectionResult(ConnectionNodeStatePayload)>
    connection_cb_nodestate_t;
typedef
    std::function<ConnectionResult(ConnectionSubscriptionPayload)>
    connection_cb_subscription_t;
typedef
    std::function<ConnectionResult(ConnectionPublishPayload)>
    connection_cb_publishing_t;
typedef
    std::function<ConnectionResult(ConnectionRelayPayload)>
    connection_cb_relay_t;

/**
 * @brief
 *  IConnection represents a high-level connection to an FTL instance over an IConnectionTransport.
 *  Incoming binary data from IConnectionTransport is translated to discrete methods and events
 *  by the IConnection implementation.
 */
class IConnection
{
public:
    virtual ~IConnection() = default;

    /**
     * @brief Starts the connection in a new thread
     */
    virtual void Start() = 0;

    /**
     * @brief Shuts down connection to client and terminates connection thread
     */
    virtual void Stop() = 0;

    /**
     * @brief Sends an intro message to this connection with metadata on the connection
     * @param payload Payload containing intro details
     */
    virtual void SendIntro(const ConnectionIntroPayload& payload) = 0;

    /**
     * @brief Sends an outro message to this connection indicating reason for disconnect
     * @param payload Payload containing outro details
     */
    virtual void SendOutro(const ConnectionOutroPayload& payload) = 0;

    /**
     * @brief Sends a message with the state of the node (estimated load, etc.)
     * @param payload Payload containing node state details
     */
    virtual void SendNodeState(const ConnectionNodeStatePayload& payload) = 0;

    /**
     * @brief
     *  Sends a channel subscription message, used to (un)subscribe to updates on streams for
     *  a particular channel
     * @param payload details on channel subscription being requested
     */
    virtual void SendChannelSubscription(const ConnectionSubscriptionPayload& payload) = 0;

    /**
     * @brief
     *  Sends a stream publishing message to this connection, used to indicate availability
     *  of a new stream.
     * @param payload payload indicating details of the publish message
     */
    virtual void SendStreamPublish(const ConnectionPublishPayload& payload) = 0;

    /**
     * @brief
     *  Sends a stream relay message, indicating that a stream should be forwarded to another node
     *  (or that a relay should be stopped).
     * @param payload payload with details on the stream relay
     */
    virtual void SendStreamRelay(const ConnectionRelayPayload& payload) = 0;

    /**
     * @brief
     *  Sets the callback that will fire when this connection has been closed.
     * 
     * @param onConnectionClosed callback to fire on connection close
     */
    virtual void SetOnConnectionClosed(std::function<void(void)> onConnectionClosed) = 0;

    /**
     * @brief Sets the callback that will fire when this connection receives an intro request.
     * 
     * @param onIntro callback to fire on intro
     */
    virtual void SetOnIntro(connection_cb_intro_t onIntro) = 0;

    /**
     * @brief Sets the callback that will fire when this connection receives an outro request.
     * 
     * @param onIntro callback to fire on outro
     */
    virtual void SetOnOutro(connection_cb_outro_t onOutro) = 0;

    /**
     * @brief
     *  Sets the callback that will fire when this connection has received a node state message.
     */
    virtual void SetOnNodeState(connection_cb_nodestate_t onNodeState) = 0;

    /**
     * @brief
     *  Sets the callback that will fire when this connection has received a subscription message
     */
    virtual void SetOnChannelSubscription(connection_cb_subscription_t onChannelSubscription) = 0;

    /**
     * @brief
     *  Sets the callback that will fire when this connection has received a stream publishing
     *  message, indicating availability (or removal) of a stream
     */
    virtual void SetOnStreamPublish(connection_cb_publishing_t onStreamPublish) = 0;

    /**
     * @brief
     *  Sets the callback that will fire when this connection has received a stream relay
     *  message, indicating that a stream should be forwarded to another node (or that a relay
     *  should be stopped)
     */
    virtual void SetOnStreamRelay(connection_cb_relay_t onStreamRelay) = 0;

    /**
     * @brief Retrieve the hostname of the FTL server represented by this connection
     */
    virtual std::string GetHostname() = 0;

    /**
     * @brief Set the hostname of the FTL node represented by this connection
     */
    virtual void SetHostname(std::string hostname) = 0;
};