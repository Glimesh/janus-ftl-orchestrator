/**
 * @file SubscriptionStore.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-11-13
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#pragma once

#include "ChannelSubscription.h"
#include "FtlTypes.h"

#include <spdlog/spdlog.h>

#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <vector>

/**
 * @brief
 *  SubscriptionStore manages subscriptions made by connections to specific channels for
 *  streaming alerts.
 * 
 */
template <class TConnection>
class SubscriptionStore
{
public:
    /* Public methods */
    /**
     * @brief Adds a subscription for the given connection on the given channel
     * @param connection connection to add subscription for
     * @param channelId channel ID to add subscription for
     * @param streamKey stream key used to relay streams to the subscriber node
     * @return bool true if the subscription was successfully added
     */
    bool AddSubscription(
        std::shared_ptr<TConnection> connection,
        ftl_channel_id_t channelId,
        std::vector<std::byte> streamKey)
    {
        std::lock_guard<std::mutex> lock(subscriptionsStoreMutex);
        std::shared_ptr<ChannelSubscription<TConnection>> subscription = 
            std::make_shared<ChannelSubscription<TConnection>>(
                connection, // SubscribedConnection
                channelId,  // ChannelId
                streamKey); // StreamKey
        if (subscriptionsByConnection.count(connection) <= 0)
        {
            subscriptionsByConnection.emplace(
                std::pair(
                    connection,
                    std::set<std::shared_ptr<ChannelSubscription<TConnection>>>()));
        }
        if (subscriptionsByChannel.count(channelId) <= 0)
        {
            subscriptionsByChannel.emplace(
                std::pair(
                    channelId,
                    std::set<std::shared_ptr<ChannelSubscription<TConnection>>>()));
        }
        subscriptionsByConnection[connection].insert(subscription);
        subscriptionsByChannel[channelId].insert(subscription);
        return true;
    }

    /**
     * @brief Removes an existing subscription for the given connection and channel
     * @param connection connection to remove subscription for
     * @param channelId channel ID to remove subscription for
     * @return bool true if the subscription was successfully found and removed
     */
    bool RemoveSubscription(std::shared_ptr<TConnection> connection, ftl_channel_id_t channelId)
    {
        std::lock_guard<std::mutex> lock(subscriptionsStoreMutex);
        bool success = true;
        if (subscriptionsByConnection.count(connection) <= 0)
        {
            spdlog::error(
                "Attempt to remove subscription for connection {} / channel {}, "
                "but no subscriptions for this connection exist",
                connection->GetHostname(),
                channelId);
            success = false;
        }
        else
        {
            auto& subs = subscriptionsByConnection[connection];
            int numSubsErased = 0;
            for (auto it = subs.begin(); it != subs.end();)
            {
                auto& sub = *it;
                if (sub->ChannelId == channelId)
                {
                    it = subs.erase(it);
                    ++numSubsErased;
                }
                else
                {
                    ++it;
                }
            }
            if (numSubsErased == 0)
            {
                spdlog::error(
                    "Attempt to remove non-existant subscription for connection {} to channel {}",
                    connection->GetHostname(),
                    channelId);
                success = false;
            }
        }

        if (subscriptionsByChannel.count(channelId) <= 0)
        {
            spdlog::error(
                "Attempt to remove subscription for connection {} / channel {}, "
                "but no subscriptions to this channel exist",
                connection->GetHostname(),
                channelId);
            success = false;
        }
        else
        {
            auto& subs = subscriptionsByChannel[channelId];
            int numSubsErased = 0;
            for (auto it = subs.begin(); it != subs.end();)
            {
                auto& sub = *it;
                if (sub->SubscribedConnection == connection)
                {
                    it = subs.erase(it);
                    numSubsErased++;
                }
                else
                {
                    ++it;
                }
            }

            if (numSubsErased == 0)
            {
                spdlog::error(
                    "Attempt to remove non-existant subscription on channel {} for connection {}",
                    connection->GetHostname(),
                    channelId);
                success = false;
            }
        }
        return success;
    }

    /**
     * @brief Get the list of channel subscriptions that exist for a connection
     * @param connection connection to fetch subscribed channels for
     * @return std::set<ftl_channel_id_t> set of channels this connection is subscribed to
     */
    std::vector<ChannelSubscription<TConnection>> GetSubscriptions(std::shared_ptr<TConnection> connection)
    {
        std::vector<ChannelSubscription<TConnection>> returnVal;
        std::lock_guard<std::mutex> lock(subscriptionsStoreMutex);
        if (subscriptionsByConnection.contains(connection))
        {
            for (const auto& subscription : subscriptionsByConnection[connection])
            {
                returnVal.push_back(*subscription);
            }
        }
        return returnVal;
    }

    /**
     * @brief Get the list of subscriptions to a given channel
     * @param channelId channel to fetch subscriptions for
     * @return std::vector<ChannelSubscription> list of subscriptions for the given channel
     */
    std::vector<ChannelSubscription<TConnection>> GetSubscriptions(ftl_channel_id_t channelId)
    {
        std::vector<ChannelSubscription<TConnection>> returnVal;
        std::lock_guard<std::mutex> lock(subscriptionsStoreMutex);
        if (subscriptionsByChannel.contains(channelId))
        {
            for (const auto& subscription : subscriptionsByChannel[channelId])
            {
                returnVal.push_back(*subscription);
            }
        }
        return returnVal;
    }

    /**
     * @brief Clears all subscriptions for the given connection
     * @param connection connection to remove all subscriptions of
     */
    void ClearSubscriptions(std::shared_ptr<TConnection> connection)
    {
        std::lock_guard<std::mutex> lock(subscriptionsStoreMutex);
        if (subscriptionsByConnection.count(connection) > 0)
        {
            for (const auto& subscription : subscriptionsByConnection[connection])
            {
                if (subscriptionsByChannel.count(subscription->ChannelId) <= 0)
                {
                    throw std::runtime_error(
                        "Subscription Store inconsistency - can not find matching channel for "
                        "connection subscription.");
                }
                subscriptionsByChannel[subscription->ChannelId].erase(subscription);
                if (subscriptionsByChannel[subscription->ChannelId].empty())
                {
                    subscriptionsByChannel.erase(subscription->ChannelId);
                }
            }

            subscriptionsByConnection.erase(connection);
        }
    }

    /**
     * @brief Clears all records
     */
    void Clear()
    {
        std::lock_guard<std::mutex> lock(subscriptionsStoreMutex);
        subscriptionsByConnection.clear();
        subscriptionsByChannel.clear();
    }

private:
    std::mutex subscriptionsStoreMutex;
    std::map<std::shared_ptr<TConnection>, std::set<std::shared_ptr<ChannelSubscription<TConnection>>>>
        subscriptionsByConnection;
    std::map<ftl_channel_id_t, std::set<std::shared_ptr<ChannelSubscription<TConnection>>>>
        subscriptionsByChannel;
};