#include "aggregationprotohandler.h"

#include <algorithm>
#include <iostream>

#include <google/protobuf/message_lite.h>
#include <re_common/proto/modelevent/modelevent.pb.h>
#include <re_common/zmq/protoreceiver/protoreceiver.h>

#include <re_common/proto/controlmessage/helper.h>

#include "utils.h"

#include "databaseclient.h"
#include "experimenttracker.h"

AggregationProtoHandler::AggregationProtoHandler(std::shared_ptr<DatabaseClient> db_client, std::shared_ptr<ExperimentTracker> exp_tracker) :
    database_(db_client), experiment_tracker_(exp_tracker) {};


void AggregationProtoHandler::BindCallbacks(zmq::ProtoReceiver& receiver) {
    receiver.RegisterProtoCallback<re_common::UserEvent>(std::bind(&AggregationProtoHandler::ProcessUserEvent, this, std::placeholders::_1));
    receiver.RegisterProtoCallback<re_common::LifecycleEvent>(std::bind(&AggregationProtoHandler::ProcessLifecycleEvent, this, std::placeholders::_1));
    //receiver.RegisterProtoCallback<re_common::WorkloadEvent>(std::bind(&AggregationProtoHandler::ProcessWorkloadEvent, this, std::placeholders::_1));
    //receiver.RegisterProtoCallback<re_common::ComponentUtilizationEvent>(std::bind(&AggregationProtoHandler::ProcessComponentUtilizationEvent, this, std::placeholders::_1));

    receiver.RegisterProtoCallback<NodeManager::ControlMessage>(std::bind(&AggregationProtoHandler::ProcessControlMessage, this, std::placeholders::_1));
}

void AggregationProtoHandler::ProcessEnvironmentMessage(const NodeManager::EnvironmentMessage& message) {
    switch (message.type()) {
        case NodeManager::EnvironmentMessage::UPDATE_DEPLOYMENT:    // For the moment we expect
        {
            ProcessControlMessage(message.control_message());
            return;
        }
    }
}

void AggregationProtoHandler::ProcessControlMessage(const NodeManager::ControlMessage& message) {
    std::cout << "Processing control message" << std::endl;
    //return;

    switch(message.type()) {
        default: {      // NOTE: For the moment we treat all message equally
            int exp_run_id = experiment_tracker_->RegisterExperimentRun(message.experiment_id(), message.time_stamp());
            //message.host_name();    // NOTE: Currently not using host name for anything

            for (const auto& node : message.nodes()) {
                ProcessNode(node, exp_run_id);
            }
        }
    }
}

void AggregationProtoHandler::ProcessNode(const NodeManager::Node& message, int experiment_run_id) {
    std::cout << "Processing node" << std::endl;

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

            // NOTE: at the moment all nodes occur on their own
            int machine_id = database_->InsertValues(
                "Machine",
                {"ExperimentRunID"},
                {std::to_string(experiment_run_id)}
            );

            std::cout << "Added a new machine!!" << std::endl;

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

void AggregationProtoHandler::ProcessComponent(const NodeManager::Component& message, int experiment_run_id, int node_id) {
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
    for (const std::string& str : message.location()) {
        full_location.append(str).append("/");
    }
    /*std::transform(
        message.location().begin(), message.location().end(),
        message.replicate_indices().begin(), message.replicate_indices().end(),
        [&full_location](const std::string& str, int rep_index) {
            full_location.append(str).append("_").append(std::to_string(rep_index));
            return true;
        }
    );*/
    auto loc_iter = message.location().begin();
    auto rep_iter = message.replicate_indices().begin();
    while (loc_iter != message.location().end() || rep_iter != message.replicate_indices().end()) {
        full_location.append(*loc_iter).append("_").append(std::to_string(*rep_iter));
        loc_iter++;
        rep_iter++;
    }

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

void AggregationProtoHandler::ProcessPort(const NodeManager::Port& message, int component_instance_id, const std::string& component_instance_location) {

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

void AggregationProtoHandler::ProcessWorker(const NodeManager::Worker& message, int experiment_run_id, int component_id, const std::string& component_path) {

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

void AggregationProtoHandler::ProcessUserEvent(const re_common::UserEvent& message){
    std::cerr << "GOT MESSAGE WITH CONTENT: " << message.message() << std::endl;

    std::vector< std::pair<std::string,std::string> > cols = {std::make_pair("TestCol","INT")};
    database_->CreateTable(message.message(), cols);
}

void AggregationProtoHandler::ProcessLifecycleEvent(const re_common::LifecycleEvent& message){
    /*auto type = message.type();
    auto time = message.info().timestamp();
    auto lifecycle_component_instance = message.component();

    int comp_inst_id = GetComponentInstanceID(lifecycle_component_instance, message.info().experiment_name());
    */
    if (message.has_component()) {
        if (message.has_port()) {
            InsertPortLifecycleEvent(message.info(), message.type(), message.component(), message.port());
        } else {
            InsertComponentLifecycleEvent(message.info(), message.type(), message.component());
        }
    }
}

void AggregationProtoHandler::InsertComponentLifecycleEvent(const re_common::Info& info,
                const re_common::LifecycleEvent_Type& type,
                const re_common::Component& component) {

    std::vector<std::string> columns = {
        "ComponentInstanceID",
        "TimeStamp",
        "Type"
    };

    std::vector<std::string> values;
    values.emplace_back(std::to_string(GetComponentInstanceID(component, info.experiment_name())));
    values.emplace_back(AggServer::FormatTimestamp(info.timestamp()));
    values.emplace_back(std::to_string(type));

    database_->InsertValues("ComponentLifecycleEvent", columns, values);
}

void AggregationProtoHandler::InsertPortLifecycleEvent(const re_common::Info& info,
                const re_common::LifecycleEvent_Type& type,
                const re_common::Component& component,
                const re_common::Port& port) {

    std::vector<std::string> columns = {
        "PortID",
        "TimeStamp",
        "Type"
    };

    std::vector<std::string> values;
    values.emplace_back(std::to_string(GetPortID(port, component, info.experiment_name())));
    values.emplace_back(AggServer::FormatTimestamp(info.timestamp()));
    values.emplace_back(std::to_string(type));

    database_->InsertValues("PortLifecycleEvent", columns, values);
}



int AggregationProtoHandler::GetComponentInstanceID(const re_common::Component& component_instance, const std::string& experiment_name) {

    int component_id = GetComponentID(component_instance.type(), experiment_name);

    std::stringstream condition_stream;
    condition_stream << "ComponentID = " << component_id << ", Name = " << component_instance.name();

    database_->GetValues("ComponentInstance", {"ComponentInstanceID"}, condition_stream.str());
}


int AggregationProtoHandler::GetPortID(const re_common::Port& port, const re_common::Component& component, const std::string& experiment_name) {

    int component_instance_id = GetComponentInstanceID(component, experiment_name);

    std::stringstream condition_stream;
    condition_stream << "ComponentInstanceID = " << component_instance_id << ", Name = " << port.name();

    database_->GetValues("ComponentInstance", {"ComponentInstanceID"}, condition_stream.str());
}

int AggregationProtoHandler::GetComponentID(const std::string& name, const std::string& experiment_name) {
    std::stringstream condition_stream;
    condition_stream << "Name = " << name << ", ExperimentRunID = (SELECT ExperimentRunID FROM ExperimentRun WHERE (SELECT ExperimentID FROM Experiment WHERE Name = " << experiment_name << " ));";

    database_->GetValues("Component", {"ComponentID"}, condition_stream.str());
}
