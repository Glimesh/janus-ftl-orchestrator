/**
 * @file MockConnectionManager.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-10-25
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#pragma once

#include "../../src/IConnectionManager.h"
#include "../../src/IConnection.h"

#include <functional>
#include <memory>

template <class TConnection>
class MockConnectionManager : public IConnectionManager<TConnection>
{
public:
    // Mock utilities
    void MockFireNewConnection(std::shared_ptr<TConnection> connection)
    {
        onNewConnection(connection);
    }

    // IConnectionManager
    void Init() override
    { }

    void Listen() override
    { }

    void SetOnNewConnection(std::function<void(std::shared_ptr<TConnection>)> onNewConnection)
    {
        this->onNewConnection = onNewConnection;
    }

private:
    std::function<void(std::shared_ptr<TConnection>)> onNewConnection;
};