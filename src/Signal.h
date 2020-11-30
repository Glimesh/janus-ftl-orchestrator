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
        void Notify()
        {
            std::unique_lock<std::mutex> lock(mutex);
            signaled = true;
            lock.unlock(); // unlock before notify to minimize mutex contention
            cv.notify_one();
        }

        void Close()
        {
            std::unique_lock<std::mutex> lock(mutex);
            closed = true;
            lock.unlock(); // unlock before notify to minimize mutex contention
            cv.notify_one();
        }

        template <class Rep, class Period>
        std::cv_status WaitFor(const std::chrono::duration<Rep, Period> &rel_time)
        {
            std::unique_lock<std::mutex> lock(mutex);
            return cv.wait(lock, [] { return signaled; });
        }

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
        Subscription();
        Subscription(std::shared_ptr<Signal::Channel> channel);

        bool IsActive();

        std::cv_status Wait();

        template <class Rep, class Period>
        std::cv_status WaitFor(const std::chrono::duration<Rep, Period> &rel_time);

    private:
        std::shared_ptr<Channel> channel = std::make_shared<Channel>();
    };

    void Notify()
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto iter = channels.begin();
        while (iter != channels.end())
        {
            if (auto subscriber = iter->lock())
            {
                subscriber->Notify();
                ++iter;
            }
            else
            {
                channels.erase(iter++);
            }
        }
    }

    Subscription Subscribe()
    {
        std::lock_guard<std::mutex> lock(mutex);
        std::shared_ptr<Channel> channel = std::make_shared<Channel>();
        channels.push_back(std::weak_ptr<Channel>(channel));
        Subscription subscription(channel);
        return subscription;
    }

private:
    std::list<std::weak_ptr<Channel>> channels;
    std::mutex mutex;
};