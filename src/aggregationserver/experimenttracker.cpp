#include "experimenttracker.h"

#include <iostream>
#include <sstream>

#include <proto/controlmessage/controlmessage.pb.h>

#include "databaseclient.h"

#include "utils.h"

#include <zmq/protoreceiver/protoreceiver.h>

using std::chrono::milliseconds;
using std::chrono::duration_cast;
using std::chrono::time_point;

ExperimentTracker::ExperimentTracker(std::shared_ptr<DatabaseClient> db_client) :
    database_(db_client) {
    
    }

int ExperimentTracker::RegisterExperimentRun(const std::string& experiment_name, double timestamp) {

    int experiment_run_id = -1;

    std::string start_time = AggServer::FormatTimestamp(timestamp);
    
    std::cout << "Registering experiment with name " << experiment_name << " @ time " << timestamp << std::endl;

    

    const std::vector<std::string> columns = {
        "Name",
        "ModelName"
    };

    const std::vector<std::string> values = {
        experiment_name,
        experiment_name
    };

    std::lock_guard<std::mutex> access_lock(access_mutex_);

    int experiment_id = database_->InsertValuesUnique("Experiment", columns, values, {"Name"});
    std::cout << "Experiment id: " << experiment_id << std::endl;

    if (experiment_run_map_.count(experiment_id)) {
        auto& exp_info = GetExperimentInfo(experiment_id);
        if (!exp_info.running) {
            exp_info.running = true;
            exp_info.job_num += 1;
            std::cout << "Added another job to pre existing experiment: " << exp_info.job_num << std::endl;

            exp_info.experiment_run_id = database_->InsertValuesUnique(
                "ExperimentRun", 
                {"ExperimentID", "JobNum", "StartTime"},
                {std::to_string(experiment_id), std::to_string(exp_info.job_num), start_time},
                {"JobNum", "ExperimentID"}
            );
            return experiment_id;

        } else {
            return exp_info.job_num;
        }
    } else {
        ExperimentRunInfo new_run;
        new_run.name = experiment_name;
        new_run.job_num = 0;
        new_run.running = true;

        new_run.receiver = std::unique_ptr<zmq::ProtoReceiver>(new zmq::ProtoReceiver());
        new_run.system_handler = std::unique_ptr<SystemEventProtoHandler>(new SystemEventProtoHandler(database_, *this, experiment_id));
        new_run.model_handler = std::unique_ptr<ModelEventProtoHandler>(new ModelEventProtoHandler(database_, *this));
        new_run.system_handler->BindCallbacks(*new_run.receiver);
        new_run.model_handler->BindCallbacks(*new_run.receiver);

        new_run.experiment_run_id = database_->InsertValuesUnique(
            "ExperimentRun", 
            {"ExperimentID", "JobNum", "StartTime"},
            {std::to_string(experiment_id), std::to_string(new_run.job_num), start_time},
            {"JobNum", "ExperimentID"}
        );
        experiment_run_map_.emplace(experiment_id, std::move(new_run));

        return experiment_id;
    }

    return experiment_run_id;
}


void ExperimentTracker::RegisterSystemEventProducer(int experiment_id, const std::string& endpoint) {
    std::cerr << "Connecting: " << experiment_id << " TO RegisterSystemEventProducer EVENT: " << endpoint << std::endl;
    GetExperimentInfo(experiment_id).receiver->Connect(endpoint);
}
void ExperimentTracker::RegisterModelEventProducer(int experiment_id, const std::string& endpoint) {
    std::cerr << "Connecting: " << experiment_id << " TO RegisterModelEventProducer EVENT: " << endpoint << std::endl;
    GetExperimentInfo(experiment_id).receiver->Connect(endpoint);
}

int ExperimentTracker::GetCurrentRunJobNum(const std::string& experiment_name) {
    return GetExperimentInfo(experiment_name).job_num;
}

int ExperimentTracker::GetCurrentRunID(const std::string& experiment_name) {
    return GetExperimentInfo(experiment_name).experiment_run_id;
}

int ExperimentTracker::GetCurrentRunID(int experiment_id){
    return GetExperimentInfo(experiment_id).experiment_run_id;
}

void ExperimentTracker::AddNodeIDWithHostname(int experiment_run_id, const std::string& hostname, int node_id) {
    GetExperimentInfo(experiment_run_id).hostname_node_id_cache.emplace(std::make_pair(hostname, node_id));
}

int ExperimentTracker::GetNodeIDFromHostname(int experiment_run_id, const std::string& hostname) {
    auto& exp_info = GetExperimentInfo(experiment_run_id);
    try {
        return exp_info.hostname_node_id_cache.at(hostname);
    } catch (const std::out_of_range& oor_exception) {
        // If we can't find anything then go back to the database
        const auto& node_id = database_->GetID("Node", "Hostname = "+database_->EscapeString(hostname));
        AddNodeIDWithHostname(experiment_run_id, hostname, node_id);
        return node_id;
    }
}

ExperimentRunInfo& ExperimentTracker::GetExperimentInfo(int experiment_id) {
    try {
        return experiment_run_map_.at(experiment_id);
    } catch (const std::exception&) {
        throw std::invalid_argument("No registered experiment run with ID: '" + std::to_string(experiment_id) + "'");
    }
}

ExperimentRunInfo& ExperimentTracker::GetExperimentInfo(const std::string& experiment_name){
    for (auto& e_pair: experiment_run_map_) {
        if (experiment_name == e_pair.second.name) {
            return e_pair.second;
        }
    }
    throw std::invalid_argument("No registered experiment with name: '" + experiment_name + "'");
}

void ExperimentTracker::StartExperimentLoggerReceivers(int experiment_id){
    auto& e = GetExperimentInfo(experiment_id);
    e.receiver->Filter("");
}