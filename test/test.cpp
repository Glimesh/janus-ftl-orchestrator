/**
 * @file test.cpp
 * @author Hayden McAfee
 * @date 2020-10-25
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#define CATCH_CONFIG_RUNNER // This tells Catch that we'll be providing the main entrypoint
#include <catch2/catch.hpp>
#include <spdlog/spdlog.h>

#include "TestLogging.h"

#include <memory>
#include <mutex>

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