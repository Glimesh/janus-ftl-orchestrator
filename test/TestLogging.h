/**
 * @file TestLogging.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-11-11
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#pragma once

#include <catch2/catch.hpp>
#include <spdlog/sinks/base_sink.h>

#include <string>

template<typename Mutex>
class TestSink : public spdlog::sinks::base_sink<Mutex>
{
protected:
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
        // Format the message:
        spdlog::memory_buf_t formatted;
        spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
        std::string formattedMsg = fmt::to_string(formatted);
        switch (msg.level)
        {
        case spdlog::level::trace:
        case spdlog::level::debug:
        case spdlog::level::info:
        {
            // BUG: Turns out that this makes Catch2 unhappy when called from
            // multiple threads.
            // possible workaround: disable this spdlog sink when in multi-threaded test cases?
            //UNSCOPED_INFO(formattedMsg);
            break;
        }
        case spdlog::level::warn:
        {
            // BUG: Turns out that this makes Catch2 unhappy when called from
            // multiple threads.
            //WARN(formattedMsg);
            break;
        }
        case spdlog::level::err:
        case spdlog::level::critical:
        {
            // BUG: Turns out that this makes Catch2 unhappy when called from
            // multiple threads.
            //FAIL_CHECK(formattedMsg);
            break;
        }
        default:
            break;
        }
    }

    void flush_() override
    { }
};