#include "aggregationprotohandler.h"

#include <functional>
#include <iostream>

#include <google/protobuf/message_lite.h>
#include <re_common/proto/modelevent/modelevent.pb.h>
#include <re_common/zmq/protoreceiver/protoreceiver.h>

#include "databaseclient.h"

AggregationProtoHandler::AggregationProtoHandler(DatabaseClient& db_client) :
    database_(db_client) {};


void AggregationProtoHandler::BindCallbacks(zmq::ProtoReceiver& receiver) {
    receiver.RegisterProtoCallback<re_common::UserEvent>(std::bind(&AggregationProtoHandler::ProcessUserEvent, this, std::placeholders::_1));
    receiver.RegisterProtoCallback<re_common::LifecycleEvent>(std::bind(&AggregationProtoHandler::ProcessLifecycleEvent, this, std::placeholders::_1));
    //receiver.RegisterProtoCallback<re_common::WorkloadEvent>(std::bind(&AggregationProtoHandler::ProcessWorkloadEvent, this, std::placeholders::_1));
    //receiver.RegisterProtoCallback<re_common::ComponentUtilizationEvent>(std::bind(&AggregationProtoHandler::ProcessComponentUtilizationEvent, this, std::placeholders::_1));
}


void AggregationProtoHandler::ProcessUserEvent(const re_common::UserEvent& message){
    std::cerr << "GOT MESSAGE LA FAMILIA" << message.message() << std::endl;

    std::vector< std::pair<std::string,std::string> > cols = {std::make_pair("TestCol","INT")};
    database_.CreateTable(message.message(), cols);
}

void AggregationProtoHandler::ProcessLifecycleEvent(const re_common::LifecycleEvent& message){
    auto type = message.type();
    auto time = message.info().timestamp();
    auto lifecycle_component_instance = message.component();

    int comp_inst_id = GetComponentInstanceID(lifecycle_component_instance);

    InsertComponentLifecycleEvent(comp_inst_id, type, time);
}

void AggregationProtoHandler::InsertComponentLifecycleEvent(int component_instance_id, re_common::LifecycleEvent_Type& type, double timestamp) {
    std::vector<std::string> columns = {
        "ComponentInstanceID",
        "TimeStamp",
        "Type"
    };

    std::vector<std::string> values;
    values.emplace_back(std::to_string(component_instance_id));
    values.emplace_back(std::to_string(timestamp));
    values.emplace_back(std::to_string(type));

    database_.InsertValues("ComponentLifecycleEvent", columns, values);
}


int AggregationProtoHandler::GetComponentInstanceID(const re_common::Component& component_instance) {

    int component_id = 0;//GetComponentID(component_instance.type(), component_instance.experiment());

    std::stringstream condition_stream;

    condition_stream << "ComponentID = " << component_id << ", ComponentTextID = " << component_instance.id();

    database_.GetValues("ComponentInstance", {"ComponentInstanceID"}, condition_stream.str());
}