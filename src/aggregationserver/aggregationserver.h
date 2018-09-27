#ifndef LOGAN_AGGREGATIONSERVER_H
#define LOGAN_AGGREGATIONSERVER_H

#include "string"

#include <util/execution.hpp>

class DatabaseClient;

class AggregationProtoHandler;

namespace re_common {
    class LifecycleEvent;
    class Component;
    class Port;
}
namespace NodeManager {
    class ControlMessage;
    class Node;
    class Component;
    class Port;
}

class AggregationServer {
public:

    AggregationServer(const std::string& receiver_ip,
            const std::string& database_ip,
            const std::string& password);

    void LogComponentLifecycleEvent(const std::string& timeofday, const std::string& hostname, int id, int core_id, double core_utilisation);
    
    void StimulatePorts(const std::vector<re_common::LifecycleEvent>& events, zmq::ProtoWriter& writer);

    std::vector<re_common::LifecycleEvent> GenerateLifecyclesFromControlMessage(const NodeManager::ControlMessage& message);
    void AddLifecycleEventsFromNode(std::vector<re_common::LifecycleEvent>& events,
            const std::string experiment_name, 
            const std::string& hostname,
            const NodeManager::Node& message);
    void AddLifecycleEventsFromComponent(std::vector<re_common::LifecycleEvent>& events,
            const std::string experiment_name, 
            const std::string& hostname,
            const NodeManager::Component& message);
    re_common::LifecycleEvent GenerateLifecycleEventFromPort(const std::string experiment_name, 
            const std::string& hostname,
            const NodeManager::Port& message);
    void FillModelEventComponent(re_common::Component* component, const NodeManager::Component& nm_component);


private:
    
    zmq::ProtoReceiver receiver;
    std::shared_ptr<DatabaseClient> database_client;

    std::unique_ptr<AggregationProtoHandler> nodemanager_protohandler;
    std::unique_ptr<AggregationProtoHandler> modelevent_protohandler;
    std::unique_ptr<AggregationProtoHandler> systemstatus_protohandler;
};

#endif 