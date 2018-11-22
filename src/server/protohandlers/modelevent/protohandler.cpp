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

#include "protohandler.h"

#include <functional>
#include <chrono>

#include <google/protobuf/util/time_util.h>

//Types
#define LOGAN_DECIMAL "DECIMAL"
#define LOGAN_VARCHAR "VARCHAR"
#define LOGAN_INT "INTEGER"

//Common column names
#define LOGAN_TIMEOFDAY "timeofday"
#define LOGAN_HOSTNAME "hostname"
#define LOGAN_MESSAGE_ID "id"
#define LOGAN_NAME "name"
#define LOGAN_TYPE "type"
#define LOGAN_MESSAGE "message"
#define LOGAN_EXPERIMENT_NAME "experiment_name"
#define LOGAN_CONTAINER_ID "container_id"
#define LOGAN_CONTAINER_NAME "container_name"

//Common column names
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
#define LOGAN_WORKER_ID "worker_id"
#define LOGAN_LOG_LEVEL "log_level"
#define LOGAN_EVENT "event"

//Model Table names
#define LOGAN_MODELEVENT_LIFECYCLE_TABLE "ModelEvents_Lifecycle"
#define LOGAN_MODELEVENT_WORKLOAD_TABLE "ModelEvents_Workload"
#define LOGAN_MODELEVENT_UTILIZATION_TABLE "ModelEvents_Utilization"

#define LOGAN_COL_INS(X) ":" X

ModelEvent::ProtoHandler::ProtoHandler(SQLiteDatabase& database):
    ::ProtoHandler(),
    database_(database)
{
    CreateLifecycleTable();
    CreateWorkloadTable();
    CreateUtilizationTable();
}

ModelEvent::ProtoHandler::~ProtoHandler(){
    std::lock_guard<std::mutex> lock(mutex_);
    std::cout << "* ModelEvent::ProtoHandler: Processed: " << rx_count_ << " Messages" << std::endl;
};

void ModelEvent::ProtoHandler::BindCallbacks(zmq::ProtoReceiver& receiver){
    receiver.RegisterProtoCallback<ModelEvent::LifecycleEvent>(std::bind(&ModelEvent::ProtoHandler::ProcessLifecycleEvent, this, std::placeholders::_1));
    receiver.RegisterProtoCallback<ModelEvent::WorkloadEvent>(std::bind(&ModelEvent::ProtoHandler::ProcessWorkloadEvent, this, std::placeholders::_1));
    receiver.RegisterProtoCallback<ModelEvent::UtilizationEvent>(std::bind(&ModelEvent::ProtoHandler::ProcessUtilizationEvent, this, std::placeholders::_1));
}

Table& ModelEvent::ProtoHandler::GetTable(const std::string& table_name){
    return *(tables_.at(table_name));
}

bool ModelEvent::ProtoHandler::GotTable(const std::string& table_name){
    try{
        GetTable(table_name);
        return true;
    }catch(const std::exception& ex){}
    return false;
}


void ModelEvent::ProtoHandler::AddInfoColumns(Table& table){
    table.AddColumn(LOGAN_TIMEOFDAY, LOGAN_VARCHAR);
    table.AddColumn(LOGAN_EXPERIMENT_NAME, LOGAN_VARCHAR);
    table.AddColumn(LOGAN_HOSTNAME, LOGAN_VARCHAR);
    table.AddColumn(LOGAN_CONTAINER_ID, LOGAN_VARCHAR);
    table.AddColumn(LOGAN_CONTAINER_NAME, LOGAN_VARCHAR);
}

void ModelEvent::ProtoHandler::AddComponentColumns(Table& table){
    table.AddColumn(LOGAN_COMPONENT_NAME, LOGAN_VARCHAR);
    table.AddColumn(LOGAN_COMPONENT_ID, LOGAN_VARCHAR);
    table.AddColumn(LOGAN_COMPONENT_TYPE, LOGAN_VARCHAR);
}

void ModelEvent::ProtoHandler::AddPortColumns(Table& table){
    table.AddColumn(LOGAN_PORT_NAME, LOGAN_VARCHAR);
    table.AddColumn(LOGAN_PORT_ID, LOGAN_VARCHAR);
    table.AddColumn(LOGAN_PORT_TYPE, LOGAN_VARCHAR);
    table.AddColumn(LOGAN_PORT_KIND, LOGAN_VARCHAR);
    table.AddColumn(LOGAN_PORT_MIDDLEWARE, LOGAN_VARCHAR);
}

