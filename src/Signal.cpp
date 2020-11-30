#include "Signal.h"

void Signal::Channel::Notify()
{
    std::unique_lock<std::mutex> lock(mutex);
    signaled = true;
    lock.unlock(); // unlock before notify to minimize mutex contention
    cv.notify_one();
}

void Signal::Channel::Close()
{
    std::unique_lock<std::mutex> lock(mutex);
    closed = true;
    lock.unlock(); // unlock before notify to minimize mutex contention
    cv.notify_one();
}

bool Signal::Channel::IsOpen()
{
    return !closed;
}

bool Signal::Subscription::IsActive()
{
    return channel->IsOpen();
}

void Signal::Notify()
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

Signal::Subscription Signal::Subscribe()
{
    std::lock_guard<std::mutex> lock(mutex);
    std::shared_ptr<Channel> channel = std::make_shared<Channel>();
    channels.push_back(std::weak_ptr<Channel>(channel));
    return Subscription(channel);
}
