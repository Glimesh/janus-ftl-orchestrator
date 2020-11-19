/**
 * @file SubscriptionStore.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-11-13
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#pragma once

#include "FtlTypes.h"
#include "IConnection.h"

#include <map>
#include <memory>
#include <mutex>
#include <set>

/**
 * @brief
 *  SubscriptionStore manages subscriptions made by connections to specific channels for
 *  streaming alerts.
 * 
 */
class SubscriptionStore
{
public:
    /* Public methods */
    /**
     * @brief Adds a subscription for the given connection on the given channel
     * @param connection connection to add subscription for
     * @param channelId channel ID to add subscription for
     * @return bool true if the subscription was successfully added
     */
    bool AddSubscription(std::shared_ptr<IConnection> connection, ftl_channel_id_t channelId);

    /**
     * @brief Removes an existing subscription for the given connection and channel
     * @param connection connection to remove subscription for
     * @param channelId channel ID to remove subscription for
     * @return bool true if the subscription was successfully found and removed
     */
    bool RemoveSubscription(std::shared_ptr<IConnection> connection, ftl_channel_id_t channelId);

    /**
     * @brief Get the list of channel subscriptions that exist for a connection
     * @param connection connection to fetch subscribed channels for
     * @return std::set<ftl_channel_id_t> set of channels this connection is subscribed to
     */
    std::set<ftl_channel_id_t> GetSubscribedChannels(std::shared_ptr<IConnection> connection);

    /**
     * @brief Get the list of connections that are subscribed to a given channel
     * @param channelId channel to fetch connection subscriptions for
     * @return std::set<std::shared_ptr<IConnection>> list of connections subscribed
     */
    std::set<std::shared_ptr<IConnection>> GetSubscribedConnections(ftl_channel_id_t channelId);

    /**
     * @brief Clears all subscriptions for the given connection
     * @param connection connection to remove all subscriptions of
     */
    void ClearSubscriptions(std::shared_ptr<IConnection> connection);

private:
    std::mutex subscriptionsStoreMutex;
    std::map<std::shared_ptr<IConnection>, std::set<ftl_channel_id_t>> subscriptionsByConnection;
    std::map<ftl_channel_id_t, std::set<std::shared_ptr<IConnection>>> subscriptionsByChannel;
};