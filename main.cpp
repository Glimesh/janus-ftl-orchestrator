/**
 * @file main.cpp
 * @author Hayden McAfee (hayden@outlook.com)
 * @version 0.1
 * @date 2020-10-18
 * 
 * @copyright Copyright (c) 2020 Hayden McAfee
 * 
 */

#include "JanusFtlOrchestrator.h"

#include <memory>

/**
 * @brief Entrypoint for the program binary.
 * 
 * @return int exit status
 */
int main()
{
    std::unique_ptr<JanusFtlOrchestrator> orchestrator = 
        std::make_unique<JanusFtlOrchestrator>();
    orchestrator->Init();
    orchestrator->Listen();
    return 0;
}