#include "aggregationreplier.h"

#include <google/protobuf/util/time_util.h>

using google::protobuf::util::TimeUtil;

AggServer::AggregationReplier::AggregationReplier(std::shared_ptr<DatabaseClient> database) :
    database_(database)
{
    RegisterCallbacks();
    // const auto&& res = database_->GetPortLifecycleEventInfo(
    //     "1970-01-01T00:00:00.000000Z",
    //     "1970-01-01T9:10:23.924073Z",
    //     {"Port.Path"},
    //     {"ComponentAssembly.0//SubscriberPort"}
    // );
    /*const auto&& res = database_->GetWorkloadEventInfo(
        "1970-01-01T00:00:00.000000Z",
        "2020-01-01T9:10:23.924073Z",
        {"WorkerInstance.Path"},
        {"TestAssembly.1/ReceiverInstance/Utility_Worker"}
    );
    std::cout << "num affected rows" << res.size() << std::endl;
    */
}

void AggServer::AggregationReplier::RegisterCallbacks() {
    RegisterProtoCallback<ExperimentRunRequest, ExperimentRunResponse>(
        "GetExperimentRuns",
        std::bind(&AggregationReplier::ProcessExperimentRunRequest, this, std::placeholders::_1)
    );
    RegisterProtoCallback<ExperimentStateRequest, ExperimentStateResponse>(
        "GetExperimentState",
        std::bind(&AggregationReplier::ProcessExperimentStateRequest, this, std::placeholders::_1)
    );
    RegisterProtoCallback<PortLifecycleRequest, PortLifecycleResponse>(
        "GetPortLifecycle",
        std::bind(&AggregationReplier::ProcessPortLifecycleRequest, this, std::placeholders::_1)
    );
    RegisterProtoCallback<WorkloadRequest, WorkloadResponse>(
        "GetWorkload",
        std::bind(&AggregationReplier::ProcessWorkloadEventRequest, this, std::placeholders::_1)
    );
}

std::unique_ptr<AggServer::ExperimentRunResponse>
AggServer::AggregationReplier::ProcessExperimentRunRequest(const AggServer::ExperimentRunRequest& message) {
    std::unique_ptr<ExperimentRunResponse> response(new ExperimentRunResponse());

    const std::string& exp_name = message.experiment_name();
    std::vector<std::pair<std::string, int> > exp_name_id_pairs;

    std::stringstream condition_stream;
    if (exp_name == "") {
        // If no name is provided ask the database for the list of all names and ExperimentIDs
        const auto& results = database_->GetValues(
            "Experiment",
            {"Name", "ExperimentID"}
        );
        for (const auto& row : results) {
            const auto& name = row.at("Name").as<std::string>();
            const auto& id = row.at("ExperimentID").as<int>();
            exp_name_id_pairs.emplace_back(std::make_pair(name, id));
        }
    } else {
        int id = database_->GetID(
            "Experiment",
            exp_name
        );
        exp_name_id_pairs.emplace_back(std::make_pair(exp_name, id));
    }

    for (const auto& pair : exp_name_id_pairs) {
        const auto& name = pair.first;
        const auto& id = pair.second;
        auto exp_info = response->add_experiments();
        exp_info->set_name(name);

        condition_stream << "ExperimentID = " << id;

        const auto& results = database_->GetValues(
            "ExperimentRun",
            {"ExperimentID", "JobNum", "StartTime", "EndTime"},
            condition_stream.str()
        );

        for (const auto& row : results) {
            auto run = exp_info->add_runs();
            run->set_experiment_run_id(row.at("ExperimentRunID").as<int>());
            run->set_job_num(row.at("JobNum").as<int>());
            TimeUtil::FromString(row.at("StartTime").as<std::string>(), run->mutable_start_time());
            try {
                std::string end_time_str = row.at("EndTime").as<std::string>();
                TimeUtil::FromString(end_time_str, run->mutable_end_time());
            } catch (const std::exception& e) {
                std::cerr << "An exception occurred while querying experiment run end time, likely due to null: " << e.what() << "\n";
            }
        }
    }

    return response;
}

std::unique_ptr<AggServer::ExperimentStateResponse>
AggServer::AggregationReplier::ProcessExperimentStateRequest(const AggServer::ExperimentStateRequest& message) {
    throw std::runtime_error("ExperimentStateRequest handling not yet implemented");
}

