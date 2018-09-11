#ifndef EXPERIMENTTRACKER_H
#define EXPERIMENTTRACKER_H

#include <string>
#include <exception>
#include <memory>
#include <mutex>

#include <map>

class DatabaseClient;

namespace NodeManager {
    class ControlMessage;
}

struct ExperimentInfo {
    std::string name;
    int job_num;
    bool running;
};

class ExperimentTracker {
public:
    ExperimentTracker(std::shared_ptr<DatabaseClient> db_client);
    int RegisterExperimentRun(const std::string& experiment_name, double timestamp);

    int GetCurrentRunJobNum(const std::string& experiment_name);
    int GetCurrentRunID(const std::string& experiment_name);

private:
    std::shared_ptr<DatabaseClient> database_;
    std::map<int, ExperimentInfo> experiment_map_;

    std::mutex access_mutex_;
};

#endif