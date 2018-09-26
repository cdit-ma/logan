#include "experimenttracker.h"

#include <iostream>
#include <sstream>

#include <proto/controlmessage/controlmessage.pb.h>

#include "databaseclient.h"

#include "utils.h"

using std::chrono::milliseconds;
using std::chrono::duration_cast;
using std::chrono::time_point;

ExperimentTracker::ExperimentTracker(std::shared_ptr<DatabaseClient> db_client) :
    database_(db_client) {}

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

    // If we already have this experiment existing
    if (experiment_map_.count(experiment_id)) {
        auto& exp_info = experiment_map_.at(experiment_id);
        if (!exp_info.running) {
            exp_info.running = true;
            exp_info.job_num += 1;

            return database_->InsertValuesUnique(
                "ExperimentRun", 
                {"ExperimentID", "JobNum", "StartTime"},
                {std::to_string(experiment_id), std::to_string(exp_info.job_num), start_time},
                {"JobNum", "ExperimentID"}
            );

        } else {
            return exp_info.job_num;
        }
    } else {
        ExperimentInfo new_exp;
        new_exp.name = experiment_name;
        new_exp.job_num = 0;
        new_exp.running = true;
        experiment_map_.emplace(experiment_id, new_exp);

        return database_->InsertValuesUnique(
            "ExperimentRun", 
            {"ExperimentID", "JobNum", "StartTime"},
            {std::to_string(experiment_id), std::to_string(new_exp.job_num), start_time},
            {"JobNum", "ExperimentID"}
        );
    }

    return experiment_id;
}

int ExperimentTracker::GetCurrentRunJobNum(const std::string& experiment_name) {
    for (const auto& exp_entry: experiment_map_) {
        if (experiment_name == exp_entry.second.name) {
            return exp_entry.second.job_num;
        }
    }
    throw std::runtime_error("Unable to return current job number for an unrecognised experiment name");
}

int ExperimentTracker::GetCurrentRunID(const std::string& experiment_name) {
    for (const auto& exp_entry: experiment_map_) {
        if (experiment_name == exp_entry.second.name) {
            return exp_entry.first;
        }
    }
    throw std::runtime_error("Unable to return current ExperimentRunID for an unrecognised experiment name");
}