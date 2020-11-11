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
    void AddStream(std::shared_ptr<Stream> stream);

    /**
     * @brief Removes a stream with the given IDs from the store
     * 
     * @param channelId channel ID to remove
     * @param streamId stream ID to remove
     * @return bool true when a stream has been removed, false if no stream has been removed
     */
    bool RemoveStream(ftl_channel_id_t channelId, ftl_stream_id_t streamId);

    /**
     * @brief Given a channel ID, returns the Stream object associated with this channel
     * @param channelId channel ID of Stream to return
     * @return std::shared_ptr<Stream> reference to Stream
     * @return nullptr if Stream could not be found
     */
    std::shared_ptr<Stream> GetStreamByChannelId(ftl_channel_id_t channelId);

private:
    std::mutex streamStoreMutex;
    std::map<ftl_channel_id_t, std::shared_ptr<Stream>> streamByChannelId;
};