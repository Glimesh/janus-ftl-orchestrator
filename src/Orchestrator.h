/**
 * @file Orchestrator.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-10-18
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#pragma once

#include "FtlTypes.h"

#include "IConnection.h"
#include "IConnectionManager.h"
#include "StreamStore.h"
#include "SubscriptionStore.h"

#include <arpa/inet.h>
#include <map>
#include <memory>
#include <mutex>
#include <set>

// Forward declarations
class Configuration;

/**
 * @brief
 *  Orchestrator handles listening for and maintaining incoming
 *  orchestration connections
 */
template <class TConnection>
class Orchestrator
{
public:
    /* Constructor/Destructor */
    Orchestrator(std::shared_ptr<IConnectionManager<TConnection>> connectionManager);

    /* Public methods */
    /**
     * @brief Performs initialization steps
     */
    void Init();

    /**
     * @brief Get connections currently registered with this orchestrator
     * 
     * @return const std::set<std::shared_ptr<IConnection>> set of connections
     */
    const std::set<std::shared_ptr<TConnection>> GetConnections();

private:
    /* Private members */
    const std::shared_ptr<IConnectionManager<TConnection>> connectionManager;
    StreamStore streamStore;
    std::mutex connectionsMutex;
    std::set<std::shared_ptr<TConnection>> pendingConnections;
    std::set<std::shared_ptr<TConnection>> connections;
    std::mutex streamsMutex;
    SubscriptionStore subscriptions;

    /* Private methods */
    /* ConnectionManager callback handlers */
    void newConnection(std::shared_ptr<TConnection> connection);
    /* Connection callback handlers */
    void connectionClosed(std::weak_ptr<TConnection> connection);
    ConnectionResult connectionIntro(
        std::weak_ptr<TConnection> connection,
        ConnectionIntroPayload payload);
    ConnectionResult connectionOutro(
        std::weak_ptr<TConnection> connection,
        ConnectionOutroPayload payload);
    ConnectionResult connectionNodeState(
        std::weak_ptr<TConnection> connection,
        ConnectionNodeStatePayload payload);
    ConnectionResult connectionChannelSubscription(
        std::weak_ptr<TConnection> connection,
        ConnectionSubscriptionPayload payload);
    ConnectionResult connectionStreamPublish(
        std::weak_ptr<TConnection> connection,
        ConnectionPublishPayload payload);
    ConnectionResult connectionStreamRelay(
        std::weak_ptr<TConnection> connection,
        ConnectionRelayPayload payload);
    
};