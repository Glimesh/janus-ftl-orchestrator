/**
 * @file Orchestrator.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-10-18
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#pragma once

#include "FtlTypes.h"

#include <arpa/inet.h>
#include <map>
#include <memory>
#include <mutex>
#include <set>

// Forward declarations
class Configuration;
class IConnection;
class IConnectionManager;
class StreamStore;

/**
 * @brief
 *  Orchestrator handles listening for and maintaining incoming
 *  orchestration connections
 */
class Orchestrator
{
public:
    /* Constructor/Destructor */
    Orchestrator(std::shared_ptr<IConnectionManager> connectionManager);

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
    const std::set<std::shared_ptr<IConnection>> GetConnections();

private:
    /* Private members */
    const std::shared_ptr<IConnectionManager> connectionManager;
    std::unique_ptr<StreamStore> streamStore;
    std::mutex connectionsMutex;
    std::set<std::shared_ptr<IConnection>> connections;
    std::mutex streamsMutex;
    std::map<ftl_channel_id_t, std::shared_ptr<IConnection>> channelIngestConnections;

    /* Private methods */
    void newConnection(std::shared_ptr<IConnection> connection);
    void connectionClosed(std::shared_ptr<IConnection> connection);
    void ingestNewStream(
        std::shared_ptr<IConnection> connection,
        ftl_channel_id_t channelId,
        ftl_stream_id_t streamId);
    void ingestStreamEnded(
        std::shared_ptr<IConnection> connection,
        ftl_channel_id_t channelId,
        ftl_stream_id_t streamId);
    void streamViewersUpdated(
        std::shared_ptr<IConnection> connection,
        ftl_channel_id_t channelId,
        ftl_stream_id_t streamId,
        uint32_t viewerCount);
    
};