#ifndef AGGREGATIONREPLIER_H
#define AGGREGATIONREPLIER_H

#include <zmq/protoreplier/protoreplier.hpp>
#include "../databaseclient.h"

#include <proto/aggregationmessage/aggregationmessage.pb.h>

namespace AggServer {

class AggregationReplier : public zmq::ProtoReplier {
public:
    AggregationReplier(std::shared_ptr<DatabaseClient> db_client);


    std::unique_ptr<AggServer::PortLifecycleResponse>
    ProcessPortLifecycleRequest(
        const AggServer::PortLifecycleRequest& message
        );

private:
    std::shared_ptr<DatabaseClient> database_;
    
};

}

#endif //AGGREGATIONREPLIER_H