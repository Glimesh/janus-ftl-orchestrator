/**
 * @file MockConnectionTransport.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-11-09
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#pragma once

#include <IConnectionTransport.h>

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <vector>

class MockConnectionTransport : public IConnectionTransport
{
public:
    void MockSetReadBuffer(std::vector<std::byte> buffer)
    {
        if (onBytesReceived)
        {
            onBytesReceived(buffer);
        }
    }

    std::optional<std::vector<std::byte>> WaitForWrite(
        std::chrono::milliseconds timeout = std::chrono::milliseconds(1000))
    {
        std::unique_lock<std::mutex> lock(writeMutex);
        if (writeBuffer.size() <= 0)
        {
            writeConditionVariable.wait_for(lock, timeout);
        }
        if (writeBuffer.size() > 0)
        {
            auto tempBuffer = writeBuffer;
            writeBuffer.clear();
            return tempBuffer;
        }
        else
        {
            return std::nullopt;
        }
    }

    /* IConnectionTransport */
    void StartAsync() override
    { }

    void Stop() override
    { }

    void Write(const std::vector<std::byte>& bytes) override
    {
        {
            std::lock_guard<std::mutex> lock(writeMutex);
            writeBuffer.insert(writeBuffer.end(), bytes.begin(), bytes.end());
        }
        writeConditionVariable.notify_all();
    }

    void SetOnBytesReceived(
        std::function<void(const std::vector<std::byte>&)> onBytesReceived) override
    {
        this->onBytesReceived = onBytesReceived;
    }

    void SetOnConnectionClosed(std::function<void(void)> onConnectionClosed) override
    {
        this->onConnectionClosed = onConnectionClosed;
    }

private:
    std::mutex writeMutex;
    std::condition_variable writeConditionVariable;
    std::vector<std::byte> writeBuffer;
    std::function<void(const std::vector<std::byte>&)> onBytesReceived;
    std::function<void(void)> onConnectionClosed;
};