#ifndef AGGSERVER_UTILS_H
#define AGGSERVER_UTILS_H

#include <string>
#include <vector>

namespace AggServer {
    // String builder functions
    std::string GetFullLocation(const std::vector<std::string>& locations, const std::vector<int>& replication_indices);
    std::string FormatTimestamp(double timestamp);
}

#endif