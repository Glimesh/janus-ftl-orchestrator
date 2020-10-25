/**
 * @file MockConnection.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-10-25
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#pragma once

#include "../../IConnection.h"

#include <functional>
#include <string>

class MockConnection : public IConnection
{
public:
    // Mock constructor
    MockConnection(std::string hostname, FtlServerKind serverKind) : 
        hostname(hostname),
        serverKind(serverKind)
    { }

    // Mock utilities
    void MockFireOnConnectionClosed()
    {
        onConnectionClosed();
    }

    // IConnection
    void Start() override
    { }

    void Stop() override
    { }

    void SetOnConnectionClosed(std::function<void(void)> onConnectionClosed) override
    {
        this->onConnectionClosed = onConnectionClosed;
    }

    std::string GetHostname() override
    {
        return hostname;
    }

    FtlServerKind GetServerKind() override
    {
        return serverKind;
    }

private:
    std::function<void(void)> onConnectionClosed;
    std::string hostname;
    FtlServerKind serverKind;
};