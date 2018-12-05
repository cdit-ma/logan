#include "modeleventprotohandler.h"

#include "utils.h"

#include "databaseclient.h"
#include "experimenttracker.h"

#include <zmq/protoreceiver/protoreceiver.h>
#include <proto/modelevent/modelevent.pb.h>

#include <google/protobuf/util/time_util.h>

#include <chrono>

#include <functional>

using google::protobuf::util::TimeUtil;

void ModelEventProtoHandler::BindCallbacks(zmq::ProtoReceiver& receiver) {    
    receiver.RegisterProtoCallback<ModelEvent::LifecycleEvent>(std::bind(&ModelEventProtoHandler::ProcessLifecycleEvent, this, std::placeholders::_1));
    receiver.RegisterProtoCallback<ModelEvent::WorkloadEvent>(std::bind(&ModelEventProtoHandler::ProcessWorkloadEvent, this, std::placeholders::_1));
    receiver.RegisterProtoCallback<ModelEvent::UtilizationEvent>(std::bind(&ModelEventProtoHandler::ProcessUtilizationEvent, this, std::placeholders::_1));
}


/*void ModelEventProtoHandler::ProcessUserEvent(const ModelEvent::UserEvent& message){

    int component_instance_id = GetComponentInstanceID(message.component(), message.info().experiment_name());

    std::string type = ModelEvent::UserEvent::Type_Name(message.type());
    std::string sample_time = AggServer::FormatTimestamp(message.info().timestamp());

    database_->InsertValues(
        "UserEvent",
        {"ComponentInstanceID", "Message", "Type", "SampleTime"},
        {std::to_string(component_instance_id), message.message(), type, sample_time}
    );
}*/

void ModelEventProtoHandler::ProcessLifecycleEvent(const ModelEvent::LifecycleEvent& message){
    std::cerr << "InsertPortLifecycleEvent" << std::endl;
    if (message.has_component()) {
        if (message.has_port()) {
            try {
                InsertPortLifecycleEvent(message.info(), message.type(), message.component(), message.port());
            } catch (const std::exception& e) {
                std::cerr << "An exception occured while trying to insert a PortLifecycleEvent: " << e.what() << std::endl;
            }
        } else {
            InsertComponentLifecycleEvent(message.info(), message.type(), message.component());
        }
    }
}


void ModelEventProtoHandler::ProcessWorkloadEvent(const ModelEvent::WorkloadEvent& message) {
    //std::cerr << "ProcessWorkloadEvent" << std::endl;

    std::string worker_instance_id = std::to_string(GetWorkerInstanceID(message.component(), message.worker()));
    std::string function = message.function_name();
    std::string workload_id = std::to_string(message.workload_id());
    std::string type = ModelEvent::WorkloadEvent::Type_Name(message.event_type());
    std::string args = message.args();
    std::string log_level = std::to_string(message.log_level());
    std::string sample_time = TimeUtil::ToString(message.info().timestamp());

    database_->InsertValues(
        "WorkloadEvent",
        {"WorkerInstanceID", "WorkloadID", "Function", "Type", "Arguments", "LogLevel", "SampleTime"},
        {worker_instance_id, workload_id, function, type, args, log_level, sample_time}
    );
}

void ModelEventProtoHandler::ProcessUtilizationEvent(const ModelEvent::UtilizationEvent& message) {
    //std::cerr << "ProcessUtilizationEvent\n";

    auto start = std::chrono::steady_clock::now();

    std::string port_id = std::to_string(GetPortID(message.port(), message.component()));

    auto port_id_aquired_time = std::chrono::steady_clock::now();

    std::string seq_num = std::to_string(message.port_event_id());
    std::string type = ModelEvent::UtilizationEvent::Type_Name(message.type());
    std::string msg = message.message();
    std::string sample_time = TimeUtil::ToString(message.info().timestamp());

    database_->InsertValues(
        "PortEvent",
        {"PortID", "PortEventSequenceNum", "Type", "Message", "SampleTime"},
        {port_id, seq_num, type, msg, sample_time}
    );

    auto finish = std::chrono::steady_clock::now();
    auto id_delay = std::chrono::duration_cast<std::chrono::microseconds>(port_id_aquired_time - start);
    auto total_delay = std::chrono::duration_cast<std::chrono::microseconds>(finish - start);

    std::cout << "PortEvent delay = " << total_delay.count() << " (" << id_delay.count() << " spent fetching port_id)" << std::endl;
}

