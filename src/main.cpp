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
    auto orchestrator = std::make_unique<Orchestrator<FtlConnection>>(
            std::make_unique<TlsConnectionManager<FtlConnection>>(
                configuration->GetPreSharedKey()));
    
    // Initialize
    orchestrator->Init();

    // Off we go
    orchestrator->Run();
    return 0;
}