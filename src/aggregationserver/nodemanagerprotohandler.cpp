#include "nodemanagerprotohandler.h"

#include "utils.h"

#include "databaseclient.h"
#include "experimenttracker.h"

#include <zmq/protoreceiver/protoreceiver.h>

#include <proto/controlmessage/helper.h>

#include <functional>


void NodeManagerProtoHandler::BindCallbacks(zmq::ProtoReceiver& receiver) {
    receiver.RegisterProtoCallback<NodeManager::ControlMessage>(std::bind(&NodeManagerProtoHandler::ProcessControlMessage, this, std::placeholders::_1));
}



void NodeManagerProtoHandler::ProcessEnvironmentMessage(const NodeManager::EnvironmentMessage& message) {
    switch (message.type()) {
        case NodeManager::EnvironmentMessage::CONFIGURE_EXPERIMENT:
        {
            ProcessControlMessage(message.control_message());
            return;
        }
    }
}

void NodeManagerProtoHandler::ProcessControlMessage(const NodeManager::ControlMessage& message) {
    switch(message.type()) {
        default: {      // NOTE: For the moment we treat all message equally
            int exp_run_id = experiment_tracker_->RegisterExperimentRun(message.experiment_id(), message.time_stamp());

            for (const auto& node : message.nodes()) {
                ProcessNode(node, exp_run_id);
            }
        }
    }
}

void NodeManagerProtoHandler::ProcessNode(const NodeManager::Node& message, int experiment_run_id) {
    switch (message.type()) {
        // For anything containing nodes just process their children
        case NodeManager::Node::HARDWARE_CLUSTER:
        case NodeManager::Node::DOCKER_CLUSTER: {
            for (const auto& node : message.nodes()) {
                ProcessNode(node, experiment_run_id);
            }

            break;
        }

        // For any leaf nodes insert into the DB
        case NodeManager::Node::HARDWARE_NODE:
        case NodeManager::Node::DOCKER_NODE:
        case NodeManager::Node::OPEN_CL: {
            std::string hostname = message.info().name();
            std::cout << hostname << std::endl;
            std::string ip = NodeManager::GetAttribute(message.attributes(), "ip_address").s(0);
            std::cout << "ip address " << ip <<std::endl; 

            // NOTE: at the moment all nodes live on their own machine
            int machine_id = database_->InsertValues(
                "Machine",
                {"ExperimentRunID"},
                {std::to_string(experiment_run_id)}
            );

            int node_id = database_->InsertValuesUnique(
                "Node",
                {"ExperimentRunID", "MachineID", "IP", "Hostname"},
                {std::to_string(experiment_run_id), std::to_string(machine_id), ip, hostname},
                {"IP", "ExperimentRunID"}
            );

            for (const auto& component : message.components()) {
                ProcessComponent(component, experiment_run_id, node_id);
            }

            break;
        }
        
        default:
            std::cerr << "Encountered unknown node type " << NodeManager::Node::NodeType_Name(message.type()) << std::endl;
    }

    //message.info().type();
}

void NodeManagerProtoHandler::ProcessComponent(const NodeManager::Component& message, int experiment_run_id, int node_id) {
    if (message.location_size() != message.replicate_indices_size()) {
            // NOTE: sizes dont match
            throw std::runtime_error(std::string("Mismatch in size of replication and location vectors for component ").append(message.info().name()));
    }

    int component_id = database_->InsertValuesUnique(
        "Component",
        {"Name", "ExperimentRunID"},
        {message.info().type(), std::to_string(experiment_run_id)},
        {"Name", "ExperimentRunID"}
    );

    std::string full_location;
    /*for (const std::string& str : message.location()) {
        full_location.append(str).append("/");
    }*/
    /*std::transform(
        message.location().begin(), message.location().end(),
        message.replicate_indices().begin(), message.replicate_indices().end(),
        [&full_location](const std::string& str, int rep_index) {
            full_location.append(str).append("_").append(std::to_string(rep_index));
            return true;
        }
    );*/
    /*auto loc_iter = message.location().begin();
    auto rep_iter = message.replicate_indices().begin();
    while (loc_iter != message.location().end() || rep_iter != message.replicate_indices().end()) {
        full_location.append(*loc_iter).append("_").append(std::to_string(*rep_iter));
        loc_iter++;
        rep_iter++;
    }*/
    auto&& location_vec = std::vector<std::string>(message.location().begin(), message.location().end());
    auto&& replication_vec = std::vector<int>(message.replicate_indices().begin(), message.replicate_indices().end());
    full_location = AggServer::GetFullLocation(location_vec, replication_vec);

    int component_instance_id = database_->InsertValuesUnique(
        "ComponentInstance",
        {"ComponentID", "Path", "Name", "NodeID"},
        {std::to_string(component_id), full_location, message.info().name(), std::to_string(node_id)},
        {"Path", "ComponentID"} // Unique path per componentID -> Unique path per ExperimentRunID
    );

    for(const auto& port : message.ports()) {
        ProcessPort(port, component_instance_id, full_location);
    }
    
    for(const auto& worker : message.workers()) {
        ProcessWorker(worker, experiment_run_id, component_instance_id, full_location);
    }
}

void NodeManagerProtoHandler::ProcessPort(const NodeManager::Port& message, int component_instance_id, const std::string& component_instance_location) {

    //const std::string&& location = GetFullLocation(message.location(), message.replication_indices());
    const std::string port_path = component_instance_location + "/" + message.info().name();
    const std::string& port_kind = NodeManager::Port_Kind_Name(message.kind());
    const std::string& port_type = message.info().type();
    const std::string& middleware = NodeManager::Middleware_Name(message.middleware());

    int port_id = database_->InsertValuesUnique(
        "Port",
        {"Name", "ComponentInstanceID", "Path", "Kind", "Type", "Middleware"},
        {message.info().name(), std::to_string(component_instance_id), port_path, port_kind, port_type, middleware},
        {"Name", "ComponentInstanceID"}
    );
}

void NodeManagerProtoHandler::ProcessWorker(const NodeManager::Worker& message, int experiment_run_id, int component_id, const std::string& component_path) {

    int worker_id = database_->InsertValuesUnique(
        "Worker",
        {"Name", "ExperimentRunID"},
        {message.info().type(), std::to_string(experiment_run_id)},
        {"Name"}
    );

    std::string worker_path = component_path + "/" + message.info().name();

    int worker_instance_id = database_->InsertValuesUnique(
        "WorkerInstance",
        {"Name", "WorkerID", "ComponentInstanceID", "Path"},
        {message.info().name(), std::to_string(worker_id), std::to_string(component_id), worker_path},
        {"Name", "ComponentInstanceID"}
    );
}
