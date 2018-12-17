#ifndef AGGREGATIONREPLIER_H
#define AGGREGATIONREPLIER_H

#include <zmq/protoreplier/protoreplier.hpp>
#include "../databaseclient.h"

#include <proto/aggregationmessage/aggregationmessage.pb.h>

namespace AggServer {

class AggregationReplier : public zmq::ProtoReplier {
public:
    AggregationReplier(std::shared_ptr<DatabaseClient> db_client);

    std::unique_ptr<AggServer::ExperimentRunResponse>
    ProcessExperimentRunRequest(
        const AggServer::ExperimentRunRequest& message
    );

    std::unique_ptr<AggServer::ExperimentStateResponse>
    ProcessExperimentStateRequest(
        const AggServer::ExperimentStateRequest& message
    );

    std::unique_ptr<AggServer::PortLifecycleResponse>
    ProcessPortLifecycleRequest(
        const AggServer::PortLifecycleRequest& message
    );

    std::unique_ptr<AggServer::WorkloadResponse>
    ProcessWorkloadEventRequest(
        const AggServer::WorkloadRequest& message
    );

    std::unique_ptr<AggServer::CPUUtilisationResponse>
    ProcessCPUUtilisationRequest(
        const AggServer::CPUUtilisationRequest& message
    );

private:
    std::shared_ptr<DatabaseClient> database_;

    void RegisterCallbacks();
};

}

#endif //AGGREGATIONREPLIER_H