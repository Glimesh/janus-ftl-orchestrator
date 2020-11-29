/**
 * @file SubscriptionStore.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-11-13
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#pragma once

#include "ChannelSubscription.h"
#include "FtlTypes.h"

#include <cstddef>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <vector>
#include <string>
#include <optional>

class Stream
{
    ftl_stream_id_t id;
    ftl_channel_id_t channel_id;
};

class Subscription
{
    ftl_channel_id_t channel_id;
    std::vector<std::byte> stream_key;
};

class Relay
{
    ftl_channel_id_t channel_id;
    std::string target_hostname;
    std::vector<std::byte> target_stream_key;
};

class NodeStatus
{
    uint32_t current_load;
    uint32_t maximum_load;
};

class Node
{
public:
    void CreateStream(ftl_stream_id_t id, ftl_channel_id_t channel_id);
    void DeleteStream(ftl_stream_id_t id);

    void CreateSubscription(ftl_stream_id_t id, ftl_channel_id_t channel_id);
    void DeleteSubscription(ftl_stream_id_t id);

    void SetStatus(NodeStatus status);

private:
    std::mutex mutex;
    std::map<ftl_stream_id_t, Stream> streams;
    std::map<ftl_channel_id_t, Subscription> subscriptions;
    std::map<ftl_channel_id_t, Relay> relays;
    NodeStatus status;
};

class NodeStore
{
public:
    void CreateNode(std::string name);
    Node* GetNode(const std::string& name);
    void DeleteNode(std::string name);

private:
    std::mutex mutex;
    std::map<std::string, Node> nodes;
};