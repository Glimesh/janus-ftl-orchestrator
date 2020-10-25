/**
 * @file MockConnectionManager.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-10-25
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#pragma once

#include "../../IConnectionManager.h"
#include "../../IConnection.h"

#include <functional>
#include <memory>

class MockConnectionManager : public IConnectionManager
{
public:
    // Mock utilities
    void MockFireNewConnection(std::shared_ptr<IConnection> connection)
    {
        onNewConnection(connection);
    }

    // IConnectionManager
    void Init() override
    { }

    void Listen() override
    { }

    void SetOnNewConnection(std::function<void(std::shared_ptr<IConnection>)> onNewConnection)
    {
        this->onNewConnection = onNewConnection;
    }

private:
    std::function<void(std::shared_ptr<IConnection>)> onNewConnection;
};