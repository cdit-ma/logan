#ifndef NODEMANAGERPROTOHANDLER_H
#define NODEMANAGERPROTOHANDLER_H

#include "aggregationprotohandler.h"

#include <re_common/proto/controlmessage/controlmessage.pb.h>

class NodeManagerProtoHandler : public AggregationProtoHandler {
public:
    NodeManagerProtoHandler(std::shared_ptr<DatabaseClient> db_client, std::shared_ptr<ExperimentTracker> exp_tracker)
        : AggregationProtoHandler(db_client, exp_tracker) {};

    void BindCallbacks(zmq::ProtoReceiver& ProtoReceiver);

private:
    // Process environment manager callbacks
    void ProcessEnvironmentMessage(const NodeManager::EnvironmentMessage& message);
    void ProcessControlMessage(const NodeManager::ControlMessage& message);
    void ProcessNode(const NodeManager::Node& message, int experiment_run_id);
    void ProcessComponent(const NodeManager::Component& message, int experiment_run_id, int node_id);
    void ProcessPort(const NodeManager::Port& message, int component_instance_id, const std::string& component_instance_location);
    void ProcessWorker(const NodeManager::Worker& message, int experiment_run_id, int component_instance_id, const std::string& worker_path);
};

#endif