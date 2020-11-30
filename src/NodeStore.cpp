#include "NodeStore.h"

void Node::CreateStream(ftl_stream_id_t id, ftl_channel_id_t channel_id)
{
    std::lock_guard<std::mutex> lock(mutex);
    routeChangeSignal.Notify();
}

void Node::DeleteStream(ftl_stream_id_t id)
{
    std::lock_guard<std::mutex> lock(mutex);
    routeChangeSignal.Notify();
}

void Node::CreateSubscription(ftl_channel_id_t channel_id)
{
    std::lock_guard<std::mutex> lock(mutex);
}
void Node::DeleteSubscription(ftl_channel_id_t channel_id)
{
    std::lock_guard<std::mutex> lock(mutex);
}

void Node::SetStatus(NodeStatus status) {}

Signal::Subscription Node::SubscribeToRouteChanges()
{
    return routeChangeSignal.Subscribe();
}

void NodeStore::CreateNode(const std::string &name)
{
    std::lock_guard<std::mutex> lock(mutex);
    if (nodes.count(name) > 0)
    {
        return;
    }

    nodes[name] = std::make_unique<Node>(name);
}

Node *NodeStore::GetNode(const std::string &name)
{
    std::lock_guard<std::mutex> lock(mutex);
    auto node = nodes.find(name);
    if (node != nodes.end())
    {
        return node->second.get();
    }

    return nullptr;
}

void NodeStore::DeleteNode(std::string name) {}
