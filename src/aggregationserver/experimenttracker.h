#ifndef EXPERIMENTTRACKER_H
#define EXPERIMENTTRACKER_H

#include <string>
#include <exception>
#include <memory>
#include <mutex>

#include <map>

#include <zmq/protoreceiver/protoreceiver.h>

#include "systemeventprotohandler.h"
#include "modeleventprotohandler.h"

class DatabaseClient;

namespace NodeManager {
    class ControlMessage;
}

struct ExperimentInfo {
    std::string name;
    int job_num;
    bool running;
    int experiment_run_id;
    std::unique_ptr<zmq::ProtoReceiver> receiver;
    std::unique_ptr<SystemEventProtoHandler> system_handler;
    std::unique_ptr<ModelEventProtoHandler> model_handler;

    std::map<std::string, int> hostname_node_id_cache;
};

class ExperimentTracker {
public:
    ExperimentTracker(std::shared_ptr<DatabaseClient> db_client);
    int RegisterExperimentRun(const std::string& experiment_name, double timestamp);

    void RegisterSystemEventProducer(int experiment_id, const std::string& endpoint);
    void RegisterModelEventProducer(int experiment_id, const std::string& endpoint);

    int GetCurrentRunJobNum(const std::string& experiment_name);
    int GetCurrentRunID(const std::string& experiment_name);
    int GetCurrentRunID(int experiment_id);

    void AddNodeIDWithHostname(int experiment_id, const std::string& hostname, int node_id);
    int GetNodeIDFromHostname(int experiment_id, const std::string& hostname);

    void StartExperimentLoggerReceivers(int experiment_id);
private:
    ExperimentInfo& GetExperimentInfo(int experiment_id);
    ExperimentInfo& GetExperimentInfo(const std::string& experiment_name);
    

    std::shared_ptr<DatabaseClient> database_;
    std::map<int, ExperimentInfo> experiment_map_;

    std::mutex access_mutex_;
};

#endif