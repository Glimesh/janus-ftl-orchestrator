/**
 * @file IConnectionTransport.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-11-08
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#pragma once

#include <cstdint>
#include <functional>
#include <vector>

/**
 * @brief
 *  IConnectionTransport represents a generic network transport between the orchestrator
 *  and FTL clients, allowing bytes to be read from and written to.
 */
class IConnectionTransport
{
public:
    virtual ~IConnectionTransport() = default;

    /**
     * @brief Starts the connection transport in a new thread
     */
    virtual void StartAsync() = 0;

    /**
     * @brief Shuts down connection
     */
    virtual void Stop() = 0;

    /**
     * @brief Write bytes to the transport
     * @param bytes bytes to be written
     */
    virtual void Write(const std::vector<std::byte>& bytes) = 0;

    /**
     * @brief Sets the callback that will fire when this connection has been closed.
     * @param onConnectionClosed callback to fire on connection close
     */
    virtual void SetOnConnectionClosed(std::function<void(void)> onConnectionClosed) = 0;

    /**
     * @brief Set the callback that will fire when this connection has received bytes
     * @param onBytesReceived callback to fire when bytes received
     */
    virtual void SetOnBytesReceived(
        std::function<void(const std::vector<std::byte>&)> onBytesReceived) = 0;
};