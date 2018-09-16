#include "aggregationprotohandler.h"

#include "databaseclient.h"
#include "experimenttracker.h"

AggregationProtoHandler::AggregationProtoHandler(std::shared_ptr<DatabaseClient> db_client, std::shared_ptr<ExperimentTracker> exp_tracker) :
    database_(db_client), experiment_tracker_(exp_tracker) {};
