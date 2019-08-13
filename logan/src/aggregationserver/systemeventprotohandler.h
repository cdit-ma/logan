#ifndef SYSTEMEVENTPROTOHANDLER_H
#define SYSTEMEVENTPROTOHANDLER_H

#include "aggregationprotohandler.h"

#include <proto/systemevent/systemevent.pb.h>

#include <map>

class SystemEventProtoHandler : public AggregationProtoHandler {
public:
    SystemEventProtoHandler(std::shared_ptr<DatabaseClient> db_client, ExperimentTracker& exp_tracker, int experiment_run_id) 
        : AggregationProtoHandler(db_client, exp_tracker), experiment_run_id_(experiment_run_id) {};

    void BindCallbacks(zmq::ProtoReceiver& ProtoReceiver);

private:
    // Hardware callbacks
    void ProcessStatusEvent(const SystemEvent::StatusEvent& status);
    void ProcessInterfaceStatus(
        const SystemEvent::InterfaceStatus& if_status,
        const std::string& hostname,
        const std::string& timestamp
    );
    void ProcessFileSystemStatus(
        const SystemEvent::FileSystemStatus& fs_status,
        const std::string& hostname,
        const std::string& timestamp
    );
    void ProcessProcessStatus(
        const SystemEvent::ProcessStatus& p_status,
        const std::string& hostname,
        const std::string& timestamp
    );
    void ProcessInfoEvent(const SystemEvent::InfoEvent& info);
    void ProcessFileSystemInfo(
        const SystemEvent::FileSystemInfo& fs_info,
        const std::string& hostname,
        int node_id
    );
    void ProcessInterfaceInfo(
        const SystemEvent::InterfaceInfo& i_info,
        const std::string& hostname,
        int node_id
    );
    void ProcessProcessInfo(
        const SystemEvent::ProcessInfo& p_info,
        const std::string& hostname
    );

    // ID lookup key generator helper functions
    std::string GetInterfaceKey(const std::string& hostname, const std::string& if_name) const;
    std::string GetFileSystemKey(const std::string& hostname, const std::string& fs_name) const;
    std::string GetProcessKey(const std::string& hostname, int pid, const std::string& starttime) const;

    // Experiment info
    int experiment_run_id_;

    // Cache maps
    std::map<std::string, int> system_id_cache_; // hostname -> SystemID
    std::map<std::string, int> filesystem_id_cache_; // hostname/fs_name -> FileSystemID
    std::map<std::string, int> interface_id_cache_; // hostname/if_name -> InterfaceID
    std::map<std::string, int> process_id_cache_; // hostname/pID/starttime -> InterfaceID
};


#endif //SYSTEMEVENTPROTOHANDLER_H