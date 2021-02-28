/**
 * @file test.cpp
 * @author Hayden McAfee
 * @date 2020-10-25
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#include "TestLogging.h"

#include <memory>
#include <mutex>

// Some Catch2 defines required for PCH support
// https://github.com/catchorg/Catch2/blob/v2.x/docs/ci-and-misc.md#precompiled-headers-pchs
#undef TWOBLUECUBES_SINGLE_INCLUDE_CATCH_HPP_INCLUDED
#define CATCH_CONFIG_IMPL_ONLY
#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>

int main(int argc, char* argv[])
{
    // Set up logging
    auto testLoggingSink = std::make_shared<TestSink<std::mutex>>();
    auto testLogger = std::make_shared<spdlog::logger>("testlogger", testLoggingSink);
    spdlog::set_default_logger(testLogger);
#ifdef DEBUG
    spdlog::set_level(spdlog::level::trace);
#endif

    // Test!
    int result = Catch::Session().run(argc, argv);
    return result;
}