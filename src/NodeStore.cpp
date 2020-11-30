#pragma once

#include "NodeStore.h"

void Node::CreateStream(ftl_stream_id_t id, ftl_channel_id_t channel_id){}
void Node::DeleteStream(ftl_stream_id_t id){}

void Node::CreateSubscription(ftl_channel_id_t channel_id){}
void Node::DeleteSubscription(ftl_channel_id_t channel_id){}

void Node::SetStatus(NodeStatus status){}

Signal::Subscription Node::SubscribeToRouteChanges(){}

void NodeStore::CreateNode(std::string name{}
Node *NodeStore::GetNode(const std::string &name){}
std::weak_ptr<Node> NodeStore::GetNodeWeak(const std::string &name){}
void NodeStore::DeleteNode(std::string name){}
