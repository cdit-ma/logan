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

//Type names
#define LOGAN_DECIMAL "DECIMAL"
#define LOGAN_VARCHAR "VARCHAR"
#define LOGAN_INT "INTEGER"

//Common column names
#define LOGAN_TIMEOFDAY "timeofday"
#define LOGAN_HOSTNAME "hostname"
#define LOGAN_MESSAGE_ID "id"
#define LOGAN_NAME "name"
#define LOGAN_TYPE "type"

//Hardware table names
#define LOGAN_CPU_TABLE "HardwareStatus_CPU"
#define LOGAN_FILE_SYSTEM_TABLE "HardwareStatus_FileSystem"
#define LOGAN_SYSTEM_STATUS_TABLE "HardwareStatus_System"
#define LOGAN_PROCESS_STATUS_TABLE "HardwareStatus_Process"
#define LOGAN_INTERFACE_STATUS_TABLE "HardwareStatus_Interface"

#define LOGAN_SYSTEM_INFO_TABLE "HardwareInfo_System"
#define LOGAN_PROCESS_INFO_TABLE "HardwareInfo_Process"
#define LOGAN_INTERFACE_INFO_TABLE "HardwareInfo_Interface"
#define LOGAN_FILE_SYSTEM_INFO_TABLE "HardwareInfo_FileSystem"

#define LOGAN_COL_INS(X) ":" X

void SystemEvent::ProtoHandler::AddInfoColumns(Table& table){
    table.AddColumn(LOGAN_TIMEOFDAY, LOGAN_VARCHAR);
    table.AddColumn(LOGAN_HOSTNAME, LOGAN_VARCHAR);
    table.AddColumn(LOGAN_MESSAGE_ID, LOGAN_INT);
}

SystemEvent::ProtoHandler::~ProtoHandler(){
    std::lock_guard<std::mutex> lock(mutex_);
    std::cout << "* SystemEvent::ProtoHandler: Processed: " << rx_count_ << " Messages" << std::endl;
};

void SystemEvent::ProtoHandler::BindInfoColumns(TableInsert& row, const std::string& time, const std::string& host_name, const int64_t message_id){
    row.BindString(LOGAN_COL_INS(LOGAN_TIMEOFDAY), time);
    row.BindString(LOGAN_COL_INS(LOGAN_HOSTNAME), host_name);
    row.BindInt(LOGAN_COL_INS(LOGAN_MESSAGE_ID), message_id);
}

SystemEvent::ProtoHandler::ProtoHandler(SQLiteDatabase& database):
    ::ProtoHandler(),
    database_(database)
{
    //Create the relevant tables
    CreateSystemStatusTable();
    CreateSystemInfoTable();
    CreateCpuTable();
    CreateFileSystemTable();
    CreateFileSystemInfoTable();
    CreateInterfaceTable();
    CreateInterfaceInfoTable();
    CreateProcessTable();
    CreateProcessInfoTable();
}

Table& SystemEvent::ProtoHandler::GetTable(const std::string& table_name){
    return *(tables_.at(table_name));
}

bool SystemEvent::ProtoHandler::GotTable(const std::string& table_name){
    try{
        GetTable(table_name);
        return true;
    }catch(const std::exception& ex){}
    return false;
}

void SystemEvent::ProtoHandler::BindCallbacks(zmq::ProtoReceiver& receiver){
    //Register call back functions and type with zmqreceiver
    receiver.RegisterProtoCallback<StatusEvent>(std::bind(&ProtoHandler::ProcessStatusEvent, this, std::placeholders::_1));
    receiver.RegisterProtoCallback<InfoEvent>(std::bind(&ProtoHandler::ProcessInfoEvent, this, std::placeholders::_1));
}

void SystemEvent::ProtoHandler::CreateSystemStatusTable(){
    if(GotTable(LOGAN_SYSTEM_STATUS_TABLE)){
        return;
    }
    auto table_ptr = std::unique_ptr<Table>(new Table(database_, LOGAN_SYSTEM_STATUS_TABLE));
    auto& table = *table_ptr;

    AddInfoColumns(table);
    table.AddColumn("cpu_utilization", LOGAN_DECIMAL);
    table.AddColumn("phys_mem_utilization", LOGAN_DECIMAL);
    table.Finalize();

    tables_.emplace(std::make_pair(LOGAN_SYSTEM_STATUS_TABLE, std::move(table_ptr)));
    database_.ExecuteSqlStatement(table.get_table_construct_statement());
}

