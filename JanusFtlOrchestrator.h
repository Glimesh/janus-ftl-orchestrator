/**
 * @file JanusFtlOrchestrator.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @version 0.1
 * @date 2020-10-18
 * 
 * @copyright Copyright (c) 2020 Hayden McAfee
 * 
 */

#pragma once

#include <arpa/inet.h>
#include <memory>
#include <set>

class OrchestrationConnection;

/**
 * @brief
 *  JanusFtlOrchestrator handles listening for and maintaining incoming
 *  orchestration connections
 */
class JanusFtlOrchestrator
{
public:
    /* Constructor/Destructor */
    /**
     * @brief Construct a new Janus Ftl Orchestrator object.
     * 
     * @param listenPort port that the orchestration service should listen for new connections on
     */
    JanusFtlOrchestrator(in_port_t listenPort = DEFAULT_LISTEN_PORT);

    /* Public methods */
    /**
     * @brief Performs initialization steps needed before we can start listening.
     */
    void Init();

    /**
     * @brief Listens for incoming connections from ftl-plugin clients.
     */
    void Listen();

    /**
     * @brief Stops listening and shuts down any outstanding resources.
     */
    void Stop();

private:
    /**
     * @brief Default port the orchestration service will listen on.
     */
    static constexpr in_port_t DEFAULT_LISTEN_PORT = 8085;

    /**
     * @brief Maximum number of connections that can be queued on the socket before they are accepted.
     */
    static constexpr int SOCKET_LISTEN_QUEUE_LIMIT = 64;

    /**
     * @brief Port that the orchestration service is listening on.
     */
    const in_port_t listenPort;

    /**
     * @brief Handle to the socket bound to listen for incoming orchestration connections.
     */
    int listenSocketHandle;

    /**
     * @brief Stores references to connections that have not yet been negotiated.
     */
    std::set<std::shared_ptr<OrchestrationConnection>> pendingConnections;
};