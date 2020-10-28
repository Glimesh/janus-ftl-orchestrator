/**
 * @file FtlConnection.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-11-08
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#pragma once

#include "IConnection.h"

#include "FtlTypes.h"
#include "IConnectionTransport.h"

#include <atomic>
#include <functional>
#include <memory>
#include <thread>

/**
 * @brief
 *  FtlConnection translates FTL Orchestration Protocol binary data to/from an IConnectionTransport
 *  to discrete FTL commands and events.
 * 
 */
class FtlConnection : public IConnection
{
public:
    /* Constructor/Destructor */
    FtlConnection(std::shared_ptr<IConnectionTransport> transport);

    /* IConnection */
    void Start() override;
    void Stop() override;
    void SendStreamAvailable(std::shared_ptr<Stream> stream) override;
    void SendStreamEnded(std::shared_ptr<Stream> stream) override;
    void SetOnConnectionClosed(std::function<void(void)> onConnectionClosed) override;
    void SetOnIngestNewStream(
        std::function<void(ftl_channel_id_t, ftl_stream_id_t)> onIngestNewStream) override;
    void SetOnIngestStreamEnded(
        std::function<void(ftl_channel_id_t, ftl_stream_id_t)> onIngestStreamEnded) override;
    void SetOnStreamViewersUpdated(
        std::function<void(ftl_channel_id_t, ftl_stream_id_t, uint32_t)>
        onStreamViewersUpdated) override;
    std::string GetHostname() override;

private:
    std::shared_ptr<IConnectionTransport> transport;
    std::atomic<bool> isStopping { 0 };
    std::thread connectionThread;
    std::function<void(void)> onConnectionClosed;
    std::function<void(ftl_channel_id_t, ftl_stream_id_t)> onIngestNewStream;
    std::function<void(ftl_channel_id_t, ftl_stream_id_t)> onIngestStreamEnded;
    std::function<void(ftl_channel_id_t, ftl_stream_id_t, uint32_t)> onStreamViewersUpdated;
    std::string hostname;

    /* Private methods */
    /**
     * @brief
     *  Method body that runs on a different thread started by Start(), handles incoming connection
     *  data.
     */
    void startConnectionThread();

    /**
     * @brief Called when underlying transport has closed.
     */
    void onTransportConnectionClosed();

    
};