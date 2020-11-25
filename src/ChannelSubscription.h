/**
 * @file ChannelSubscription.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-11-23
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#pragma once

#include "FtlTypes.h"
#include "IConnection.h"

#include <memory>
#include <vector>

/**
 * @brief Represents a subscription a node holds to a particular channel
 */
template <class TConnection>
struct ChannelSubscription
{
    std::shared_ptr<TConnection> SubscribedConnection;
    ftl_channel_id_t ChannelId;
    std::vector<std::byte> StreamKey;
};