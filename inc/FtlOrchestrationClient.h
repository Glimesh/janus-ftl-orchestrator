/**
 * @file FtlOrchestrationClient.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-11-25
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#pragma once

#include <string>

/**
 * @brief
 *  This is a class that allows you to easily connect to and communicate with the FTL Orchestration
 *  Service via the FTL Orchestration protocol.
 *  This is meant to be consumed by applications as a header-only library.
 */
class FtlOrchestrationClient
{
public:
    /**
     * @brief Construct a new Ftl Orchestration Client to connect to the given hostname
     * @param hostname hostname of FTL Orchestration Server
     */
    FtlOrchestrationClient(std::string hostname) : serverHostname(hostname)
    {

    }

private:
    std::string serverHostname;
};