void SystemEvent::ProtoHandler::CreateSystemInfoTable(){
    if(GotTable(LOGAN_SYSTEM_INFO_TABLE)){
        return;
    }
    auto table_ptr = std::unique_ptr<Table>(new Table(database_, LOGAN_SYSTEM_INFO_TABLE));
    auto& table = *table_ptr;

    AddInfoColumns(table);
    //OS Info
    table.AddColumn("os_name", LOGAN_VARCHAR);
    table.AddColumn("os_arch", LOGAN_VARCHAR);
    table.AddColumn("os_description", LOGAN_VARCHAR);
    table.AddColumn("os_version", LOGAN_VARCHAR);
    table.AddColumn("os_vendor", LOGAN_VARCHAR);
    table.AddColumn("os_vendor_name", LOGAN_VARCHAR);

    //CPU Info
    table.AddColumn("cpu_model", LOGAN_VARCHAR);
    table.AddColumn("cpu_vendor", LOGAN_VARCHAR);
    table.AddColumn("cpu_frequency_hz", LOGAN_INT);
    table.AddColumn("physical_memory_kB", LOGAN_INT);
    table.Finalize();

    tables_.emplace(std::make_pair(LOGAN_SYSTEM_INFO_TABLE, std::move(table_ptr)));
    database_.ExecuteSqlStatement(table.get_table_construct_statement());
}

void SystemEvent::ProtoHandler::CreateCpuTable(){
    if(GotTable(LOGAN_CPU_TABLE)){
        return;
    }
    auto table_ptr = std::unique_ptr<Table>(new Table(database_, LOGAN_CPU_TABLE));
    auto& table = *table_ptr;

    AddInfoColumns(table);
    table.AddColumn("core_id", LOGAN_INT);
    table.AddColumn("core_utilization", LOGAN_DECIMAL);
    table.Finalize();

    tables_.emplace(std::make_pair(LOGAN_CPU_TABLE, std::move(table_ptr)));
    database_.ExecuteSqlStatement(table.get_table_construct_statement());
}

void SystemEvent::ProtoHandler::CreateFileSystemTable(){
    if(GotTable(LOGAN_FILE_SYSTEM_TABLE)){
        return;
    }
    auto table_ptr = std::unique_ptr<Table>(new Table(database_, LOGAN_FILE_SYSTEM_TABLE));
    auto& table = *table_ptr;

    AddInfoColumns(table);
    table.AddColumn(LOGAN_NAME, LOGAN_VARCHAR);
    table.AddColumn("utilization", LOGAN_DECIMAL);
    table.Finalize();

    tables_.emplace(std::make_pair(LOGAN_FILE_SYSTEM_TABLE, std::move(table_ptr)));
    database_.ExecuteSqlStatement(table.get_table_construct_statement());
}

void SystemEvent::ProtoHandler::CreateFileSystemInfoTable(){
    if(GotTable(LOGAN_FILE_SYSTEM_INFO_TABLE)){
        return;
    }
    auto table_ptr = std::unique_ptr<Table>(new Table(database_, LOGAN_FILE_SYSTEM_INFO_TABLE));
    auto& table = *table_ptr;

    AddInfoColumns(table);
    table.AddColumn(LOGAN_NAME, LOGAN_VARCHAR);
    table.AddColumn(LOGAN_TYPE, LOGAN_VARCHAR);
    table.AddColumn("total_size_kB", LOGAN_INT);
    table.Finalize();

    tables_.emplace(std::make_pair(LOGAN_FILE_SYSTEM_INFO_TABLE, std::move(table_ptr)));
    database_.ExecuteSqlStatement(table.get_table_construct_statement());
}

