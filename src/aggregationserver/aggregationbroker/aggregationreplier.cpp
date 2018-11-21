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
    const auto&& res = database_->GetWorkloadEventInfo(
        "1970-01-01T00:00:00.000000Z",
        "2020-01-01T9:10:23.924073Z",
        {"WorkerInstance.Path"},
        {"TestAssembly.1/ReceiverInstance/Utility_Worker"}
    );
    std::cout << "num affected rows" << res.size() << std::endl;
}

void AggServer::AggregationReplier::RegisterCallbacks() {
    RegisterProtoCallback<AggServer::PortLifecycleRequest, AggServer::PortLifecycleResponse>(
        "GetPortLifecycle",
        std::bind(&AggregationReplier::ProcessPortLifecycleRequest, this, std::placeholders::_1)
    );
    RegisterProtoCallback<AggServer::WorkloadRequest, AggServer::WorkloadResponse>(
        "GetWorkload",
        std::bind(&AggregationReplier::ProcessWorkloadEventRequest, this, std::placeholders::_1)
    );
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
                throw std::runtime_error("Unable to parse WorkloadEventType from string: "+type_string);
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