#ifndef LOGAN_DATABASECLIENT_H
#define LOGAN_DATABASECLIENT_H

#include <string>
#include <vector>
#include <mutex>

#include <pqxx/pqxx>

#include <iostream>

class DatabaseClient {
public:
    DatabaseClient(const std::string& connection_details);
    ~DatabaseClient();
    void Connect(const std::string& connection_string){};

    void CreateTable(
        const std::string& table_name,
        const std::vector<std::pair<std::string,
        std::string> >& columns
    );

    int InsertValues(
        const std::string& table_name,
        const std::vector<std::string>& columns,
        const std::vector<std::string>& values
    );

    int InsertValuesUnique(
        const std::string& table_name,
        const std::vector<std::string>& columns,
        const std::vector<std::string>& values,
        const std::vector<std::string>& unique_col
    );

    const pqxx::result GetValues(
        const std::string table_name,
        const std::vector<std::string>& columns,
        const std::string& query=""
    );

    int GetID(
        const std::string& table_name,
        const std::string& query
    );

    std::string EscapeString(const std::string& str);

    const pqxx::result GetPortLifecycleEventInfo(
        std::string start_time,
        std::string end_time,
        const std::vector<std::string>& condition_columns,
        const std::vector<std::string>& condition_values
    );

    const pqxx::result GetWorkloadEventInfo(
        std::string start_time,
        std::string end_time,
        const std::vector<std::string>& condition_columns,
        const std::vector<std::string>& condition_values
    );

    const pqxx::result GetCPUUtilInfo(
        std::string start_time,
        std::string end_time,
        const std::vector<std::string>& condition_columns,
        const std::vector<std::string>& condition_values
    );

private:
    const std::string BuildWhereClause(
        const std::vector<std::string>& cols,
        const std::vector<std::string>& vals
    );
    const std::string BuildColTuple(const std::vector<std::string>& cols);

    pqxx::work& AquireBatchedTransaction();
    void ReleaseBatchedTransaction();
    // Aquire batch transaction lock before flushing
    void FlushBatchedTransaction();
    

    pqxx::connection connection_;
    pqxx::connection batched_connection_;

    std::unique_ptr<pqxx::work> batched_transaction;
    unsigned int batched_write_count_; 

    std::mutex conn_mutex_;
    std::mutex batched_transaction_mutex_;
};

#endif //LOGAN_DATABASECLIENT_H