void SystemEvent::ProtoHandler::CreateInterfaceTable(){
    if(GotTable(LOGAN_INTERFACE_STATUS_TABLE)){
        return;
    }
    auto table_ptr = std::unique_ptr<Table>(new Table(database_, LOGAN_INTERFACE_STATUS_TABLE));
    auto& table = *table_ptr;
    
    AddInfoColumns(table);
    table.AddColumn(LOGAN_NAME, LOGAN_VARCHAR);
    table.AddColumn("rx_packets", LOGAN_INT);
    table.AddColumn("rx_bytes", LOGAN_INT);
    table.AddColumn("tx_packets", LOGAN_INT);
    table.AddColumn("tx_bytes", LOGAN_INT);
    table.Finalize();

    tables_.emplace(std::make_pair(LOGAN_INTERFACE_STATUS_TABLE, std::move(table_ptr)));
    database_.ExecuteSqlStatement(table.get_table_construct_statement());
}

void SystemEvent::ProtoHandler::CreateInterfaceInfoTable(){
    if(GotTable(LOGAN_INTERFACE_INFO_TABLE)){
        return;
    }
    auto table_ptr = std::unique_ptr<Table>(new Table(database_, LOGAN_INTERFACE_INFO_TABLE));
    auto& table = *table_ptr;

    AddInfoColumns(table);
    table.AddColumn(LOGAN_NAME, LOGAN_VARCHAR);
    table.AddColumn(LOGAN_TYPE, LOGAN_VARCHAR);
    table.AddColumn("description", LOGAN_VARCHAR);
    table.AddColumn("ipv4_addr", LOGAN_VARCHAR);
    table.AddColumn("ipv6_addr", LOGAN_VARCHAR);
    table.AddColumn("mac_addr", LOGAN_VARCHAR);
    table.AddColumn("speed", LOGAN_INT);
    table.Finalize();

    tables_.emplace(std::make_pair(LOGAN_INTERFACE_INFO_TABLE, std::move(table_ptr)));
    database_.ExecuteSqlStatement(table.get_table_construct_statement());
}

void SystemEvent::ProtoHandler::CreateProcessTable(){
    if(GotTable(LOGAN_PROCESS_STATUS_TABLE)){
        return;
    }
    auto table_ptr = std::unique_ptr<Table>(new Table(database_, LOGAN_PROCESS_STATUS_TABLE));
    auto& table = *table_ptr;

    AddInfoColumns(table);
    table.AddColumn("pid", LOGAN_INT);
    table.AddColumn("name", LOGAN_VARCHAR);
    table.AddColumn("core_id", LOGAN_INT);

    table.AddColumn("cpu_utilization", LOGAN_DECIMAL);
    table.AddColumn("phys_mem_used_kB", LOGAN_INT);
    table.AddColumn("phys_mem_utilization", LOGAN_DECIMAL);
    table.AddColumn("thread_count", LOGAN_INT);

    table.AddColumn("disk_read_kB", LOGAN_INT);
    table.AddColumn("disk_written_kB", LOGAN_INT);
    table.AddColumn("disk_total_kB", LOGAN_INT);

    table.AddColumn("cpu_time_ms", LOGAN_INT);
    table.AddColumn("state", LOGAN_VARCHAR);
    table.Finalize();

    tables_.emplace(std::make_pair(LOGAN_PROCESS_STATUS_TABLE, std::move(table_ptr)));
    database_.ExecuteSqlStatement(table.get_table_construct_statement());
}

void SystemEvent::ProtoHandler::CreateProcessInfoTable(){
    if(GotTable(LOGAN_PROCESS_INFO_TABLE)){
        return;
    }
    auto table_ptr = std::unique_ptr<Table>(new Table(database_, LOGAN_PROCESS_INFO_TABLE));
    auto& table = *table_ptr;

    AddInfoColumns(table);
    table.AddColumn("pid", LOGAN_INT);
    table.AddColumn("cwd", LOGAN_VARCHAR);
    table.AddColumn(LOGAN_NAME, LOGAN_VARCHAR);
    table.AddColumn("args", LOGAN_VARCHAR);
    table.AddColumn("start_time", LOGAN_INT);
    table.Finalize();

    tables_.emplace(std::make_pair(LOGAN_PROCESS_INFO_TABLE, std::move(table_ptr)));
    database_.ExecuteSqlStatement(table.get_table_construct_statement());
}

