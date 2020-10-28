/**
 * @file FtlConnection.cpp
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-11-08
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#include "FtlConnection.h"

#pragma region Constructor/Destructor
FtlConnection::FtlConnection(std::shared_ptr<IConnectionTransport> transport) : 
    transport(transport)
{ }
#pragma endregion

#pragma region IConnection
void FtlConnection::Start()
{
    // Bind to transport events
    transport->SetOnConnectionClosed(std::bind(&FtlConnection::onTransportConnectionClosed, this));

    // Start the transport
    transport->Start();

    // Spin up the thread that will listen to data coming from our transport
    connectionThread = std::thread(&FtlConnection::startConnectionThread, this);
    connectionThread.detach();
}

void FtlConnection::Stop()
{
    // Stop the transport, which should halt our connection thread.
    transport->Stop();
    if (connectionThread.joinable())
    {
        connectionThread.join();
    }
}

void FtlConnection::SendStreamAvailable(std::shared_ptr<Stream> stream)
{

}

void FtlConnection::SendStreamEnded(std::shared_ptr<Stream> stream)
{
    
}

void FtlConnection::SetOnConnectionClosed(std::function<void(void)> onConnectionClosed)
{
    this->onConnectionClosed = onConnectionClosed;
}

void FtlConnection::SetOnIngestNewStream(
    std::function<void(ftl_channel_id_t, ftl_stream_id_t)> onIngestNewStream)
{
    this->onIngestNewStream = onIngestNewStream;
}

void FtlConnection::SetOnIngestStreamEnded(
    std::function<void(ftl_channel_id_t, ftl_stream_id_t)> onIngestStreamEnded)
{
    this->onIngestStreamEnded = onIngestStreamEnded;
}

void FtlConnection::SetOnStreamViewersUpdated(
    std::function<void(ftl_channel_id_t, ftl_stream_id_t, uint32_t)> onStreamViewersUpdated)
{
    this->onStreamViewersUpdated = onStreamViewersUpdated;
}

std::string FtlConnection::GetHostname()
{
    return hostname;
}
#pragma endregion

#pragma region Private methods
void FtlConnection::startConnectionThread()
{
    while (true)
    {
        if (isStopping)
        {
            break;
        }

        // Try to read in some data
        std::vector<uint8_t> buffer;
        std::vector<uint8_t> readBytes = transport->Read();
        buffer.insert(buffer.end(), readBytes.begin(), readBytes.end());
    }

}

void FtlConnection::onTransportConnectionClosed()
{
    isStopping = true;
}
#pragma endregion