/**
 * @file MockConnectionTransport.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-11-09
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#pragma once

#include "../../IConnectionTransport.h"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <vector>

class MockConnectionTransport : public IConnectionTransport
{
public:
    void MockSetReadBuffer(std::vector<uint8_t> buffer)
    {
        std::lock_guard<std::mutex> lock(readMutex);
        readBuffer = buffer;
    }

    std::vector<uint8_t> MockGetWriteBuffer(bool clear = false)
    {
        std::lock_guard<std::mutex> lock(writeMutex);
        auto returnBuffer = writeBuffer;
        if (clear)
        {
            writeBuffer.clear();
        }
        return returnBuffer;
    }

    std::optional<std::vector<uint8_t>> WaitForWrite(
        std::chrono::milliseconds timeout = std::chrono::milliseconds(1000))
    {
        std::unique_lock<std::mutex> lock(writeMutex);
        writeConditionVariable.wait_for(lock, timeout);
        if (writeBuffer.size() > 0)
        {
            return std::optional<std::vector<uint8_t>>(writeBuffer);
        }
        else
        {
            return std::optional<std::vector<uint8_t>>();
        }
    }

    /* IConnectionTransport */
    void Start() override
    { }

    void Stop() override
    {
        onConnectionClosed();
    }

    std::vector<uint8_t> Read() override
    {
        std::lock_guard<std::mutex> lock(readMutex);
        auto returnBuffer = readBuffer;
        readBuffer.clear();
        return returnBuffer;
    }

    void Write(const std::vector<uint8_t>& bytes) override
    {
        {
            std::lock_guard<std::mutex> lock(writeMutex);
            writeBuffer.insert(writeBuffer.end(), bytes.begin(), bytes.end());
        }
        writeConditionVariable.notify_all();
    }

    void SetOnConnectionClosed(std::function<void(void)> onConnectionClosed) override
    {
        this->onConnectionClosed = onConnectionClosed;
    }

private:
    std::mutex readMutex;
    std::vector<uint8_t> readBuffer;
    std::mutex writeMutex;
    std::condition_variable writeConditionVariable;
    std::vector<uint8_t> writeBuffer;
    std::function<void(void)> onConnectionClosed;
};