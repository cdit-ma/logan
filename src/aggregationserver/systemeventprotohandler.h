#ifndef SYSTEMEVENTPROTOHANDLER_H
#define SYSTEMEVENTPROTOHANDLER_H

#include "aggregationprotohandler.h"

#include <proto/systemevent/systemevent.pb.h>

class SystemEventProtoHandler : public AggregationProtoHandler {
public:
    SystemEventProtoHandler(std::shared_ptr<DatabaseClient> db_client, ExperimentTracker& exp_tracker) 
        : AggregationProtoHandler(db_client, exp_tracker) {};

    void BindCallbacks(zmq::ProtoReceiver& ProtoReceiver);

private:
    // Hardware callbacks
    void ProcessStatusEvent(const SystemEvent::StatusEvent& status) {};
    void ProcessInfoEvent(const SystemEvent::InfoEvent& info) {};
};


#endif //SYSTEMEVENTPROTOHANDLER_H