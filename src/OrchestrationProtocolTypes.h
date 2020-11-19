/**
 * @file OrchestrationProtocolTypes.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-11-09
 * @copyright Copyright (c) 2020 Hayden McAfee
 * @brief
 *  Contains types useful for communicating via the FTL Orchestration Protocol
 *  see docs/PROTOCOL.md
 */

#pragma once

#include <cstdint>

enum class OrchestrationMessageDirectionKind
{
    Request = 0,
    Response = 1,
};

enum class OrchestrationMessageType : uint8_t
{
    Intro               = 0,
    Outro               = 1,
    NodeState           = 2,
    ChannelSubscription = 16,
    StreamPublish       = 17,
    StreamRelay         = 20,
};

struct OrchestrationMessageHeader
{
    OrchestrationMessageDirectionKind MessageDirection;
    bool MessageFailure;
    OrchestrationMessageType MessageType;
    uint8_t MessageId;
    uint16_t MessagePayloadLength;
};