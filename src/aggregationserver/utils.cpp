#include "utils.h"

#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>

std::string AggServer::GetFullLocation(const std::vector<std::string>& locations, const std::vector<int>& replication_indices) {
    std::string full_location;

    auto loc_iter = locations.begin();
    auto rep_iter = replication_indices.begin();

    while (loc_iter != locations.end() || rep_iter != replication_indices.end()) {
        full_location.append(*loc_iter).append(".").append(std::to_string(*rep_iter)).append("/");
        loc_iter++;
        rep_iter++;
    }

    return full_location;
}

std::string AggServer::FormatTimestamp(double timestamp) {
    //std::string timestamp_statement = std::string("to_timestamp('") + std::to_string(timestamp) + "')::timestamp";
    std::chrono::milliseconds start_time((int)(timestamp*1000));
    time_t st = start_time.count();

    //std::string timestamp_statement = std::to_string(std::put_time(start_time.count()));

    std::stringstream time_stream;
    //time_stream << std::ctime(time_point<std::chrono::system_clock>(duration_cast<milliseconds>(start_time)));
    //time_stream << std::put_time(std::gmtime(&st), "%c %Z");
    time_stream << std::ctime(&st);

    return time_stream.str();
}