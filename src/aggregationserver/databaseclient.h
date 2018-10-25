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
    void Connect(const std::string& connection_string){};

    void CreateTable(const std::string& table_name,
        const std::vector<std::pair<std::string,
        std::string> >& columns);

    int InsertValues(const std::string& table_name,
        const std::vector<std::string>& columns,
        const std::vector<std::string>& values);

    int InsertValuesUnique(const std::string& table_name,
        const std::vector<std::string>& columns,
        const std::vector<std::string>& values,
        const std::vector<std::string>& unique_col);

    const pqxx::result GetValues(const std::string table_name,
        const std::vector<std::string>& columns,
        const std::string& query="");

    int GetID(const std::string& table_name,
        const std::string& query);

    std::string EscapeString(const std::string& str);

    const pqxx::result GetPortLifecycleEventInfo(std::string start_time, std::string end_time);

    long long total_unprepared_us=0;
    long long total_prepared_us=0;

    int TestInsertUnprepared(const std::vector<std::string>& plce_values) {
        std::stringstream query_stream;

        query_stream << "INSERT INTO PortLifecycleEvent (PortID, Type, SampleTime) VALUES (";
        for (unsigned int i=0; i<plce_values.size()-1; i++) {
            query_stream << plce_values.at(i) << ',';
        }
        query_stream << plce_values.at(plce_values.size()-1) << ") " << std::endl;
        query_stream << " RETURNING PortLifecycleEventID;";

    std::lock_guard<std::mutex> conn_guard(conn_mutex_);

        try {
            auto start = std::chrono::steady_clock::now();
            pqxx::work transaction(connection_, "InsertPCLETransaction");
            auto&& result = transaction.exec(query_stream.str());
            transaction.commit();
            
            auto us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start);

            //std::cout << "Time to get events: " << double(benchmark_end-benchmark_start)/CLOCKS_PER_SEC << std::endl;
            total_unprepared_us += us.count();
            //std::cout << "Time to insert unprepared events: " << us.count() << std::endl;

            return result.at(0)["PortLifecycleEventID"].as<int>();
            throw std::runtime_error("ID associated with values not found in database query result");
        } catch (const std::exception& e)  {
            std::cerr << e.what() << std::endl;
            throw;
        }
    };
    int TestInsertPrepared(const std::vector<std::string>& plce_values) {
        std::stringstream query_stream;

        query_stream << "INSERT INTO PortLifecycleEvent (PortID, Type, SampleTime) VALUES ($1, $2, $3) " << std::endl;
        query_stream << "RETURNING PortLifecycleEventID;";

    std::lock_guard<std::mutex> conn_guard(conn_mutex_);

        try {
            auto start = std::chrono::steady_clock::now();


            pqxx::work transaction(connection_, "InsertPCLEPreparedTransaction");
            if (!transaction.prepared("InsertPLCE").exists()) {
                connection_.prepare("InsertPLCE", query_stream.str());
            }
            auto&& result = transaction.prepared("InsertPLCE")(plce_values[0])(plce_values[1])(plce_values[2]).exec();
            transaction.commit();
            
            auto us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start);

            //std::cout << "Time to get events: " << double(benchmark_end-benchmark_start)/CLOCKS_PER_SEC << std::endl;
            total_prepared_us += us.count();
            //std::cout << "Time to insert prepared events: " << us.count() << std::endl;

            return result.at(0)["PortLifecycleEventID"].as<int>();
            throw std::runtime_error("ID associated with values not found in database query result");
        } catch (const std::exception& e)  {
            std::cerr << e.what() << std::endl;
            throw;
        }
    };

private:
    const std::string BuildWhereClause(const std::vector<std::string>& cols, const std::vector<std::string>& vals);
    const std::string BuildColTuple(const std::vector<std::string>& cols);


    pqxx::connection connection_;

    std::mutex conn_mutex_;
};

#endif //LOGAN_DATABASECLIENT_H