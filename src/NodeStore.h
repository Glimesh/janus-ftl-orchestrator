/**
 * @file SubscriptionStore.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-11-13
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#pragma once

#include "FtlTypes.h"
#include "Signal.h"

#include <cstddef>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

struct Stream
{
    ftl_stream_id_t id;
    ftl_channel_id_t channel_id;
};

struct Subscription
{
    ftl_channel_id_t channel_id;
    std::vector<std::byte> stream_key;
};

struct Relay
{
    ftl_channel_id_t channel_id;
    std::string target_hostname;
    std::vector<std::byte> target_stream_key;
};

struct NodeStatus
{
    uint32_t current_load;
    uint32_t maximum_load;
};

class Node
{
public:
    void CreateStream(ftl_stream_id_t id, ftl_channel_id_t channel_id);
    void DeleteStream(ftl_stream_id_t id);

    void CreateSubscription(ftl_channel_id_t channel_id);
    void DeleteSubscription(ftl_channel_id_t channel_id);

    void SetStatus(NodeStatus status);

    Signal::Subscription SubscribeToRouteChanges();

private:
    std::mutex mutex;
    std::map<ftl_stream_id_t, Stream> streams;
    std::map<ftl_channel_id_t, Subscription> subscriptions;
    std::map<ftl_channel_id_t, Relay> relays;
    NodeStatus status;
    Signal routeChangeSignal;
};

class NodeStore
{
public:
    void CreateNode(std::string name);
    Node* GetNode(const std::string &name);
    std::weak_ptr<Node> GetNodeWeak(const std::string &name);
    void DeleteNode(std::string name);

private:
    std::unordered_map<std::string, Node> nodes;
};