void ModelEvent::ProtoHandler::AddWorkerColumns(Table& table){
    table.AddColumn(LOGAN_WORKER_NAME, LOGAN_VARCHAR);
    table.AddColumn(LOGAN_WORKER_ID, LOGAN_VARCHAR);
    table.AddColumn(LOGAN_WORKER_TYPE, LOGAN_VARCHAR);
}


void ModelEvent::ProtoHandler::CreateLifecycleTable(){
    if(GotTable(LOGAN_MODELEVENT_LIFECYCLE_TABLE)){
        return;
    }
    
    auto table_ptr = std::unique_ptr<Table>(new Table(database_, LOGAN_MODELEVENT_LIFECYCLE_TABLE));
    auto& table = *table_ptr;

    AddInfoColumns(table);
    AddComponentColumns(table);
    AddPortColumns(table);
    table.AddColumn(LOGAN_EVENT, LOGAN_VARCHAR);
    table.Finalize();

    tables_.emplace(std::make_pair(LOGAN_MODELEVENT_LIFECYCLE_TABLE, std::move(table_ptr)));

    //Queue the insert
    database_.ExecuteSqlStatement(table.get_table_construct_statement());
}

void ModelEvent::ProtoHandler::CreateWorkloadTable(){
    if(GotTable(LOGAN_MODELEVENT_WORKLOAD_TABLE)){
        return;
    }

    auto table_ptr = std::unique_ptr<Table>(new Table(database_, LOGAN_MODELEVENT_WORKLOAD_TABLE));
    auto& table = *table_ptr;

    //Info
    AddInfoColumns(table);
    AddComponentColumns(table);
    AddWorkerColumns(table);

    //Workload specific info
    table.AddColumn(LOGAN_TYPE, LOGAN_VARCHAR);
    table.AddColumn(LOGAN_LOG_LEVEL, LOGAN_INT);
    table.AddColumn("workload_id", LOGAN_INT);
    table.AddColumn("function_name", LOGAN_VARCHAR);
    table.AddColumn("args", LOGAN_VARCHAR);

    table.Finalize();

    tables_.emplace(std::make_pair(LOGAN_MODELEVENT_WORKLOAD_TABLE, std::move(table_ptr)));
    database_.ExecuteSqlStatement(table.get_table_construct_statement());
}

void ModelEvent::ProtoHandler::CreateUtilizationTable(){
    if(GotTable(LOGAN_MODELEVENT_UTILIZATION_TABLE)){
        return;
    }
    auto table_ptr = std::unique_ptr<Table>(new Table(database_, LOGAN_MODELEVENT_UTILIZATION_TABLE));
    auto& table = *table_ptr;
    AddInfoColumns(table);
    AddComponentColumns(table);
    AddPortColumns(table);

    table.AddColumn("port_event_id", LOGAN_INT);
    table.AddColumn(LOGAN_TYPE, LOGAN_VARCHAR);
    table.AddColumn(LOGAN_MESSAGE, LOGAN_VARCHAR);
    table.Finalize();

    tables_.emplace(std::make_pair(LOGAN_MODELEVENT_UTILIZATION_TABLE, std::move(table_ptr)));
    database_.ExecuteSqlStatement(table.get_table_construct_statement());
}

void ModelEvent::ProtoHandler::BindInfoColumns(TableInsert& row, const ModelEvent::Info& info){
    row.BindString(LOGAN_COL_INS(LOGAN_TIMEOFDAY), google::protobuf::util::TimeUtil::ToString(info.timestamp()));
    row.BindString(LOGAN_COL_INS(LOGAN_EXPERIMENT_NAME), info.experiment_name());
    row.BindString(LOGAN_COL_INS(LOGAN_HOSTNAME), info.hostname());
    row.BindString(LOGAN_COL_INS(LOGAN_CONTAINER_ID), info.container_id());
    row.BindString(LOGAN_COL_INS(LOGAN_CONTAINER_NAME), info.container_name());
}

void ModelEvent::ProtoHandler::BindComponentColumns(TableInsert& row, const ModelEvent::Component& component){
    row.BindString(LOGAN_COL_INS(LOGAN_COMPONENT_NAME), component.name());
    row.BindString(LOGAN_COL_INS(LOGAN_COMPONENT_ID), component.id());
    row.BindString(LOGAN_COL_INS(LOGAN_COMPONENT_TYPE), component.type());
}

