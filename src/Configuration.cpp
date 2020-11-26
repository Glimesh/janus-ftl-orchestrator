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
        std::byte(0x00), std::byte(0x01), std::byte(0x02), std::byte(0x03),
        std::byte(0x04), std::byte(0x05), std::byte(0x06), std::byte(0x07),
        std::byte(0x08), std::byte(0x09), std::byte(0x0a), std::byte(0x0b),
        std::byte(0x0c), std::byte(0x0d), std::byte(0x0e), std::byte(0x0f),
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

std::vector<std::byte> Configuration::GetPreSharedKey()
{
    return preSharedKey;
}
#pragma endregion

#pragma region Private methods
std::vector<std::byte> Configuration::hexStringToByteArray(std::string hexString)
{
    std::vector<std::byte> retVal;
    std::stringstream convertStream;

    unsigned int buffer;
    unsigned int offset = 0;
    while (offset < hexString.length()) 
    {
        convertStream.clear();
        convertStream << std::hex << hexString.substr(offset, 2);
        convertStream >> std::hex >> buffer;
        retVal.push_back(static_cast<std::byte>(buffer));
        offset += 2;
    }

    return retVal;
}
#pragma endregion