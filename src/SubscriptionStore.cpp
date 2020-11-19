/**
 * @file SubscriptionStore.cpp
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-11-13
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#include "SubscriptionStore.h"

#include <spdlog/spdlog.h>

#pragma region Public methods
bool SubscriptionStore::AddSubscription(
    std::shared_ptr<IConnection> connection,
    ftl_channel_id_t channelId)
{
    std::lock_guard<std::mutex> lock(subscriptionsStoreMutex);
    if (subscriptionsByConnection.count(connection) <= 0)
    {
        subscriptionsByConnection.emplace(
            std::pair(connection, std::set<ftl_channel_id_t>()));
    }
    if (subscriptionsByChannel.count(channelId) <= 0)
    {
        subscriptionsByChannel.emplace(
            std::pair(channelId, std::set<std::shared_ptr<IConnection>>()));
    }
    subscriptionsByConnection[connection].insert(channelId);
    subscriptionsByChannel[channelId].insert(connection);
    return true;
}

bool SubscriptionStore::RemoveSubscription(
    std::shared_ptr<IConnection> connection,
    ftl_channel_id_t channelId)
{
    std::lock_guard<std::mutex> lock(subscriptionsStoreMutex);
    bool success = true;
    if (subscriptionsByConnection.count(connection) <= 0)
    {
        spdlog::error(
            "Attempt to remove subscription for connection {}, "
            "but no subscriptions for this connection exist",
            connection->GetHostname(),
            channelId);
        success = false;
    }
    else
    {
        if (subscriptionsByConnection[connection].count(channelId) <= 0)
        {
            spdlog::error(
                "Attempt to remove non-existant subscription for connection {} to channel {}",
                connection->GetHostname(),
                channelId);
            success = false;
        }
        else
        {
            subscriptionsByConnection[connection].erase(channelId);
            if (subscriptionsByConnection[connection].empty())
            {
                subscriptionsByConnection.erase(connection);
            }
        }
    }

    if (subscriptionsByChannel.count(channelId) <= 0)
    {
        spdlog::error(
            "Attempt to remove subscription for channel {}, "
            "but no subscriptions to this channel exist",
            channelId);
        success = false;
    }
    else
    {
        if (subscriptionsByChannel[channelId].count(connection) <= 0)
        {
            spdlog::error(
                "Attempt to remove non-existant subscription on channel {} for connection {}",
                connection->GetHostname(),
                channelId);
            success = false;
        }
        else
        {
            subscriptionsByChannel[channelId].erase(connection);
            if (subscriptionsByChannel[channelId].empty())
            {
                subscriptionsByChannel.erase(channelId);
            }
        }
    }
    return success;
}

std::set<ftl_channel_id_t> SubscriptionStore::GetSubscribedChannels(
    std::shared_ptr<IConnection> connection)
{
    std::lock_guard<std::mutex> lock(subscriptionsStoreMutex);
    if (subscriptionsByConnection.count(connection) > 0)
    {
        return subscriptionsByConnection[connection];
    }
    return std::set<ftl_channel_id_t>();
}

std::set<std::shared_ptr<IConnection>> SubscriptionStore::GetSubscribedConnections(
    ftl_channel_id_t channelId)
{
    std::lock_guard<std::mutex> lock(subscriptionsStoreMutex);
    if (subscriptionsByChannel.count(channelId) > 0)
    {
        return subscriptionsByChannel[channelId];
    }
    return std::set<std::shared_ptr<IConnection>>();
}

void SubscriptionStore::ClearSubscriptions(std::shared_ptr<IConnection> connection)
{
    std::lock_guard<std::mutex> lock(subscriptionsStoreMutex);
    if (subscriptionsByConnection.count(connection) > 0)
    {
        for (const auto& channel : subscriptionsByConnection[connection])
        {
            if (subscriptionsByChannel.count(channel) <= 0)
            {
                throw std::runtime_error(
                    "Subscription Store inconsistency - can not find matching channel for "
                    "connection subscription.");
            }
            subscriptionsByChannel[channel].erase(connection);
            if (subscriptionsByChannel[channel].empty())
            {
                subscriptionsByChannel.erase(channel);
            }
        }

        subscriptionsByConnection.erase(connection);
    }
}
#pragma endregion /Public methods