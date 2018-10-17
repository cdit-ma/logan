#include "aggregationreplier.h"

#include <google/protobuf/util/time_util.h>

using google::protobuf::util::TimeUtil;

AggServer::AggregationReplier::AggregationReplier(std::shared_ptr<DatabaseClient> database) :
    database_(database)
{

}

std::unique_ptr<AggServer::PortLifecycleResponse>
AggServer::AggregationReplier::ProcessPortLifecycleRequest(const AggServer::PortLifecycleRequest& message) {
    std::cerr << "HELLO" << std::endl;

    std::unique_ptr<AggServer::PortLifecycleResponse> response = std::unique_ptr<AggServer::PortLifecycleResponse>(
        new AggServer::PortLifecycleResponse()
    );


    time_t start_time;
    time_t end_time;
    if (message.time_interval_size() >= 1) {
        start_time = TimeUtil::TimestampToTimeT(message.time_interval()[0]);
    }

    if (message.time_interval_size() >= 2) {
        end_time = TimeUtil::TimestampToTimeT(message.time_interval()[1]);
    }

    try {
        const pqxx::result res = database_->GetValues(
            "PortLifecycleEvent",
            {"PortID", "Type", "to_char((sampletime::timestamp), 'YYYY-MM-DD\"T\"HH24:MI:SS.US\"Z\"') AS SampleTime"}
        );

        for (const auto& row : res) {
            auto event = response->add_events();

            auto&& type_int = row["Type"].as<int>();
            event->set_type((AggServer::LifecycleType)type_int);

            auto&& timestamp_str = row["SampleTime"].as<std::string>();
            bool did_parse = TimeUtil::FromString(timestamp_str, event->mutable_time());
            if (!did_parse) {
                throw std::runtime_error("Failed to parse SampleTime field from string: "+timestamp_str);
            }

        }

    } catch (const std::exception& ex) {
        std::cerr << "An exception occurred while querying PortLifecycleEvents:" << ex.what() << std::endl;
        throw;
    }

    return response;
}