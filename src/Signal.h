#pragma once

#include <unordered_map>
#include <shared_mutex>
#include <optional>
#include <condition_variable>
#include <list>

class Signal
{
    class Channel
    {
    public:
        void Notify();

        void Close();

        template <class Rep, class Period>
        void WaitFor(const std::chrono::duration<Rep, Period> &rel_time)
        {
            std::unique_lock<std::mutex> lock(mutex);
            cv.wait_for(lock, rel_time, [&] { return signaled; });
            signaled = false;
        }

        bool IsOpen();

    private:
        bool closed = false;
        bool signaled = false;
        std::mutex mutex;
        std::condition_variable cv;
    };

public:
    class Subscription
    {
    public:
        Subscription() {}
        Subscription(std::shared_ptr<Channel> channel) : channel(channel) {}

        bool IsActive();

        template <class Rep, class Period>
        void WaitFor(const std::chrono::duration<Rep, Period> &rel_time)
        {
            channel->WaitFor(rel_time);
        }

    private:
        std::shared_ptr<Channel> channel;
    };

    void Notify();

    Subscription Subscribe();

private:
    std::list<std::weak_ptr<Channel>> channels;
    std::mutex mutex;
};