void ModelEvent::ProtoHandler::BindWorkerColumns(TableInsert& row, const ModelEvent::Worker& worker){
    row.BindString(LOGAN_COL_INS(LOGAN_WORKER_NAME), worker.name());
    row.BindString(LOGAN_COL_INS(LOGAN_WORKER_ID), worker.id());
    row.BindString(LOGAN_COL_INS(LOGAN_WORKER_TYPE), worker.type());
}

void ModelEvent::ProtoHandler::BindPortColumns(TableInsert& row, const ModelEvent::Port& port){
    row.BindString(LOGAN_COL_INS(LOGAN_PORT_NAME), port.name());
    row.BindString(LOGAN_COL_INS(LOGAN_PORT_ID), port.id());
    row.BindString(LOGAN_COL_INS(LOGAN_PORT_TYPE), port.type());
    row.BindString(LOGAN_COL_INS(LOGAN_PORT_KIND), ModelEvent::Port::Kind_Name(port.kind()));
    row.BindString(LOGAN_COL_INS(LOGAN_PORT_MIDDLEWARE), port.middleware());
}

void ModelEvent::ProtoHandler::ProcessLifecycleEvent(const ModelEvent::LifecycleEvent& event){
    {
        std::lock_guard<std::mutex> lock(mutex_);
        rx_count_ ++;
    }

    try{
        auto row = GetTable(LOGAN_MODELEVENT_LIFECYCLE_TABLE).get_insert_statement();

        if(event.has_info())
            BindInfoColumns(row, event.info());
        
        if(event.has_component())
            BindComponentColumns(row, event.component());

        if(event.has_port())
            BindPortColumns(row, event.port());

        row.BindString(LOGAN_COL_INS(LOGAN_EVENT), ModelEvent::LifecycleEvent::Type_Name(event.type()));
        database_.ExecuteSqlStatement(row.get_statement());
    }catch(const std::exception& ex){
        std::cerr << "* ModelProtoHander::ProcessLifecycleEvent() Exception: " << ex.what() << std::endl;
    }
}


void ModelEvent::ProtoHandler::ProcessWorkloadEvent(const ModelEvent::WorkloadEvent& event){
    {
        std::lock_guard<std::mutex> lock(mutex_);
        rx_count_ ++;
    }
    try{
        auto row = GetTable(LOGAN_MODELEVENT_WORKLOAD_TABLE).get_insert_statement();

        if(event.has_info())
            BindInfoColumns(row, event.info());
        
        if(event.has_component())
            BindComponentColumns(row, event.component());

        if(event.has_worker())
            BindWorkerColumns(row, event.worker());

        row.BindString(LOGAN_COL_INS(LOGAN_TYPE), ModelEvent::WorkloadEvent::Type_Name(event.event_type()));
        row.BindInt(LOGAN_COL_INS(LOGAN_LOG_LEVEL), event.log_level());
        row.BindInt(LOGAN_COL_INS("workload_id"), event.workload_id());

        row.BindString(LOGAN_COL_INS("function_name"), event.function_name());
        row.BindString(LOGAN_COL_INS("args"), event.args());

        database_.ExecuteSqlStatement(row.get_statement());
    }catch(const std::exception& ex){
        std::cerr << "* ModelProtoHander::ProcessWorkloadEvent() Exception: " << ex.what() << std::endl;
    }
}

void ModelEvent::ProtoHandler::ProcessUtilizationEvent(const ModelEvent::UtilizationEvent& event){
    {
        std::lock_guard<std::mutex> lock(mutex_);
        rx_count_ ++;
    }
    try{
        auto row = GetTable(LOGAN_MODELEVENT_UTILIZATION_TABLE).get_insert_statement();

        if(event.has_info())
            BindInfoColumns(row, event.info());
        
        if(event.has_component())
            BindComponentColumns(row, event.component());

        if(event.has_port())
            BindPortColumns(row, event.port());

        
        row.BindInt(LOGAN_COL_INS("port_event_id"), event.port_event_id());
        row.BindString(LOGAN_COL_INS(LOGAN_TYPE), ModelEvent::UtilizationEvent::Type_Name(event.type()));
        row.BindString(LOGAN_COL_INS(LOGAN_MESSAGE), event.message());

        database_.ExecuteSqlStatement(row.get_statement());
    }catch(const std::exception& ex){
        std::cerr << "* ModelProtoHander::ProcessUtilizationEvent() Exception: " << ex.what() << std::endl;
    }
}

