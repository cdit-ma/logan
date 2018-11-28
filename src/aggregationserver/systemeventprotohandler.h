#ifndef SYSTEMEVENTPROTOHANDLER_H
#define SYSTEMEVENTPROTOHANDLER_H

#include "aggregationprotohandler.h"

#include <proto/systemevent/systemevent.pb.h>

class SystemEventProtoHandler : public AggregationProtoHandler {
public:
    SystemEventProtoHandler(std::shared_ptr<DatabaseClient> db_client, ExperimentTracker& exp_tracker, int experiment_run_id) 
        : AggregationProtoHandler(db_client, exp_tracker), experiment_run_id_(experiment_run_id) {};

    void BindCallbacks(zmq::ProtoReceiver& ProtoReceiver);

private:
    // Hardware callbacks
    void ProcessStatusEvent(const SystemEvent::StatusEvent& status) {};
    void ProcessInfoEvent(const SystemEvent::InfoEvent& info);
    void ProcessFileSystemInfo(const SystemEvent::FileSystemInfo& fs_info, int node_id);
    void ProcessInterfaceInfo(const SystemEvent::InterfaceInfo& i_info, int node_id);

    // Experiment info
    int experiment_run_id_;
};


#endif //SYSTEMEVENTPROTOHANDLER_H