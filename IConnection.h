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
    virtual void SendStreamEnded(std::shared_ptr<Stream> stream) = 0;

    /**
     * @brief
     *  Sets the callback that will fire when this connection has been closed.
     * 
     * @param onConnectionClosed callback to fire on connection close
     */
    virtual void SetOnConnectionClosed(std::function<void(void)> onConnectionClosed) = 0;

    /**
     * @brief
     *  Sets the callback that will fire when this connection has indicated that a new stream
     *  ingest has been started.
     * 
     * @param onIngestNewStream callback to fire on new stream ingested
     */
    virtual void SetOnIngestNewStream(
        std::function<void(ftl_channel_id_t, ftl_stream_id_t)> onIngestNewStream) = 0;

    /**
     * @brief
     *  Sets the callback that will fire when this connection has indicated that an existing
     *  stream ingest has ended.
     * 
     * @param onIngestStreamEnded callback to fire on existing stream ended
     */
    virtual void SetOnIngestStreamEnded(
        std::function<void(ftl_channel_id_t, ftl_stream_id_t)> onIngestStreamEnded) = 0;

    /**
     * @brief 
     *  Sets the callback that will fire when this connection indicates how many viewers
     *  it is serving this stream to.
     * 
     * @param onStreamViewersUpdated callback to fire when the streamviewer count is updated
     */
    virtual void SetOnStreamViewersUpdated(
        std::function<void(ftl_channel_id_t, ftl_stream_id_t, uint32_t)> onStreamViewersUpdated) = 0;

    /**
     * @brief Retrieve the hostname of the FTL server represented by this connection
     * 
     * @return std::string the hostname
     */
    virtual std::string GetHostname() = 0;
};