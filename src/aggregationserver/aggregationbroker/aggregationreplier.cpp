#include "aggregationreplier.h"

#include <google/protobuf/util/time_util.h>

using google::protobuf::util::TimeUtil;

AggServer::AggregationReplier::AggregationReplier(std::shared_ptr<DatabaseClient> database) :
    database_(database)
{
    const auto&& res = database_->GetPortLifecycleEventInfo(
        "1970-01-01T00:00:00.000000Z",
        "1970-01-01T9:10:23.924073Z",
        {"Port.Path"},
        {"top_level_assembly.1/tx_assembly.1//tx_port"}
    );
    std::cout << "num affected rows" << res.size() << std::endl;
}

std::unique_ptr<AggServer::PortLifecycleResponse>
AggServer::AggregationReplier::ProcessPortLifecycleRequest(const AggServer::PortLifecycleRequest& message) {
    std::cerr << "Received PortLifecycleEventRequest" << std::endl;

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

    std::cout << "Getting PortLifecycleEvents between " << start << " and " << end << std::endl;

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
            port->set_kind(kind);
            port->set_middleware(row["Middleware"].as<std::string>());

            // Build ComponentInstance
            auto component_inst = port->mutable_component_instance();
            component_inst->set_name(row["ComponentInstanceName"].as<std::string>());
            component_inst->set_path(row["ComponentInstancePath"].as<std::string>());

            // Build Component
            auto component = component_inst->mutable_component();
            component->set_name(row["ComponentName"].as<std::string>());

            // Build Node
            auto node = component_inst->mutable_node();
            node->set_hostname(row["NodeHostname"].as<std::string>());
            node->set_ip(row["NodeIP"].as<std::string>());
        }

    } catch (const std::exception& ex) {
        std::cerr << "An exception occurred while querying PortLifecycleEvents:" << ex.what() << std::endl;
        throw;
    }

    return response;
}