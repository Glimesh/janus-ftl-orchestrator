/**
 * @file Stream.cpp
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-10-26
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#include "Stream.h"

#pragma region Constructor/Destructor
Stream::Stream(
    std::shared_ptr<IConnection> ingestConnection,
    ftl_channel_id_t channelId,
    ftl_stream_id_t streamId
) : 
    ingestConnection(ingestConnection),
    channelId(channelId),
    streamId(streamId)
{ }
#pragma endregion

#pragma region Getters/Setters
std::shared_ptr<IConnection> Stream::GetIngestConnection()
{
    return ingestConnection;
}

ftl_channel_id_t Stream::GetChannelId()
{
    return channelId;
}

ftl_stream_id_t Stream::GetStreamId()
{
    return streamId;
}
#pragma endregion