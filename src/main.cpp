/**
 * @file main.cpp
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-10-18
 * @copyright Copyright (c) 2020 Hayden McAfee
 * 
 */

#include "Configuration.h"
// #include "FtlConnection.h"
// #include "Orchestrator.h"
#include "FtlOrchestractorService.h"
// #include "TlsConnectionManager.h"

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <spdlog/spdlog.h>

#include <memory>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;


void RunServer() {
  std::string server_address("0.0.0.0:50051");
  FtlOrchestratorServiceImpl service;

  grpc::EnableDefaultHealthCheckService(true);
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}


/**
 * @brief Entrypoint for the program binary.
 * 
 * @return int exit status
 */
int main()
{
    std::unique_ptr<Configuration> configuration = std::make_unique<Configuration>();
    configuration->Load();

    // // Set up our service to listen to orchestration connections via TCP/TLS
    // auto connectionManager = 
    //     std::make_shared<TlsConnectionManager<FtlConnection>>(configuration->GetPreSharedKey());
    // std::unique_ptr<Orchestrator<FtlConnection>> orchestrator = 
    //     std::make_unique<Orchestrator<FtlConnection>>(connectionManager);
    
    // // Initialize our classes
    // connectionManager->Init();
    // orchestrator->Init();

    // Off we go
    RunServer();


    return 0;
}