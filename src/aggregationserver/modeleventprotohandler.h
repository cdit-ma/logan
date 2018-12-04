#ifndef MODELEVENTPROTOHANDLER_H
#define MODELEVENTPROTOHANDLER_H

#include "aggregationprotohandler.h"

#include <proto/modelevent/modelevent.pb.h>

class ModelEventProtoHandler : public AggregationProtoHandler {
public:
    ModelEventProtoHandler(std::shared_ptr<DatabaseClient> db_client, ExperimentTracker& exp_tracker, int experiment_run_id)
        : AggregationProtoHandler(db_client, exp_tracker), experiment_run_id_(experiment_run_id) {};

    void BindCallbacks(zmq::ProtoReceiver& ProtoReceiver);

private:
    // Model callbacks
    //void ProcessUserEvent(const ModelEvent::UserEvent& message);
    void ProcessLifecycleEvent(const ModelEvent::LifecycleEvent& message);
    void ProcessWorkloadEvent(const ModelEvent::WorkloadEvent& message);
    void ProcessUtilizationEvent(const ModelEvent::UtilizationEvent& message);

    // Insertion helpers
    void InsertComponentLifecycleEvent(const ModelEvent::Info& info,
            const ModelEvent::LifecycleEvent_Type& type,
            const ModelEvent::Component& component);

    void InsertPortLifecycleEvent(const ModelEvent::Info& info,
            const ModelEvent::LifecycleEvent_Type& type,
            const ModelEvent::Component& component,
            const ModelEvent::Port& port);

    // ID retrieval helpers
    int GetComponentID(const std::string& name, const std::string& experiment_name);
    int GetComponentInstanceID(const ModelEvent::Component& component_instance, const std::string& experiment_name);
    int GetPortID(const ModelEvent::Port& port, const ModelEvent::Component& component, const std::string& experiment_name);
    int GetWorkerInstanceID(const ModelEvent::Component& component_instance, const std::string& worker_name, const std::string& experiment_name);


    int experiment_run_id_;
};

#endif