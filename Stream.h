/**
 * @file Stream.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-10-26
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#pragma once

#include "FtlTypes.h"

#include <memory>

class IConnection;

/**
 * @brief Describes an FTL stream
 */
class Stream
{
public:
    /* Constructor/Destructor */
    Stream(std::shared_ptr<IConnection> ingestConnection, ftl_channel_id_t channelId, ftl_stream_id_t streamId);

    /* Getters/Setters */
    std::shared_ptr<IConnection> GetIngestConnection();
    ftl_channel_id_t GetChannelId();
    ftl_stream_id_t GetStreamId();

private:
    /* Private members */
    const std::shared_ptr<IConnection> ingestConnection;
    const ftl_channel_id_t channelId;
    const ftl_stream_id_t streamId;
};