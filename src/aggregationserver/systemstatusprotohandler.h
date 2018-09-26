#ifndef SystemStatusProtoHandler_H
#define SystemStatusProtoHandler_H

#include "aggregationprotohandler.h"

#include <proto/systemstatus/systemstatus.pb.h>

class SystemStatusProtoHandler : public AggregationProtoHandler {
public:
    SystemStatusProtoHandler(std::shared_ptr<DatabaseClient> db_client, std::shared_ptr<ExperimentTracker> exp_tracker) 
        : AggregationProtoHandler(db_client, exp_tracker) {};

    void BindCallbacks(zmq::ProtoReceiver& ProtoReceiver);

private:
    // Hardware callbacks
    void ProcessSystemStatus(const re_common::SystemStatus& status) {};
    void ProcessSystemInfo(const re_common::SystemInfo& info) {};
};


#endif