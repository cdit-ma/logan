#ifndef AGGREGATIONPROTOHANDLER_H
#define AGGREGATIONPROTOHANDLER_H

#include "../server/protohandler.h"
#include <string>

class DatabaseClient;
class ExperimentTracker;

namespace re_common {
    class Component;

    // Model events
    class UserEvent;
    class LifecycleEvent;
        //enum LifecycleEvent_Type;
    class WorkloadEvent;
    class ComponentUtilizationEvent;
    class MessageEvent;

    // Hardware events
    class SystemInfo;
    class SystemStatus;
}

#include <re_common/proto/controlmessage/controlmessage.pb.h>
#include <re_common/proto/modelevent/modelevent.pb.h>

class AggregationProtoHandler : public ProtoHandler {
    public:
        AggregationProtoHandler(std::shared_ptr<DatabaseClient> db_client, std::shared_ptr<ExperimentTracker> exp_tracker);

        void BindCallbacks(zmq::ProtoReceiver& ProtoReceiver);

    private:
        // Process environment manager callbacks
        void ProcessEnvironmentMessage(const NodeManager::EnvironmentMessage& message);
        void ProcessControlMessage(const NodeManager::ControlMessage& message);
        void ProcessNode(const NodeManager::Node& message, int experiment_run_id);
        void ProcessComponent(const NodeManager::Component& message, int experiment_run_id, int node_id);
        void ProcessPort(const NodeManager::Port& message, int component_instance_id, const std::string& component_instance_location);
        void ProcessWorker(const NodeManager::Worker& message, int experiment_run_id, int component_instance_id, const std::string& worker_path);

        // Model callbacks
        void ProcessUserEvent(const re_common::UserEvent& message);
        void ProcessLifecycleEvent(const re_common::LifecycleEvent& message);
        void ProcessWorkloadEvent(const re_common::WorkloadEvent& message);
        void ProcessComponentUtilizationEvent(const re_common::ComponentUtilizationEvent& message);

        // Hardware callbacks
        void ProcessSystemStatus(const re_common::SystemStatus& status);
        void ProcessOneTimeSystemInfo(const re_common::SystemInfo& info);


        // Insertion helpers
        //void InsertExperiment(const std::string& name, const std::string& model_name, const std::string metadata="");
        //void InsertExperimentRun(int experiment_id, int job_sequence_nums, double starttime, double endtime=-1, const std::string metadata="");
        //void InsertCluster(int experiment_run_id, const std::string& name);
        //void InsertMachine(int experiment_run_id, const std::string& hostname, const std::string& ip);
        //void InsertNode(int machine_id, int experiment_run_id, const std::string& hostname, const std::string& ip);
        //void InsertComponent(int experiment_run_id, const std::string& name);
        //void InsertComponentInstance(int component_id, int node_id, const std::string& name);
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


        // Members
        std::shared_ptr<DatabaseClient> database_;
        std::shared_ptr<ExperimentTracker> experiment_tracker_;
};

#endif //AGGREGATIONPROTOHANDLER_H