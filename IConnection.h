/**
 * @file IConnection.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-10-25
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#pragma once

#include "Stream.h"

#include <functional>
#include <string>

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
     * @brief Sends an outro message to this connection indicating reason for disconnect
     * 
     * @param message Message explaining disconnect reason
     */
    virtual void SendOutro(std::string message) = 0;

    /**
     * @brief Sends a message to this connection indicating that a new stream is available
     * 
     * @param stream the stream that has been made available
     */
    virtual void SendStreamAvailable(std::shared_ptr<Stream> stream) = 0;

    /**
     * @brief Sends a message to this connection indicating that a stream has ended
     * 
     * @param stream the stream that has ended
     */
    virtual void SendStreamRemoved(std::shared_ptr<Stream> stream) = 0;

    /**
     * @brief Sends a stream metadata update to this connection
     * 
     * @param stream stream to send metadata for
     */
    virtual void SendStreamMetadata(std::shared_ptr<Stream> stream) = 0;

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
    virtual void SetOnIntro(
        std::function<void(uint8_t, uint8_t, uint8_t, std::string)> onIntro) = 0;

    /**
     * @brief Sets the callback that will fire when this connection receives an outro request.
     * 
     * @param onIntro callback to fire on outro
     */
    virtual void SetOnOutro(std::function<void(std::string)> onOutro) = 0;

    /**
     * @brief
     *  Sets the callback that will fire when this connection has requested a subscription
     *  to updates for a given channel ID
     * 
     * @param onSubscribeChannel callback to fire on new channel subscription request
     */
    virtual void SetOnSubscribeChannel(
        std::function<void(ftl_channel_id_t)> onSubscribeChannel) = 0;

    /**
     * @brief
     *  Sets the callback that will fire when this connection has requested to unsubscribe
     *  from updates for a given channel ID
     * 
     * @param onUnsubscribeChannel callback to fire on channel unsubscribe request
     */
    virtual void SetOnUnsubscribeChannel(
        std::function<void(ftl_channel_id_t)> onUnsubscribeChannel) = 0;

    /**
     * @brief
     *  Sets the callback that will fire when this connection has indicated that a new stream
     *  ingest has been started.
     * 
     * @param onStreamAvailable callback to fire on new stream ingested
     */
    virtual void SetOnStreamAvailable(
        std::function<void(ftl_channel_id_t, ftl_stream_id_t, std::string)> onStreamAvailable) = 0;

    /**
     * @brief
     *  Sets the callback that will fire when this connection has indicated that an existing
     *  stream ingest has ended.
     * 
     * @param onStreamRemoved callback to fire on existing stream ended
     */
    virtual void SetOnStreamRemoved(
        std::function<void(ftl_channel_id_t, ftl_stream_id_t)> onStreamRemoved) = 0;

    /**
     * @brief 
     *  Sets the callback that will fire when this connection provides metadata for a stream
     * 
     * @param onStreamViewersUpdated callback to fire when new metadata is sent
     */
    virtual void SetOnStreamMetadata(
        std::function<void(ftl_channel_id_t, ftl_stream_id_t, uint32_t)> onStreamMetadata) = 0;

    /**
     * @brief Retrieve the hostname of the FTL server represented by this connection
     * 
     * @return std::string the hostname
     */
    virtual std::string GetHostname() = 0;
};