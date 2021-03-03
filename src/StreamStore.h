/**
 * @file StreamStore.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-10-26
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#pragma once

#include "FtlTypes.h"
#include "IConnection.h"
#include "Stream.h"

#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>

/**
 * @brief Manages storage and retrieval of Streams
 */
template <class TConnection>
class StreamStore
{
public:
    /* Public methods */
    /**
     * @brief
     *  Adds stream to the store.
     *  NOTE: It is expected that callers verify there is no duplicate Stream in the Store already.
     * @param stream Stream to add
     */
    void AddStream(Stream<TConnection> stream)
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
            streamsByIngestConnection[stream.IngestConnection] = std::list<Stream<TConnection>>();
        }
        streamsByIngestConnection[stream.IngestConnection].push_back(stream);
    }

    /**
     * @brief Removes a stream with the given IDs from the store
     * @param channelId channel ID to remove
     * @param streamId stream ID to remove
     * @return std::optional<Stream> Stream, if it exists
     */
    std::optional<Stream<TConnection>> RemoveStream(
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
                // HACK - make this non-fatal
                spdlog::error("Inconsistent StreamStore state - could not locate connection for "
                    "existing stream.");
                return std::nullopt;
                // throw std::runtime_error(
                //     "Inconsistent StreamStore state - could not locate connection for "
                //     "existing stream.");
                // /HACK
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

    /**
     * @brief Given a channel ID, returns the Stream object associated with this channel
     * @param channelId channel ID of Stream to return
     * @return std::optional<Stream> Stream, if it exists
     */
    std::optional<Stream<TConnection>> GetStreamByChannelId(ftl_channel_id_t channelId)
    {
        std::lock_guard<std::mutex> lock(streamStoreMutex);
        if (streamByChannelId.count(channelId) > 0)
        {
            return streamByChannelId[channelId];
        }
        return std::nullopt;
    }

    /**
     * @brief Remove and return all streams originating from the given connection.
     * @param connection the connection to find streams for
     * @return std::optional<std::list<Stream>> list of streams removed, if any
     */
    std::optional<std::list<Stream<TConnection>>> RemoveAllConnectionStreams(
        std::shared_ptr<TConnection> connection)
    {
        std::lock_guard<std::mutex> lock(streamStoreMutex);
        if (streamsByIngestConnection.count(connection) > 0)
        {
            std::list<Stream<TConnection>> streams = streamsByIngestConnection[connection];
            streamsByIngestConnection.erase(connection);
            for (const auto& stream : streams)
            {
                if (streamByChannelId.count(stream.ChannelId) <= 0)
                {
                    // HACK - make this not fatal
                    spdlog::error("Inconsistent StreamStore state - could not locate matching "
                        "stream entry for connection.");
                    // throw std::runtime_error(
                    //     "Inconsistent StreamStore state - could not locate matching stream entry "
                    //     "for connection.");
                    // /HACK
                }
                streamByChannelId.erase(stream.ChannelId);
            }
            return streams;
        }
        return std::nullopt;
    }

    /**
     * @brief Clears all records
     */
    void Clear()
    {
        std::lock_guard<std::mutex> lock(streamStoreMutex);
        streamByChannelId.clear();
        streamsByIngestConnection.clear();
    }

private:
    std::mutex streamStoreMutex;
    std::map<ftl_channel_id_t, Stream<TConnection>> streamByChannelId;
    std::map<std::shared_ptr<TConnection>, std::list<Stream<TConnection>>> streamsByIngestConnection;
};