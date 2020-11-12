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
    std::unique_ptr<StreamStore> streamStore;
    std::mutex connectionsMutex;
    std::set<std::shared_ptr<TConnection>> connections;
    std::mutex streamsMutex;
    std::map<ftl_channel_id_t, std::shared_ptr<TConnection>> channelSubscriptions;

    /* Private methods */
    /* ConnectionManager callback handlers */
    void newConnection(std::shared_ptr<TConnection> connection);
    /* Connection callback handlers */
    void connectionClosed(std::weak_ptr<TConnection> connection);
    ConnectionResult connectionIntro(
        std::weak_ptr<TConnection> connection,
        uint8_t versionMajor,
        uint8_t versionMinor,
        uint8_t versionRevision,
        std::string hostname);
    ConnectionResult connectionOutro(
        std::weak_ptr<TConnection> connection,
        std::string reason);
    ConnectionResult connectionSubscribeChannel(
        std::weak_ptr<TConnection> connection,
        ftl_channel_id_t channelId);
    ConnectionResult connectionUnsubscribeChannel(
        std::weak_ptr<TConnection> connection,
        ftl_channel_id_t channelId);
    ConnectionResult connectionStreamAvailable(
        std::weak_ptr<TConnection> connection,
        ftl_channel_id_t channelId,
        ftl_stream_id_t streamId,
        std::string hostname);
    ConnectionResult connectionStreamRemoved(
        std::weak_ptr<TConnection> connection,
        ftl_channel_id_t channelId,
        ftl_stream_id_t streamId);
    ConnectionResult connectionStreamMetadata(
        std::weak_ptr<TConnection> connection,
        ftl_channel_id_t channelId,
        ftl_stream_id_t streamId,
        uint32_t viewerCount);
    
};