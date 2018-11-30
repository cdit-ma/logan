#include "systemeventprotohandler.h"

#include "utils.h"

#include "databaseclient.h"
#include "experimenttracker.h"

#include <zmq/protoreceiver/protoreceiver.h>

#include <google/protobuf/util/time_util.h>

#include <functional>

using google::protobuf::util::TimeUtil;

void SystemEventProtoHandler::BindCallbacks(zmq::ProtoReceiver& receiver) {
    receiver.RegisterProtoCallback<SystemEvent::StatusEvent>(std::bind(&SystemEventProtoHandler::ProcessStatusEvent, this, std::placeholders::_1));
    receiver.RegisterProtoCallback<SystemEvent::InfoEvent>(std::bind(&SystemEventProtoHandler::ProcessInfoEvent, this, std::placeholders::_1));
}


void SystemEventProtoHandler::ProcessInfoEvent(const SystemEvent::InfoEvent& info) {
    //const std::string& timestamp = TimeUtil::ToString(info.timestamp());
    const std::string& hostname = info.hostname();
    const int node_id = experiment_tracker_.GetNodeIDFromHostname(experiment_run_id_, hostname);
    const std::string& node_id_str = std::to_string(node_id);

    const std::string& os_name = info.os_name();
    const std::string& os_arch = info.os_arch();
    const std::string& os_desc = info.os_description();
    const std::string& os_version = info.os_version();
    const std::string& os_vendor = info.os_vendor();
    const std::string& os_vendor_name = info.os_vendor_name();

    const std::string& cpu_model = info.cpu_model();
    const std::string& cpu_vendor = info.cpu_vendor();
    const std::string& cpu_frequency = std::to_string(info.cpu_frequency_hz());

    const std::string& physical_memory = std::to_string(info.physical_memory_kilobytes());

    database_->InsertValuesUnique(
        "Hardware.System",
        {"NodeID", "OSName", "OSArch", "OSDescription", "OSVersion", "OSVendor", "OSVendorName", "CPUModel", "CPUVendor", "CPUFrequencyHZ", "PhysicalMemoryKB"},
        {node_id_str, os_name, os_arch, os_desc, os_version, os_vendor, os_vendor_name, cpu_model, cpu_vendor, cpu_frequency, physical_memory},
        {"NodeID"}
    );

    for (const auto& fs_info : info.file_system_info()) {
        ProcessFileSystemInfo(fs_info, node_id);
    }

    for (const auto& i_info : info.interface_info()) {
        ProcessInterfaceInfo(i_info, node_id);
    }
}


void SystemEventProtoHandler::ProcessFileSystemInfo(const SystemEvent::FileSystemInfo& fs_info, int node_id) {
    const std::string& name = fs_info.name();
    const std::string& type = SystemEvent::FileSystemInfo_Type_Name(fs_info.type());

    const std::string& size = std::to_string(fs_info.size_kilobytes());
    
    database_->InsertValuesUnique(
        "Hardware.Filesystem",
        {"Name", "Type", "Size", "NodeID"},
        {name, type, size, std::to_string(node_id)},
        {"NodeID", "Name"}
    );
}

void SystemEventProtoHandler::ProcessInterfaceInfo(const SystemEvent::InterfaceInfo& i_info, int node_id) {
    const std::string& node_id_str = std::to_string(node_id);
    const std::string& name = i_info.name();
    const std::string& type = i_info.type();
    const std::string& desc = i_info.description();
    const std::string& ipv4 = i_info.ipv4_addr();
    const std::string& ipv6 = i_info.ipv6_addr();
    const std::string& mac = i_info.mac_addr();
    const std::string& speed = std::to_string((int64_t)i_info.speed());

    database_->InsertValuesUnique(
        "Hardware.Interface",
        {"NodeID", "Name", "Type", "Description", "IPv4", "IPv6", "MAC", "Speed"},
        {node_id_str, name, type, desc, ipv4, ipv6, mac, speed},
        {"NodeID", "Name"}
    );
}