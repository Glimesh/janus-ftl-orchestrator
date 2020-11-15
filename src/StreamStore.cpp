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

    if (streamsByIngestConnection.count(stream.IngestConnection) <= 0)
    {
        streamsByIngestConnection[stream.IngestConnection] = std::list<Stream>();
    }
    streamsByIngestConnection[stream.IngestConnection].push_back(stream);
}

std::optional<Stream> StreamStore::RemoveStream(
    ftl_channel_id_t channelId,
    ftl_stream_id_t streamId)
{
    std::lock_guard<std::mutex> lock(streamStoreMutex);
    if (streamByChannelId.count(channelId) > 0)
    {
        Stream returnStream = streamByChannelId[channelId];
        streamByChannelId.erase(channelId);
        if (streamsByIngestConnection.count(returnStream.IngestConnection) <= 0)
        {
            throw std::runtime_error(
                "Inconsistent StreamStore state - could not locate connection for "
                "existing stream.");
        }
        auto& streamList = streamsByIngestConnection[returnStream.IngestConnection];
        streamList.remove_if(
            [&channelId, &streamId](auto stream)
            {
                return ((stream.ChannelId == channelId) && (stream.StreamId == streamId));
            });
        if (streamList.empty())
        {
            streamsByIngestConnection.erase(returnStream.IngestConnection);
        }
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

std::optional<std::list<Stream>> StreamStore::RemoveAllConnectionStreams(
    std::shared_ptr<IConnection> connection)
{
    std::lock_guard<std::mutex> lock(streamStoreMutex);
    if (streamsByIngestConnection.count(connection) > 0)
    {
        std::list<Stream> streams = streamsByIngestConnection[connection];
        streamsByIngestConnection.erase(connection);
        for (const auto& stream : streams)
        {
            if (streamByChannelId.count(stream.ChannelId) <= 0)
            {
                throw std::runtime_error(
                    "Inconsistent StreamStore state - could not locate matching stream entry "
                    "for connection.");
            }
            streamByChannelId.erase(stream.ChannelId);
        }
        return streams;
    }
    return std::nullopt;
}
#pragma endregion