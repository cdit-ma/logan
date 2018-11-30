#ifndef LOGAN_AGGREGATIONSERVER_H
#define LOGAN_AGGREGATIONSERVER_H

#include "string"

#include <util/execution.hpp>

#include <zmq/protorequester/protorequester.hpp>
#include "experimenttracker.h"

class DatabaseClient;

class AggregationProtoHandler;


namespace ModelEvent {
    class LifecycleEvent;
    class Component;
    class Port;
}
namespace NodeManager {
    class ControlMessage;
    class Container;
    class Node;
    class Component;
    class Port;
}

class AggregationServer {
public:

    AggregationServer(const std::string& receiver_ip,
            const std::string& database_ip,
            const std::string& password,
            const std::string& environment_endpoint);
    
    void StimulatePorts(const std::vector<ModelEvent::LifecycleEvent>& events, zmq::ProtoWriter& writer);

    std::vector<ModelEvent::LifecycleEvent> GenerateLifecyclesFromControlMessage(const NodeManager::ControlMessage& message);
    void AddLifecycleEventsFromNode(std::vector<ModelEvent::LifecycleEvent>& events,
            const std::string experiment_name, 
            const std::string& hostname,
            const NodeManager::Node& message);
    void AddLifecycleEventsFromContainers(std::vector<ModelEvent::LifecycleEvent>& events,
            const std::string& experiment_name, 
            const std::string& hostname,
            const NodeManager::Container& message);
    void AddLifecycleEventsFromComponent(std::vector<ModelEvent::LifecycleEvent>& events,
            const std::string& experiment_name, 
            const std::string& hostname,
            const NodeManager::Component& message);
    ModelEvent::LifecycleEvent GenerateLifecycleEventFromPort(const std::string experiment_name, 
            const std::string& hostname,
            const NodeManager::Port& message);
    void FillModelEventComponent(ModelEvent::Component* component, const NodeManager::Component& nm_component);


private:
    
    zmq::ProtoReceiver receiver;
    std::shared_ptr<DatabaseClient> database_client;

    std::unique_ptr<zmq::ProtoRequester> env_requester;
    std::unique_ptr<ExperimentTracker>  experiment_tracker;

    std::unique_ptr<AggregationProtoHandler> nodemanager_protohandler;
};

#endif 