/* logan
 * Copyright (C) 2016-2017 The University of Adelaide
 *
 * This file is part of "logan"
 *
 * "logan" is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * "logan" is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "modelprotohandler.h"

#include <functional>
#include <chrono>

#include "../sqlitedatabase.h"
#include "../table.h"
#include "../tableinsert.h"

#include "../../re_common/proto/modelevent/modelevent.pb.h"
#include "../../re_common/zmq/protoreceiver/protoreceiver.h"

//Types
#define LOGAN_DECIMAL "DECIMAL"
#define LOGAN_VARCHAR "VARCHAR"
#define LOGAN_INT "INTEGER"

//Common column names
#define LOGAN_TIMEOFDAY "timeofday"
#define LOGAN_HOSTNAME "hostname"
#define LOGAN_COMPONENT_NAME "component_name"
#define LOGAN_COMPONENT_ID "component_id"
#define LOGAN_COMPONENT_TYPE "component_type"
#define LOGAN_PORT_NAME "port_name"
#define LOGAN_PORT_ID "port_id"
#define LOGAN_PORT_KIND "port_kind"
#define LOGAN_PORT_TYPE "port_type"
#define LOGAN_PORT_MIDDLEWARE "port_middleware"
#define LOGAN_WORKER_NAME "worker_name"
#define LOGAN_WORKER_TYPE "worker_type"
#define LOGAN_EVENT "event"
#define LOGAN_MESSAGE_ID "id"
#define LOGAN_NAME "name"
#define LOGAN_TYPE "type"

//Model Table names
#define LOGAN_LIFECYCLE_PORT_TABLE "Model_Lifecycle_EventPort"
#define LOGAN_LIFECYCLE_COMPONENT_TABLE "Model_Lifecycle_Component"
#define LOGAN_EVENT_USER_TABLE "Model_Event_User"
#define LOGAN_EVENT_WORKLOAD_TABLE "Model_Event_Workload"
#define LOGAN_EVENT_COMPONENT_TABLE "Model_Event_Component"

ModelProtoHandler::ModelProtoHandler() : ProtoHandler(){}
ModelProtoHandler::~ModelProtoHandler(){}

void ModelProtoHandler::ConstructTables(SQLiteDatabase* database){
    database_ = database;
    CreatePortEventTable();
    CreateComponentEventTable();
    CreateUserEventTable();
    CreateWorkloadEventTable();
    CreateComponentUtilizationTable();
}

void ModelProtoHandler::BindCallbacks(zmq::ProtoReceiver* receiver){
    auto ue_callback = std::bind(&ModelProtoHandler::ProcessUserEvent, this, std::placeholders::_1);
    receiver->RegisterNewProto(re_common::UserEvent::default_instance(), ue_callback);

    auto le_callback = std::bind(&ModelProtoHandler::ProcessLifecycleEvent, this, std::placeholders::_1);
    receiver->RegisterNewProto(re_common::LifecycleEvent::default_instance(), le_callback);

    auto we_callback = std::bind(&ModelProtoHandler::ProcessWorkloadEvent, this, std::placeholders::_1);
    receiver->RegisterNewProto(re_common::WorkloadEvent::default_instance(), we_callback);

    auto cu_callback = std::bind(&ModelProtoHandler::ProcessComponentUtilizationEvent, this, std::placeholders::_1);
    receiver->RegisterNewProto(re_common::ComponentUtilizationEvent::default_instance(), cu_callback);
}


void ModelProtoHandler::CreatePortEventTable(){
    if(table_map_.count(LOGAN_LIFECYCLE_PORT_TABLE)){
        return;
    }

    Table* t = new Table(database_, LOGAN_LIFECYCLE_PORT_TABLE);
    t->AddColumn(LOGAN_TIMEOFDAY, LOGAN_DECIMAL);
    t->AddColumn(LOGAN_HOSTNAME, LOGAN_VARCHAR);
    t->AddColumn(LOGAN_COMPONENT_NAME, LOGAN_VARCHAR);
    t->AddColumn(LOGAN_COMPONENT_ID, LOGAN_VARCHAR);
    t->AddColumn(LOGAN_COMPONENT_TYPE, LOGAN_VARCHAR);
    t->AddColumn(LOGAN_PORT_NAME, LOGAN_VARCHAR);
    t->AddColumn(LOGAN_PORT_ID, LOGAN_VARCHAR);
    t->AddColumn(LOGAN_PORT_KIND, LOGAN_VARCHAR);
    t->AddColumn(LOGAN_PORT_TYPE, LOGAN_VARCHAR);
    t->AddColumn(LOGAN_PORT_MIDDLEWARE, LOGAN_VARCHAR);
    t->AddColumn(LOGAN_EVENT, LOGAN_VARCHAR);
    t->Finalize();

    table_map_[LOGAN_LIFECYCLE_PORT_TABLE] = t;
    database_->QueueSqlStatement(t->get_table_construct_statement());
}

void ModelProtoHandler::CreateComponentEventTable(){
    if(table_map_.count(LOGAN_LIFECYCLE_COMPONENT_TABLE)){
        return;
    }

    Table* t = new Table(database_, LOGAN_LIFECYCLE_COMPONENT_TABLE);
    t->AddColumn(LOGAN_TIMEOFDAY, LOGAN_DECIMAL);
    t->AddColumn(LOGAN_HOSTNAME, LOGAN_VARCHAR);
    t->AddColumn(LOGAN_COMPONENT_NAME, LOGAN_VARCHAR);
    t->AddColumn(LOGAN_COMPONENT_ID, LOGAN_VARCHAR);
    t->AddColumn(LOGAN_COMPONENT_TYPE, LOGAN_VARCHAR);
    t->AddColumn(LOGAN_EVENT, LOGAN_VARCHAR);
    t->Finalize();

    table_map_[LOGAN_LIFECYCLE_COMPONENT_TABLE] = t;
    database_->QueueSqlStatement(t->get_table_construct_statement());
}

void ModelProtoHandler::CreateUserEventTable(){
    if(table_map_.count(LOGAN_EVENT_USER_TABLE)){
        return;
    }

    Table* t = new Table(database_, LOGAN_EVENT_USER_TABLE);
    t->AddColumn(LOGAN_TIMEOFDAY, LOGAN_DECIMAL);
    t->AddColumn(LOGAN_HOSTNAME, LOGAN_VARCHAR);
    t->AddColumn(LOGAN_COMPONENT_NAME, LOGAN_VARCHAR);
    t->AddColumn(LOGAN_COMPONENT_ID, LOGAN_VARCHAR);
    t->AddColumn(LOGAN_COMPONENT_TYPE, LOGAN_VARCHAR);
    t->AddColumn("message", LOGAN_VARCHAR);
    t->AddColumn(LOGAN_TYPE, LOGAN_VARCHAR);
    t->Finalize();

    table_map_[LOGAN_EVENT_USER_TABLE] = t;
    database_->QueueSqlStatement(t->get_table_construct_statement());
}

void ModelProtoHandler::CreateWorkloadEventTable(){
    if(table_map_.count(LOGAN_EVENT_WORKLOAD_TABLE)){
        return;
    }

    Table* t = new Table(database_, LOGAN_EVENT_WORKLOAD_TABLE);
    //Info
    t->AddColumn(LOGAN_TIMEOFDAY, LOGAN_DECIMAL);
    t->AddColumn(LOGAN_HOSTNAME, LOGAN_VARCHAR);
    //Component specific
    t->AddColumn(LOGAN_COMPONENT_NAME, LOGAN_VARCHAR);
    t->AddColumn(LOGAN_COMPONENT_ID, LOGAN_VARCHAR);
    t->AddColumn(LOGAN_COMPONENT_TYPE, LOGAN_VARCHAR);

    //Workload specific info
    t->AddColumn("workload_id", LOGAN_INT);
    t->AddColumn(LOGAN_WORKER_NAME, LOGAN_VARCHAR);
    t->AddColumn(LOGAN_WORKER_TYPE, LOGAN_VARCHAR);
    t->AddColumn("function", LOGAN_VARCHAR);
    t->AddColumn("event_type", LOGAN_VARCHAR);
    t->AddColumn("args", LOGAN_VARCHAR);

    t->Finalize();
    table_map_[LOGAN_EVENT_WORKLOAD_TABLE] = t;
    database_->QueueSqlStatement(t->get_table_construct_statement());
}

void ModelProtoHandler::CreateComponentUtilizationTable(){
    if(table_map_.count(LOGAN_EVENT_COMPONENT_TABLE)){
        return;
    }

    Table* t = new Table(database_, LOGAN_EVENT_COMPONENT_TABLE);
    //Info
    t->AddColumn(LOGAN_TIMEOFDAY, LOGAN_DECIMAL);
    t->AddColumn(LOGAN_HOSTNAME, LOGAN_VARCHAR);
    //Component specific
    t->AddColumn(LOGAN_COMPONENT_NAME, LOGAN_VARCHAR);
    t->AddColumn(LOGAN_COMPONENT_ID, LOGAN_VARCHAR);
    t->AddColumn(LOGAN_COMPONENT_TYPE, LOGAN_VARCHAR);
    //Port specific
    t->AddColumn(LOGAN_PORT_NAME, LOGAN_VARCHAR);
    t->AddColumn(LOGAN_PORT_ID, LOGAN_VARCHAR);
    t->AddColumn(LOGAN_PORT_KIND, LOGAN_VARCHAR);
    t->AddColumn(LOGAN_PORT_TYPE, LOGAN_VARCHAR);
    t->AddColumn(LOGAN_PORT_MIDDLEWARE, LOGAN_VARCHAR);

    t->AddColumn("port_event_id", LOGAN_INT);
    t->AddColumn(LOGAN_TYPE, LOGAN_VARCHAR);
    t->Finalize();
    table_map_[LOGAN_EVENT_COMPONENT_TABLE] = t;
    database_->QueueSqlStatement(t->get_table_construct_statement());
}

void ModelProtoHandler::ProcessLifecycleEvent(google::protobuf::MessageLite* message){
    re_common::LifecycleEvent* event = (re_common::LifecycleEvent*)message;
    if(event->has_port() && event->has_component()){
        //Process port event
        //Insert test Statements
        auto ins = table_map_[LOGAN_LIFECYCLE_PORT_TABLE]->get_insert_statement();
        ins.BindDouble(LOGAN_TIMEOFDAY, event->info().timestamp());
        ins.BindString(LOGAN_HOSTNAME, event->info().hostname());
        ins.BindString(LOGAN_COMPONENT_NAME, event->component().name());
        ins.BindString(LOGAN_COMPONENT_ID, event->component().id());
        ins.BindString(LOGAN_COMPONENT_TYPE, event->component().type());
        ins.BindString(LOGAN_PORT_NAME, event->port().name());
        ins.BindString(LOGAN_PORT_ID, event->port().id());
        ins.BindString(LOGAN_PORT_TYPE, event->port().type());
        ins.BindString(LOGAN_PORT_MIDDLEWARE, event->port().middleware());
        ins.BindString(LOGAN_EVENT, re_common::LifecycleEvent::Type_Name(event->type()));

        database_->QueueSqlStatement(ins.get_statement());
    }

    else if(event->has_component()){
            auto ins = table_map_[LOGAN_LIFECYCLE_COMPONENT_TABLE]->get_insert_statement();
            ins.BindDouble(LOGAN_TIMEOFDAY, event->info().timestamp());
            ins.BindString(LOGAN_HOSTNAME, event->info().hostname());
            ins.BindString(LOGAN_COMPONENT_NAME, event->component().name());
            ins.BindString(LOGAN_COMPONENT_ID, event->component().id());
            ins.BindString(LOGAN_COMPONENT_TYPE, event->component().type());
            ins.BindString(LOGAN_EVENT, re_common::LifecycleEvent::Type_Name(event->type()));
            database_->QueueSqlStatement(ins.get_statement());
    }
}

void ModelProtoHandler::ProcessUserEvent(google::protobuf::MessageLite* message){

    re_common::UserEvent* event = (re_common::UserEvent*)message;
    auto ins = table_map_[LOGAN_EVENT_USER_TABLE]->get_insert_statement();
    ins.BindDouble(LOGAN_TIMEOFDAY, event->info().timestamp());
    ins.BindString(LOGAN_HOSTNAME, event->info().hostname());
    ins.BindString(LOGAN_COMPONENT_NAME, event->component().name());
    ins.BindString(LOGAN_COMPONENT_ID, event->component().id());
    ins.BindString(LOGAN_COMPONENT_TYPE, event->component().type());
    ins.BindString("message", event->message());
    ins.BindString(LOGAN_TYPE, re_common::UserEvent::Type_Name(event->type()));
    database_->QueueSqlStatement(ins.get_statement());
}

void ModelProtoHandler::ProcessWorkloadEvent(google::protobuf::MessageLite* message){
    re_common::WorkloadEvent* event = (re_common::WorkloadEvent*)message;
    auto ins = table_map_[LOGAN_EVENT_WORKLOAD_TABLE]->get_insert_statement();
    //Info
    ins.BindDouble(LOGAN_TIMEOFDAY, event->info().timestamp());
    ins.BindString(LOGAN_HOSTNAME, event->info().hostname());

    //Component
    ins.BindString(LOGAN_COMPONENT_ID, event->component().id());
    ins.BindString(LOGAN_COMPONENT_NAME, event->component().name());
    ins.BindString(LOGAN_COMPONENT_TYPE, event->component().type());

    //Workload
    ins.BindString(LOGAN_WORKER_NAME, event->name());
    ins.BindInt("workload_id", event->id());
    ins.BindString(LOGAN_WORKER_TYPE, event->type());
    ins.BindString("function", event->function());
    ins.BindString("event_type", re_common::WorkloadEvent::Type_Name(event->event_type()));
    ins.BindString("args", event->args());
    database_->QueueSqlStatement(ins.get_statement());
}

void ModelProtoHandler::ProcessComponentUtilizationEvent(google::protobuf::MessageLite* message){
    re_common::ComponentUtilizationEvent* event = (re_common::ComponentUtilizationEvent*)message;
    auto ins = table_map_[LOGAN_EVENT_COMPONENT_TABLE]->get_insert_statement();

    //Info
    ins.BindDouble(LOGAN_TIMEOFDAY, event->info().timestamp());
    ins.BindString(LOGAN_HOSTNAME, event->info().hostname());

    //Component
    ins.BindString(LOGAN_COMPONENT_NAME, event->component().name());
    ins.BindString(LOGAN_COMPONENT_ID, event->component().id());
    ins.BindString(LOGAN_COMPONENT_TYPE, event->component().type());

    //Port
    ins.BindString(LOGAN_PORT_NAME, event->port().name());
    ins.BindString(LOGAN_PORT_ID, event->port().id());
    ins.BindString(LOGAN_PORT_KIND, re_common::Port::Kind_Name(event->port().kind()));
    ins.BindString(LOGAN_PORT_TYPE, event->port().type());
    ins.BindString(LOGAN_PORT_MIDDLEWARE, event->port().middleware());

    ins.BindInt("port_event_id", event->port_event_id());
    ins.BindString(LOGAN_TYPE, re_common::ComponentUtilizationEvent::Type_Name(event->type()));

    database_->QueueSqlStatement(ins.get_statement());
}