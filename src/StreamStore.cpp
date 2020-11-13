/**
 * @file StreamStore.cpp
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-10-26
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#include "StreamStore.h"

#include "Stream.h"

#include <spdlog/spdlog.h>
#include <sstream>
#include <stdexcept>

#pragma region Public methods
void StreamStore::AddStream(Stream stream)
{
    std::lock_guard<std::mutex> lock(streamStoreMutex);
    ftl_channel_id_t channelId = stream.ChannelId;
    if (streamByChannelId.count(channelId) > 0)
    {
        std::stringstream errStr;
        errStr << "Found Stream with duplicate channel id " << channelId
            << "when attempting to add new Stream to StreamStore!";
        throw std::runtime_error(errStr.str());
    }
    streamByChannelId[channelId] = stream;
}

std::optional<Stream> StreamStore::RemoveStream(ftl_channel_id_t channelId, ftl_stream_id_t streamId)
{
    std::lock_guard<std::mutex> lock(streamStoreMutex);
    if (streamByChannelId.count(channelId) > 0)
    {
        Stream returnStream = streamByChannelId[channelId];
        streamByChannelId.erase(channelId);
        return returnStream;
    }
    return std::nullopt;
}

std::optional<Stream> StreamStore::GetStreamByChannelId(ftl_channel_id_t channelId)
{
    std::lock_guard<std::mutex> lock(streamStoreMutex);
    if (streamByChannelId.count(channelId) > 0)
    {
        return streamByChannelId[channelId];
    }
    return std::nullopt;
}
#pragma endregion