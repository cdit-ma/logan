#ifndef MODELEVENTPROTOHANDLER_H
#define MODELEVENTPROTOHANDLER_H

#include "aggregationprotohandler.h"

#include <proto/modelevent/modelevent.pb.h>

class ModelEventProtoHandler : public AggregationProtoHandler {
public:
    ModelEventProtoHandler(std::shared_ptr<DatabaseClient> db_client, std::shared_ptr<ExperimentTracker> exp_tracker)
        : AggregationProtoHandler(db_client, exp_tracker) {};

    void BindCallbacks(zmq::ProtoReceiver& ProtoReceiver);

private:
    // Model callbacks
    void ProcessUserEvent(const re_common::UserEvent& message);
    void ProcessLifecycleEvent(const re_common::LifecycleEvent& message);
    void ProcessWorkloadEvent(const re_common::WorkloadEvent& message);
    void ProcessComponentUtilizationEvent(const re_common::ComponentUtilizationEvent& message);

    // Insertion helpers
    void InsertComponentLifecycleEvent(const re_common::Info& info,
            const re_common::LifecycleEvent_Type& type,
            const re_common::Component& component);

    void InsertPortLifecycleEvent(const re_common::Info& info,
            const re_common::LifecycleEvent_Type& type,
            const re_common::Component& component,
            const re_common::Port& port);

    // ID retrieval helpers
    int GetComponentID(const std::string& name, const std::string& experiment_name);
    int GetComponentInstanceID(const re_common::Component& component_instance, const std::string& experiment_name);
    int GetPortID(const re_common::Port& port, const re_common::Component& component, const std::string& experiment_name);
    int GetWorkerInstanceID(const re_common::Component& component_instance, const std::string& worker_name, const std::string& experiment_name);

};

#endif