std::unique_ptr<AggServer::PortLifecycleResponse>
AggServer::AggregationReplier::ProcessPortLifecycleRequest(const AggServer::PortLifecycleRequest& message) {

    std::unique_ptr<AggServer::PortLifecycleResponse> response = std::unique_ptr<AggServer::PortLifecycleResponse>(
        new AggServer::PortLifecycleResponse()
    );

    std::string start, end;

    // Start time defaults to 0 if not specified
    if (message.time_interval_size() >= 1) {
        start = TimeUtil::ToString(message.time_interval()[0]);
    } else {

        start = TimeUtil::ToString(TimeUtil::SecondsToTimestamp(0));
    }

    // End time defaults to 0 if not specified
    if (message.time_interval_size() >= 2) {
        end = TimeUtil::ToString(message.time_interval()[1]);
    } else {
        end = TimeUtil::ToString(TimeUtil::SecondsToTimestamp(0));
    }

    // Get filter conditions
    std::vector<std::string> condition_cols;
    std::vector<std::string> condition_vals;
    condition_cols.emplace_back("Component.ExperimentRunID");
    condition_vals.emplace_back(std::to_string(message.experiment_run_id()));
    for(const auto& port_path : message.port_paths()) {
        condition_cols.emplace_back("Port.Path");
        condition_vals.emplace_back(port_path);
    }
    for(const auto& component_inst_path : message.component_instance_paths()) {
        condition_cols.emplace_back("ComponentInstance.Path");
        condition_vals.emplace_back(component_inst_path);
    }
    for(const auto& component_name : message.component_names()) {
        condition_cols.emplace_back("Component.Name");
        condition_vals.emplace_back(component_name);
    }

    try {
        const pqxx::result res = database_->GetPortLifecycleEventInfo(start, end, condition_cols, condition_vals);

        for (const auto& row : res) {
            auto event = response->add_events();

            // Build Event
            auto&& type_int = row["Type"].as<int>();
            event->set_type((AggServer::LifecycleType)type_int);
            auto&& timestamp_str = row["SampleTime"].as<std::string>();
            bool did_parse = TimeUtil::FromString(timestamp_str, event->mutable_time());
            if (!did_parse) {
                throw std::runtime_error("Failed to parse SampleTime field from string: "+timestamp_str);
            }

            // Build Port
            auto port = event->mutable_port();
            port->set_name(row["PortName"].as<std::string>());
            port->set_path(row["PortPath"].as<std::string>());
            Port::Kind kind;
            bool did_parse_lifecycle = AggServer::Port::Kind_Parse(row["PortKind"].as<std::string>(), &kind);
            if (!did_parse_lifecycle) {
                throw std::runtime_error("Failed to parse Lifecycle Kind field from string: "+row["PortKind"].as<std::string>());
            }
            port->set_kind(kind);
            port->set_middleware(row["Middleware"].as<std::string>());
            port->set_graphml_id(row["PortGraphmlID"].as<std::string>());

        }

    } catch (const std::exception& ex) {
        std::cerr << "An exception occurred while querying PortLifecycleEvents:" << ex.what() << std::endl;
        throw;
    }

    return response;
}


std::unique_ptr<AggServer::WorkloadResponse>
AggServer::AggregationReplier::ProcessWorkloadEventRequest(const AggServer::WorkloadRequest& message) {

    std::unique_ptr<AggServer::WorkloadResponse> response = std::unique_ptr<AggServer::WorkloadResponse>(
        new AggServer::WorkloadResponse()
    );

    std::string start, end;

    // Start time defaults to 0 if not specified
    if (message.time_interval_size() >= 1) {
        start = TimeUtil::ToString(message.time_interval()[0]);
    } else {

        start = TimeUtil::ToString(TimeUtil::SecondsToTimestamp(0));
    }

    // End time defaults to 0 if not specified
    if (message.time_interval_size() >= 2) {
        end = TimeUtil::ToString(message.time_interval()[1]);
    } else {
        end = TimeUtil::ToString(TimeUtil::SecondsToTimestamp(0));
    }

    // Get filter conditions
    std::vector<std::string> condition_cols;
    std::vector<std::string> condition_vals;
    condition_cols.emplace_back("Component.ExperimentRunID");
    condition_vals.emplace_back(std::to_string(message.experiment_run_id()));
    for(const auto& worker_path : message.worker_paths()) {
        condition_cols.emplace_back("Worker.Path");
        condition_vals.emplace_back(worker_path);
    }
    for(const auto& component_inst_path : message.component_instance_paths()) {
        condition_cols.emplace_back("ComponentInstance.Path");
        condition_vals.emplace_back(component_inst_path);
    }
    for(const auto& component_name : message.component_names()) {
        condition_cols.emplace_back("Component.Name");
        condition_vals.emplace_back(component_name);
    }

    try {
        const pqxx::result res = database_->GetWorkloadEventInfo(start, end, condition_cols, condition_vals);

        for (const auto& row : res) {
            auto event = response->add_events();

            // Build Event
            WorkloadEvent::WorkloadEventType type;
            std::string type_string = row["Type"].as<std::string>();
            bool did_parse_workloadtype = WorkloadEvent::WorkloadEventType_Parse(type_string, &type);
            if (!did_parse_workloadtype) {
                // Workaround for string mismatch due to Windows being unable to handle ERROR as a name (LOG-94)
                if (type_string == "ERROR") {
                    type = WorkloadEvent::ERROR_EVENT;
                } else {
                    throw std::runtime_error("Unable to parse WorkloadEventType from string: "+type_string);
                }
            }
            event->set_type(type);
            //event->set_type((AggServer::WorkloadEvent::WorkloadEventType)type_int);
            auto&& timestamp_str = row["SampleTime"].as<std::string>();
            bool did_parse = TimeUtil::FromString(timestamp_str, event->mutable_time());
            if (!did_parse) {
                throw std::runtime_error("Failed to parse SampleTime field from string: "+timestamp_str);
            }
            event->set_function_name(row["FunctionName"].as<std::string>());
            event->set_args(row["Arguments"].as<std::string>());

            // Build Port
            auto worker_inst = event->mutable_worker_inst();
            worker_inst->set_name(row["WorkerInstanceName"].as<std::string>());
            worker_inst->set_path(row["WorkerInstancePath"].as<std::string>());
            worker_inst->set_graphml_id(row["WorkerInstanceGraphmlID"].as<std::string>());

        }

    } catch (const std::exception& ex) {
        std::cerr << "An exception occurred while querying PortLifecycleEvents:" << ex.what() << std::endl;
        throw;
    }

    return response;
}