#include "modeleventprotohandler.h"

#include "utils.h"

#include "databaseclient.h"
#include "experimenttracker.h"

#include <zmq/protoreceiver/protoreceiver.h>
#include <proto/modelevent/modelevent.pb.h>

#include <functional>


void ModelEventProtoHandler::BindCallbacks(zmq::ProtoReceiver& receiver) {
    //receiver.RegisterProtoCallback<re_common::UserEvent>(std::bind(&ModelEventProtoHandler::ProcessUserEvent, this, std::placeholders::_1));
    receiver.RegisterProtoCallback<re_common::LifecycleEvent>(std::bind(&ModelEventProtoHandler::ProcessLifecycleEvent, this, std::placeholders::_1));
    receiver.RegisterProtoCallback<re_common::WorkloadEvent>(std::bind(&ModelEventProtoHandler::ProcessWorkloadEvent, this, std::placeholders::_1));
    receiver.RegisterProtoCallback<re_common::UtilizationEvent>(std::bind(&ModelEventProtoHandler::ProcessUtilizationEvent, this, std::placeholders::_1));
}


/*void ModelEventProtoHandler::ProcessUserEvent(const re_common::UserEvent& message){

    int component_instance_id = GetComponentInstanceID(message.component(), message.info().experiment_name());

    std::string type = re_common::UserEvent::Type_Name(message.type());
    std::string sample_time = AggServer::FormatTimestamp(message.info().timestamp());

    database_->InsertValues(
        "UserEvent",
        {"ComponentInstanceID", "Message", "Type", "SampleTime"},
        {std::to_string(component_instance_id), message.message(), type, sample_time}
    );
}*/

void ModelEventProtoHandler::ProcessLifecycleEvent(const re_common::LifecycleEvent& message){
    if (message.has_component()) {
        if (message.has_port()) {
            try {
                InsertPortLifecycleEvent(message.info(), message.type(), message.component(), message.port());
            } catch (const std::exception& e) {
                std::cerr << e.what() << std::endl;
            }
        } else {
            InsertComponentLifecycleEvent(message.info(), message.type(), message.component());
        }
    }
}


void ModelEventProtoHandler::ProcessWorkloadEvent(const re_common::WorkloadEvent& message) {
    std::string worker_instance_id = std::to_string(GetWorkerInstanceID(message.component(), message.worker().name(), message.info().experiment_name()));
    std::string function = message.function_name();
    std::string type = re_common::WorkloadEvent::Type_Name(message.event_type());
    std::string args = message.args();
    std::string sample_time = AggServer::FormatTimestamp(message.info().timestamp());

    database_->InsertValues(
        "WorkloadEvent",
        {"WorkerInstanceID", "Function", "Type", "Arguments", "SampleTime"},
        {worker_instance_id, function, type, args, sample_time}
    );
}

void ModelEventProtoHandler::ProcessUtilizationEvent(const re_common::UtilizationEvent& message) {
    std::string port_id = std::to_string(GetPortID(message.port(), message.component(), message.info().experiment_name()));
    std::string seq_num = std::to_string(message.port_event_id());
    std::string type = re_common::UtilizationEvent::Type_Name(message.type());
    std::string sample_time = AggServer::FormatTimestamp(message.info().timestamp());

    database_->InsertValues(
        "PortEvent",
        {"PortID", "PortEventSequenceNum", "Type", "SampleTime"},
        {port_id, seq_num, type, sample_time}
    );
}

void ModelEventProtoHandler::InsertComponentLifecycleEvent(const re_common::Info& info,
                const re_common::LifecycleEvent_Type& type,
                const re_common::Component& component) {

    std::vector<std::string> columns = {
        "ComponentInstanceID",
        "SampleTime",
        "Type"
    };

    std::vector<std::string> values;
    values.emplace_back(std::to_string(GetComponentInstanceID(component, info.experiment_name())));
    values.emplace_back(AggServer::FormatTimestamp(info.timestamp()));
    values.emplace_back(std::to_string(type));

    database_->InsertValues("ComponentLifecycleEvent", columns, values);
}

void ModelEventProtoHandler::InsertPortLifecycleEvent(const re_common::Info& info,
                const re_common::LifecycleEvent_Type& type,
                const re_common::Component& component,
                const re_common::Port& port) {

    std::vector<std::string> columns = {
        "PortID",
        "SampleTime",
        "Type"
    };

    std::vector<std::string> values;
    values.emplace_back(std::to_string(GetPortID(port, component, info.experiment_name())));
    values.emplace_back(database_->EscapeString(AggServer::FormatTimestamp(info.timestamp())));
    values.emplace_back(std::to_string(type));

    database_->InsertValues("PortLifecycleEvent", columns, values);
}



int ModelEventProtoHandler::GetComponentInstanceID(const re_common::Component& component_instance, const std::string& experiment_name) {
    int component_id = GetComponentID(component_instance.type(), experiment_name);

    std::string&& comp_id = database_->EscapeString(std::to_string(component_id));
    std::string&& path = database_->EscapeString(component_instance.id()); // Needs to be updated to use the proper path

    std::stringstream condition_stream;
    condition_stream << "ComponentID = " << comp_id << " AND Path = " << path; 

    return database_->GetID("ComponentInstance", condition_stream.str());
}


int ModelEventProtoHandler::GetPortID(const re_common::Port& port, const re_common::Component& component, const std::string& experiment_name) {

    int component_instance_id = GetComponentInstanceID(component, experiment_name);
    
    std::string&& comp_inst_id = database_->EscapeString(std::to_string(component_instance_id));
    std::string&& name = database_->EscapeString(port.name());

    std::stringstream condition_stream;
    condition_stream << "ComponentInstanceID = " << comp_inst_id << " AND Name = " << name;

    return database_->GetID("Port", condition_stream.str());
}

int ModelEventProtoHandler::GetWorkerInstanceID(const re_common::Component& component, const std::string& worker_name, const std::string& experiment_name) {

    int component_instance_id = GetComponentInstanceID(component, experiment_name);

    std::string&& comp_inst_id = database_->EscapeString(std::to_string(component_instance_id));
    std::string&& name = database_->EscapeString(worker_name);

    std::stringstream condition_stream;
    condition_stream << "ComponentInstanceID = " << comp_inst_id << " AND Name = " << name;

    return database_->GetID("WorkerInstance", condition_stream.str());
}

int ModelEventProtoHandler::GetComponentID(const std::string& name, const std::string& experiment_name) {
    std::stringstream condition_stream;
    condition_stream <<
    "Name = " << database_->EscapeString(name) << " AND ExperimentRunID = (\
        SELECT ExperimentRunID \
        FROM ExperimentRun \
        WHERE ExperimentID = (\
            SELECT ExperimentID \
            FROM Experiment \
            WHERE Name = " << database_->EscapeString(experiment_name) << " \
        )\
    )";

    return database_->GetID("Component", condition_stream.str());
}