void ModelEventProtoHandler::InsertComponentLifecycleEvent(const ModelEvent::Info& info,
                const ModelEvent::LifecycleEvent_Type& type,
                const ModelEvent::Component& component) {
                
    std::vector<std::string> columns = {
        "ComponentInstanceID",
        "SampleTime",
        "Type"
    };

    std::vector<std::string> values;
    values.emplace_back(std::to_string(GetComponentInstanceID(component)));
    //values.emplace_back(AggServer::FormatTimestamp(info.timestamp()));
    values.emplace_back(TimeUtil::ToString(info.timestamp()));
    values.emplace_back(std::to_string(type));

    database_->InsertValues("ComponentLifecycleEvent", columns, values);
}

void ModelEventProtoHandler::InsertPortLifecycleEvent(const ModelEvent::Info& info,
                const ModelEvent::LifecycleEvent_Type& type,
                const ModelEvent::Component& component,
                const ModelEvent::Port& port) {

    

    std::vector<std::string> columns = {
        "PortID",
        "SampleTime",
        "Type"
    };

    std::vector<std::string> values;
    values.emplace_back(std::to_string(GetPortID(port, component)));
    //values.emplace_back(database_->EscapeString(AggServer::FormatTimestamp(info.timestamp())));
    values.emplace_back(TimeUtil::ToString(info.timestamp()));
    values.emplace_back(std::to_string(type));

    database_->InsertValues("PortLifecycleEvent", columns, values);
}



int ModelEventProtoHandler::GetComponentInstanceID(const ModelEvent::Component& component_instance) {
    try {
        return component_inst_id_cache_.at(component_instance.id());
    } catch (const std::out_of_range& oor_ex) {
        int component_id = GetComponentID(component_instance.type());

        std::string&& comp_id = database_->EscapeString(std::to_string(component_id));
        std::string&& graphml_id = database_->EscapeString(component_instance.id()); // Graphml ID, not database row ID

        std::stringstream condition_stream;
        condition_stream << "ComponentID = " << comp_id << " AND GraphmlID = " << graphml_id;

        int component_inst_id = database_->GetID("ComponentInstance", condition_stream.str());
        component_inst_id_cache_.emplace(std::make_pair(component_instance.id(), component_inst_id));
        return component_inst_id;
    }
}


int ModelEventProtoHandler::GetPortID(const ModelEvent::Port& port, const ModelEvent::Component& component) {
    try {
        return port_id_cache_.at(port.id());
    } catch (const std::out_of_range& oor_ex) {
        int component_instance_id = GetComponentInstanceID(component);
        
        std::string&& comp_inst_id = database_->EscapeString(std::to_string(component_instance_id));
        std::string&& name = database_->EscapeString(port.name());

        std::stringstream condition_stream;
        condition_stream << "ComponentInstanceID = " << comp_inst_id << " AND Name = " << name;

        int port_id = database_->GetID("Port", condition_stream.str());
        port_id_cache_.emplace(std::make_pair(port.id(), port_id));
        return port_id;
    }
}

int ModelEventProtoHandler::GetWorkerInstanceID(const ModelEvent::Component& component, const ModelEvent::Worker& worker_instance) {
    try {
        return worker_inst_id_cache_.at(worker_instance.id());
    } catch (const std::out_of_range& oor_ex) {
        int component_instance_id = GetComponentInstanceID(component);

        std::string&& comp_inst_id = database_->EscapeString(std::to_string(component_instance_id));
        std::string&& name = database_->EscapeString(worker_instance.name());

        std::stringstream condition_stream;
        condition_stream << "ComponentInstanceID = " << comp_inst_id << " AND Name = " << name;

        int worker_inst_id = database_->GetID("WorkerInstance", condition_stream.str());
        worker_inst_id_cache_.emplace(std::make_pair(worker_instance.id(), worker_inst_id));
        return worker_inst_id;
    }
}

int ModelEventProtoHandler::GetComponentID(const std::string& name) {
    try {
        return component_id_cache_.at(name);
    } catch (const std::out_of_range& oor_ex) {
        std::stringstream condition_stream;
        condition_stream << "Name = " << database_->EscapeString(name) << " AND ExperimentRunID = " << experiment_run_id_;

        int component_id = database_->GetID("Component", condition_stream.str());
        component_id_cache_.emplace(std::make_pair(name, component_id));
        return component_id;
    }
}
