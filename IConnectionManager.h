/**
 * @file IConnectionManager.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-10-25
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#pragma once

#include "IConnection.h"

#include <functional>
#include <memory>

/**
 * @brief Describes a class that accepts new IConnections
 */
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
     */
    virtual void Listen() = 0;

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
        std::function<void(std::shared_ptr<IConnection>)> onNewConnection) = 0;
};