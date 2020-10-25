/**
 * @file Configuration.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @version 0.1
 * @date 2020-10-24
 * 
 * @copyright Copyright (c) 2020 Hayden McAfee
 * 
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
    std::vector<uint8_t> GetPreSharedKey();

private:
    /* Backing stores */
    std::vector<uint8_t> preSharedKey;

    /* Private methods */
    std::vector<uint8_t> hexStringToByteArray(std::string hexString);
};