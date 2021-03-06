/**
 * @file IConnectionManager.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-10-25
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#pragma once

#include "IConnectionTransport.h"

#include <functional>
#include <future>
#include <memory>
#include <optional>

/**
 * @brief Describes a class that accepts new IConnections
 */
template <class TConnection>
class IConnectionManager
{
public:
    virtual ~IConnectionManager() = default;

    /**
     * @brief Performs any needed initialization before listening for new connections
     */
    virtual void Init() = 0;

    /**
     * @brief Starts listening for incoming connections, blocking the current thread
     * @param readyPromise a promise that is fulfilled as soon as the service is actively listening
     */
    virtual void Listen(std::promise<void>&& readyPromise = std::promise<void>()) = 0;

    /**
     * @brief Stops listening for incoming connections
     */
    virtual void StopListening() = 0;

    /**
     * @brief
     *  Set the callback that will be fired when a new connection has been established
     *  and has received initial identification information.
     *  Note that these events may come in on a new thread, so take care to synchronize
     *  any updates that occur as a result.
     * 
     * @param onNewConnection callback to fire
     */
    virtual void SetOnNewConnection(
        std::function<void(std::shared_ptr<TConnection>)> onNewConnection) = 0;
};