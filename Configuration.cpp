/**
 * @file Configuration.cpp
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-10-25
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#include "Configuration.h"

#include <spdlog/spdlog.h>
#include <sstream>

#pragma region Public methods
void Configuration::Load()
{
    // Set default pre-shared key
    preSharedKey = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    };

    // FTL_ORCHESTRATOR_PSK -> PreSharedKey
    if (char* varVal = std::getenv("FTL_ORCHESTRATOR_PSK"))
    {
        preSharedKey = hexStringToByteArray(std::string(varVal));
    }
    else
    {
        spdlog::warn(
            "Using default Pre-Shared Key. Consider setting your own key using "
            "the environment variable FTL_ORCHESTRATOR_PSK!");
    }
}

std::vector<uint8_t> Configuration::GetPreSharedKey()
{
    return preSharedKey;
}
#pragma endregion

#pragma region Private methods
std::vector<uint8_t> Configuration::hexStringToByteArray(std::string hexString)
{
    std::vector<uint8_t> retVal;
    std::stringstream convertStream;

    unsigned int buffer;
    unsigned int offset = 0;
    while (offset < hexString.length()) 
    {
        convertStream.clear();
        convertStream << std::hex << hexString.substr(offset, 2);
        convertStream >> std::hex >> buffer;
        retVal.push_back(static_cast<uint8_t>(buffer));
        offset += 2;
    }

    return retVal;
}
#pragma endregion