void SystemEvent::ProtoHandler::ProcessStatusEvent(const StatusEvent& status){
    {
        std::lock_guard<std::mutex> lock(mutex_);
        rx_count_ ++;
    }

    //Get the Globals
    const auto& host_name = status.hostname();
    const auto& message_id = status.message_id();
    const auto& timestamp = google::protobuf::util::TimeUtil::ToString(status.timestamp());
    
    {
        auto row = GetTable(LOGAN_SYSTEM_STATUS_TABLE).get_insert_statement();

        BindInfoColumns(row, timestamp, host_name, message_id);
        row.BindDouble(LOGAN_COL_INS("cpu_utilization"), status.cpu_utilization());
        row.BindDouble(LOGAN_COL_INS("phys_mem_utilization"), status.phys_mem_utilization());
        ExecuteTableStatement(row);
    }

    for(size_t i = 0; i < status.cpu_core_utilization_size(); i++){
        auto row = GetTable(LOGAN_CPU_TABLE).get_insert_statement();

        BindInfoColumns(row, timestamp, host_name, message_id);

        row.BindInt(LOGAN_COL_INS("core_id"), i);
        row.BindDouble(LOGAN_COL_INS("core_utilization"), status.cpu_core_utilization(i));
        ExecuteTableStatement(row);
    }

    for(size_t i = 0; i < status.processes_size(); i++){
        const auto& proc_pb = status.processes(i);
        auto row = GetTable(LOGAN_PROCESS_STATUS_TABLE).get_insert_statement();

        BindInfoColumns(row, timestamp, host_name, message_id);
        row.BindInt(LOGAN_COL_INS("pid"), proc_pb.pid());
        row.BindString(LOGAN_COL_INS("name"), proc_pb.name());
        row.BindInt(LOGAN_COL_INS("core_id"), proc_pb.cpu_core_id());

        row.BindDouble(LOGAN_COL_INS("cpu_utilization"), proc_pb.cpu_utilization());
        row.BindInt(LOGAN_COL_INS("phys_mem_used_kB"), proc_pb.phys_mem_used_kb());
        row.BindDouble(LOGAN_COL_INS("phys_mem_utilization"), proc_pb.phys_mem_utilization());
        row.BindInt(LOGAN_COL_INS("thread_count"), proc_pb.thread_count());

        row.BindInt(LOGAN_COL_INS("disk_read_kB"), proc_pb.disk_read_kilobytes());
        row.BindInt(LOGAN_COL_INS("disk_written_kB"), proc_pb.disk_written_kilobytes());
        row.BindInt(LOGAN_COL_INS("disk_total_kB"), proc_pb.disk_total_kilobytes());
        
        row.BindInt(LOGAN_COL_INS("cpu_time_ms"), google::protobuf::util::TimeUtil::DurationToMilliseconds(proc_pb.cpu_time()));
        row.BindString(LOGAN_COL_INS("state"), ProcessStatus::State_Name(proc_pb.state()));
        ExecuteTableStatement(row);
    }

    for(size_t i = 0; i < status.interfaces_size(); i++){
        const auto& iface_pb = status.interfaces(i);
        auto row = GetTable(LOGAN_INTERFACE_STATUS_TABLE).get_insert_statement();

        BindInfoColumns(row, timestamp, host_name, message_id);
        row.BindString(LOGAN_COL_INS(LOGAN_NAME), iface_pb.name());
        row.BindInt(LOGAN_COL_INS("rx_packets"), iface_pb.rx_packets());
        row.BindInt(LOGAN_COL_INS("rx_bytes"), iface_pb.rx_bytes());
        row.BindInt(LOGAN_COL_INS("tx_packets"), iface_pb.tx_packets());
        row.BindInt(LOGAN_COL_INS("tx_bytes"), iface_pb.tx_bytes());
        ExecuteTableStatement(row);
    }

    for(size_t i = 0; i < status.file_systems_size(); i++){
        const auto& fs_pb = status.file_systems(i);
        auto row = GetTable(LOGAN_FILE_SYSTEM_TABLE).get_insert_statement();

        BindInfoColumns(row, timestamp, host_name, message_id);
        row.BindString(LOGAN_COL_INS(LOGAN_NAME), fs_pb.name());
        row.BindDouble(LOGAN_COL_INS("utilization"), fs_pb.utilization());
        ExecuteTableStatement(row);
    }

    for(size_t i = 0; i < status.process_info_size(); i++){
        const auto& proc_pb = status.process_info(i);
        if(proc_pb.pid() > 0){
            auto row = GetTable(LOGAN_PROCESS_INFO_TABLE).get_insert_statement();

            BindInfoColumns(row, timestamp, host_name, message_id);
            row.BindInt(LOGAN_COL_INS("pid"), proc_pb.pid());
            row.BindString(LOGAN_COL_INS("cwd"), proc_pb.cwd());
            row.BindString(LOGAN_COL_INS(LOGAN_NAME), proc_pb.name());
            row.BindString(LOGAN_COL_INS("args"), proc_pb.args());

            const auto& start_time = google::protobuf::util::TimeUtil::ToString(proc_pb.start_time());

            row.BindString(LOGAN_COL_INS("start_time"), start_time);
            ExecuteTableStatement(row);
        }
    }
}

