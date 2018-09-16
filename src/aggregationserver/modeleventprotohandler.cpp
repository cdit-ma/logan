#include "modeleventprotohandler.h"

#include "utils.h"

#include "databaseclient.h"
#include "experimenttracker.h"

#include <re_common/zmq/protoreceiver/protoreceiver.h>
#include <re_common/proto/modelevent/modelevent.pb.h>

#include <functional>


void ModelEventProtoHandler::BindCallbacks(zmq::ProtoReceiver& receiver) {
    receiver.RegisterProtoCallback<re_common::UserEvent>(std::bind(&ModelEventProtoHandler::ProcessUserEvent, this, std::placeholders::_1));
    receiver.RegisterProtoCallback<re_common::LifecycleEvent>(std::bind(&ModelEventProtoHandler::ProcessLifecycleEvent, this, std::placeholders::_1));
    receiver.RegisterProtoCallback<re_common::WorkloadEvent>(std::bind(&ModelEventProtoHandler::ProcessWorkloadEvent, this, std::placeholders::_1));
    receiver.RegisterProtoCallback<re_common::ComponentUtilizationEvent>(std::bind(&ModelEventProtoHandler::ProcessComponentUtilizationEvent, this, std::placeholders::_1));
}


void ModelEventProtoHandler::ProcessUserEvent(const re_common::UserEvent& message){

    int component_instance_id = GetComponentInstanceID(message.component(), message.info().experiment_name());

    std::string type = re_common::UserEvent::Type_Name(message.type());
    std::string sample_time = AggServer::FormatTimestamp(message.info().timestamp());

    database_->InsertValues(
        "UserEvent",
        {"ComponentInstanceID", "Message", "Type", "SampleTime"},
        {std::to_string(component_instance_id), message.message(), type, sample_time}
    );
}

void ModelEventProtoHandler::ProcessLifecycleEvent(const re_common::LifecycleEvent& message){
    if (message.has_component()) {
        if (message.has_port()) {
            InsertPortLifecycleEvent(message.info(), message.type(), message.component(), message.port());
        } else {
            InsertComponentLifecycleEvent(message.info(), message.type(), message.component());
        }
    }
}


void ModelEventProtoHandler::ProcessWorkloadEvent(const re_common::WorkloadEvent& message) {
    std::string worker_instance_id = std::to_string(GetWorkerInstanceID(message.component(), message.name(), message.info().experiment_name()));
    std::string function = message.function();
    std::string type = re_common::WorkloadEvent::Type_Name(message.event_type());
    std::string args = message.args();
    std::string sample_time = AggServer::FormatTimestamp(message.info().timestamp());

    database_->InsertValues(
        "WorkloadEvent",
        {"WorkerInstanceID", "Function", "Type", "Arguments", "SampleTime"},
        {worker_instance_id, function, type, args, sample_time}
    );
}

void ModelEventProtoHandler::ProcessComponentUtilizationEvent(const re_common::ComponentUtilizationEvent& message) {
    std::string port_id = std::to_string(GetPortID(message.port(), message.component(), message.info().experiment_name()));
    std::string seq_num = std::to_string(message.port_event_id());
    std::string type = re_common::ComponentUtilizationEvent::Type_Name(message.type());
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
        "TimeStamp",
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
        "TimeStamp",
        "Type"
    };

    std::vector<std::string> values;
    values.emplace_back(std::to_string(GetPortID(port, component, info.experiment_name())));
    values.emplace_back(AggServer::FormatTimestamp(info.timestamp()));
    values.emplace_back(std::to_string(type));

    database_->InsertValues("PortLifecycleEvent", columns, values);
}



int ModelEventProtoHandler::GetComponentInstanceID(const re_common::Component& component_instance, const std::string& experiment_name) {
    int component_id = GetComponentID(component_instance.type(), experiment_name);

    std::stringstream condition_stream;
    condition_stream << "ComponentID = " << component_id << ", Name = " << component_instance.name();

    database_->GetValues("ComponentInstance", {"ComponentInstanceID"}, condition_stream.str());
}


int ModelEventProtoHandler::GetPortID(const re_common::Port& port, const re_common::Component& component, const std::string& experiment_name) {

    int component_instance_id = GetComponentInstanceID(component, experiment_name);

    std::stringstream condition_stream;
    condition_stream << "ComponentInstanceID = " << component_instance_id << ", Name = " << port.name();

    database_->GetValues("Port", {"PortID"}, condition_stream.str());
}

int ModelEventProtoHandler::GetWorkerInstanceID(const re_common::Component& component, const std::string& worker_name, const std::string& experiment_name) {

    int component_instance_id = GetComponentInstanceID(component, experiment_name);

    std::stringstream condition_stream;
    condition_stream << "ComponentInstanceID = " << component_instance_id << ", Name = " << worker_name;

    database_->GetValues("WorkerInstance", {"WorkerInstanceID"}, condition_stream.str());
}

int ModelEventProtoHandler::GetComponentID(const std::string& name, const std::string& experiment_name) {
    std::stringstream condition_stream;
    condition_stream << "Name = " << name << ", ExperimentRunID = (SELECT ExperimentRunID FROM ExperimentRun WHERE (SELECT ExperimentID FROM Experiment WHERE Name = " << experiment_name << " ));";

    database_->GetValues("Component", {"ComponentID"}, condition_stream.str());
}
