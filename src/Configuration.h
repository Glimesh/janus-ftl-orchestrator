/**
 * @file Configuration.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-10-24
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

class Configuration
{
public:
    /* Public methods */
    void Load();

    /* Configuration values */
    std::vector<std::byte> GetPreSharedKey();

private:
    /* Backing stores */
    std::vector<std::byte> preSharedKey;

    /* Private methods */
    std::vector<std::byte> hexStringToByteArray(std::string hexString);
};