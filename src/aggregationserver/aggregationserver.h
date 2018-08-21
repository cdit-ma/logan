#ifndef LOGAN_AGGREGATIONSERVER_H
#define LOGAN_AGGREGATIONSERVER_H

#include "string"

class AggregationServer {
public:
    void LogComponentLifecycleEvent(const std::string& timeofday, const std::string& hostname, int id, int core_id, double core_utilisation);
    
    void LogCPUStatus(const std::string& timeofday, const std::string& hostname, int id, int core_id, double core_utilisation);
};

#endif 