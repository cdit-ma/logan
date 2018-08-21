#ifndef LOGAN_DATABASECLIENT_H
#define LOGAN_DATABASECLIENT_H

#include <string>
#include <vector>
#include <utility>

#include <pqxx/pqxx>

class DatabaseClient {
public:
    DatabaseClient(const std::string& connection_details);
    virtual void Connect(const std::string& connection_string){};
    virtual void CreateTable(const std::string& table_name,
                            std::vector<std::pair<std::string, std::string> > columns);
    virtual void InsertValues(const std::string& table_name,
                            std::vector<std::string> columns, std::vector<std::string> values);
    virtual void GetValues(const std::string table_name,
                            std::vector<std::string> columns, const std::string& query="");

private:
    pqxx::connection connection_;
};

#endif //LOGAN_DATABASECLIENT_H