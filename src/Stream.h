/**
 * @file Stream.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-10-26
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#pragma once

#include "FtlTypes.h"

#include <memory>

/**
 * @brief Describes an FTL stream
 */
template <class TConnection>
struct Stream
{
    std::shared_ptr<TConnection> IngestConnection;
    ftl_channel_id_t ChannelId;
    ftl_stream_id_t StreamId;
};