#ifndef AGGREGATIONPROTOHANDLER_H
#define AGGREGATIONPROTOHANDLER_H

#include "../server/protohandler.h"
#include <memory>

//#include <re_common/proto/modelevent/modelevent.pb.h>

class DatabaseClient;
class ExperimentTracker;

class AggregationProtoHandler : public ProtoHandler {
public:
    AggregationProtoHandler(std::shared_ptr<DatabaseClient> db_client, ExperimentTracker& exp_tracker);

    virtual void BindCallbacks(zmq::ProtoReceiver& ProtoReceiver) = 0;

protected:
    std::shared_ptr<DatabaseClient> database_;
    ExperimentTracker& experiment_tracker_;

};

#endif //AGGREGATIONPROTOHANDLER_H