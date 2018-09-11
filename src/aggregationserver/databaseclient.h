#ifndef LOGAN_DATABASECLIENT_H
#define LOGAN_DATABASECLIENT_H

#include <string>
#include <vector>
#include <utility>

#include <pqxx/pqxx>

typedef std::pair<std::string, std::vector<std::string> > column_results_t;

class DatabaseClient {
public:
    DatabaseClient(const std::string& connection_details);
    void Connect(const std::string& connection_string){};
    void CreateTable(const std::string& table_name,
            const std::vector<std::pair<std::string, std::string> >& columns);
    int InsertValues(const std::string& table_name,
            const std::vector<std::string>& columns,
            const std::vector<std::string>& values);
    int InsertValuesUnique(const std::string& table_name,
            const std::vector<std::string>& columns,
            const std::vector<std::string>& values,
            const std::vector<std::string>& unique_col);
    const pqxx::result& GetValues(const std::string table_name,
            const std::vector<std::string>& columns,
            const std::string& query="");

private:
    const std::string BuildWhereClause(const std::vector<std::string>& cols, const std::vector<std::string>& vals);
    const std::string BuildColTuple(const std::vector<std::string>& cols);


    pqxx::connection connection_;
};

#endif //LOGAN_DATABASECLIENT_H