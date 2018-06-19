#include "clientserver.h"
#include <controlmessage.pb.h>
#include "server/modelprotohandler/modelprotohandler.h"
#include "server/hardwareprotohandler/hardwareprotohandler.h"
#include <zmq.hpp>


ClientServer::ClientServer(Execution& execution, const std::string& address, const std::string& experiment_id, const std::string& environment_manager_address) : execution_(execution)
{

    std::string default_database_file_name = experiment_id + ".sql";

    address_ = address;
    experiment_id_ = experiment_id;
    environment_manager_address_ = environment_manager_address;

    requester_ = std::unique_ptr<EnvironmentRequester>(new EnvironmentRequester(environment_manager_address_, experiment_id_, EnvironmentRequester::DeploymentType::LOGAN));

    requester_->AddUpdateCallback(std::bind(&ClientServer::HandleUpdate, this, std::placeholders::_1));
    requester_->Init(environment_manager_address_);
    requester_->SetIPAddress(address);

    requester_->Start();

    auto message = requester_->GetLoganInfo(address);

    if(message.type() != NodeManager::EnvironmentMessage::LOGAN_RESPONSE){
        std::cerr << "Unrecognised response from environment manager." << std::endl;
        throw std::runtime_error("Unrecognised response from environment manager.");
    }

    for(const auto logger : message.logger()){
        if(logger.type() == NodeManager::Logger::SERVER){
            std::cout << "Server: " << std::endl;
            std::vector<std::string> client_list;
            for(const auto& address : logger.client_addresses()){
                std::cerr << "\tClient: " << address << std::endl;
                client_list.push_back(address);
            }
            servers_.push_back(std::move(std::unique_ptr<Server>(new Server(logger.db_file_name(), client_list))));
            servers_.back()->AddProtoHandler(new HardwareProtoHandler());
            servers_.back()->AddProtoHandler(new ModelProtoHandler());
            servers_.back()->Start();
        }
        if(logger.type() == NodeManager::Logger::CLIENT){
            std::cout << "Client: " << std::endl;
            double frequency = 0;
            std::vector<std::string> processes;
            bool live_mode = logger.mode() == NodeManager::Logger::LIVE;
            clients_.push_back(std::move(std::unique_ptr<LogController>(new LogController())));
            clients_.back()->Start("tcp://" + logger.publisher_address() + ":" + logger.publisher_port(), frequency, processes, live_mode);
            std::cout << "Publish Address: tcp://" << logger.publisher_address() << ":" << logger.publisher_port() << std::endl;
        }
    }

    should_run_ = clients_.size() || servers_.size();
}

bool ClientServer::ShouldRun(){
    return should_run_;
}

void ClientServer::Terminate(){
    for(const auto& client : clients_){
        client->Stop();
    }

    for(const auto& server : servers_){
        server->Terminate();
    }
}

void ClientServer::HandleUpdate(NodeManager::EnvironmentMessage& message){
    switch(message.type()){
        case NodeManager::EnvironmentMessage::ERROR_RESPONSE:{
            Terminate();
            break;
        }
        default:{
            Terminate();
        }
    }
}
