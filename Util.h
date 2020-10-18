/**
 * @file Util.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @brief Miscellaneous utilities
 * @version 0.1
 * @date 2020-10-18
 * 
 * @copyright Copyright (c) 2020 Hayden McAfee
 * 
 */

#pragma once

#include <string>
#include <string.h>

class Util
{
public:
    static std::string ErrnoToString(int error)
    {
        char errnoStrBuf[256];
        char* errMsg = strerror_r(error, errnoStrBuf, sizeof(errnoStrBuf));
        return std::string(errMsg);
    }
};