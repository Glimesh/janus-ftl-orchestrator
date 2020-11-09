/**
 * @file main.cpp
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-10-18
 * @copyright Copyright (c) 2020 Hayden McAfee
 * 
 */

#include "Configuration.h"
#include "FtlConnection.h"
#include "Orchestrator.h"
#include "TlsConnectionManager.h"

#include <memory>

/**
 * @brief Entrypoint for the program binary.
 * 
 * @return int exit status
 */
int main()
{
    std::unique_ptr<Configuration> configuration = std::make_unique<Configuration>();
    configuration->Load();

    // Set up our service to listen to orchestration connections via TCP/TLS
    auto connectionManager = 
        std::make_shared<TlsConnectionManager<FtlConnection>>(configuration->GetPreSharedKey());
    std::unique_ptr<Orchestrator> orchestrator = 
        std::make_unique<Orchestrator>(connectionManager);
    
    // Initialize our classes
    connectionManager->Init();
    orchestrator->Init();

    // Off we go
    connectionManager->Listen();
    return 0;
}