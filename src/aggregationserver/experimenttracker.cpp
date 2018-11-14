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

    if (experiment_map_.count(experiment_id)) {
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
            std::cout << "Added job to new experiment" << std::endl;
        ExperimentInfo new_exp;
        new_exp.name = experiment_name;
        new_exp.job_num = 0;
        new_exp.running = true;
        new_exp.receiver = std::unique_ptr<zmq::ProtoReceiver>(new zmq::ProtoReceiver());
        new_exp.system_handler = std::unique_ptr<SystemEventProtoHandler>(new SystemEventProtoHandler(database_, *this));
        new_exp.model_handler = std::unique_ptr<ModelEventProtoHandler>(new ModelEventProtoHandler(database_, *this));
        new_exp.system_handler->BindCallbacks(*new_exp.receiver);
        new_exp.model_handler->BindCallbacks(*new_exp.receiver);
        

        new_exp.experiment_run_id = database_->InsertValuesUnique(
            "ExperimentRun", 
            {"ExperimentID", "JobNum", "StartTime"},
            {std::to_string(experiment_id), std::to_string(new_exp.job_num), start_time},
            {"JobNum", "ExperimentID"}
        );
        experiment_map_.emplace(experiment_id, std::move(new_exp));

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


ExperimentInfo& ExperimentTracker::GetExperimentInfo(int experiment_id) {
    try {
        return experiment_map_.at(experiment_id);
    } catch (const std::exception&) {
        throw std::invalid_argument("No registered experiment run with ID: '" + std::to_string(experiment_id) + "'");
    }
}

ExperimentInfo& ExperimentTracker::GetExperimentInfo(const std::string& experiment_name){
    for (auto& e_pair: experiment_map_) {
        if (experiment_name == e_pair.second.name) {
            return e_pair.second;
        }
    }
    throw std::invalid_argument("No registered experiment with name: '" + experiment_name + "'");
}

void ExperimentTracker::StartExperimentLoggerReceivers(int experiment_id){
    auto& e = GetExperimentInfo(experiment_id);
    e.receiver->Filter("");
    e.receiver->Start();
}