void SystemEvent::ProtoHandler::ExecuteTableStatement(TableInsert& row){
    database_.ExecuteSqlStatement(row.get_statement());
}

void SystemEvent::ProtoHandler::ProcessInfoEvent(const InfoEvent& info){
    {
        std::lock_guard<std::mutex> lock(mutex_);
        rx_count_ ++;
    }

    if(registered_nodes_.count(info.hostname())){
        return;
    }

    //Get the Globals
    const auto& host_name = info.hostname();
    const auto& message_id = info.message_id();
    const auto& timestamp = google::protobuf::util::TimeUtil::ToString(info.timestamp());

    {
        //Register the node
        registered_nodes_.insert(host_name);

        auto row = GetTable(LOGAN_SYSTEM_INFO_TABLE).get_insert_statement();

        BindInfoColumns(row, timestamp, host_name, message_id);

        //Bind OS Info
        row.BindString(LOGAN_COL_INS("os_name"), info.os_name());
        row.BindString(LOGAN_COL_INS("os_arch"), info.os_arch());
        row.BindString(LOGAN_COL_INS("os_description"), info.os_description());
        row.BindString(LOGAN_COL_INS("os_version"), info.os_version());
        row.BindString(LOGAN_COL_INS("os_vendor"), info.os_vendor());
        row.BindString(LOGAN_COL_INS("os_vendor_name"), info.os_vendor_name());

        //Bind CPU Info
        row.BindString(LOGAN_COL_INS("cpu_model"), info.cpu_model());
        row.BindString(LOGAN_COL_INS("cpu_vendor"), info.cpu_vendor());
        row.BindInt(LOGAN_COL_INS("cpu_frequency_hz"), info.cpu_frequency_hz());
        row.BindInt(LOGAN_COL_INS("physical_memory_kB"), info.physical_memory_kilobytes());

        ExecuteTableStatement(row);
    }

    for(size_t i = 0; i < info.file_system_info_size(); i++){
        const auto& fs_pb = info.file_system_info(i);

        auto row = GetTable(LOGAN_FILE_SYSTEM_INFO_TABLE).get_insert_statement();

        BindInfoColumns(row, timestamp, host_name, message_id);
        
        row.BindString(LOGAN_COL_INS(LOGAN_NAME), fs_pb.name());
        row.BindString(LOGAN_COL_INS(LOGAN_TYPE), FileSystemInfo::Type_Name(fs_pb.type()));
        row.BindInt(LOGAN_COL_INS("total_size_kB"), fs_pb.size_kilobytes());

        ExecuteTableStatement(row);
    }

    for(size_t i = 0; i < info.interface_info_size(); i++){
        const auto& iface_pb = info.interface_info(i);

        auto row = GetTable(LOGAN_INTERFACE_INFO_TABLE).get_insert_statement();
        BindInfoColumns(row, timestamp, host_name, message_id);

        row.BindString(LOGAN_COL_INS(LOGAN_NAME), iface_pb.name());
        row.BindString(LOGAN_COL_INS("type"), iface_pb.type());
        row.BindString(LOGAN_COL_INS("description"), iface_pb.description());
        row.BindString(LOGAN_COL_INS("ipv4_addr"), iface_pb.ipv4_addr());
        row.BindString(LOGAN_COL_INS("ipv6_addr"), iface_pb.ipv6_addr());
        row.BindString(LOGAN_COL_INS("mac_addr"), iface_pb.mac_addr());
        row.BindInt(LOGAN_COL_INS("speed"), iface_pb.speed());

        ExecuteTableStatement(row);
    }
}
