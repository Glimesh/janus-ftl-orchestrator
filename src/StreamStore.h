/**
 * @file StreamStore.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-10-26
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#pragma once

#include "FtlTypes.h"

#include <map>
#include <memory>
#include <mutex>
#include <optional>

class Stream;

/**
 * @brief Manages storage and retrieval of Streams
 */
class StreamStore
{
public:
    /* Public methods */
    /**
     * @brief Adds stream to the store.
     * NOTE: It is expected that callers verify there is no duplicate Stream in the Store already.
     * @param stream Stream to add
     */
    void AddStream(Stream stream);

    /**
     * @brief Removes a stream with the given IDs from the store
     * 
     * @param channelId channel ID to remove
     * @param streamId stream ID to remove
     * @return std::optional<Stream> Stream, if it exists
     */
    std::optional<Stream> RemoveStream(ftl_channel_id_t channelId, ftl_stream_id_t streamId);

    /**
     * @brief Given a channel ID, returns the Stream object associated with this channel
     * @param channelId channel ID of Stream to return
     * @return std::optional<Stream> Stream, if it exists
     */
    std::optional<Stream> GetStreamByChannelId(ftl_channel_id_t channelId);

private:
    std::mutex streamStoreMutex;
    std::map<ftl_channel_id_t, Stream> streamByChannelId;
};