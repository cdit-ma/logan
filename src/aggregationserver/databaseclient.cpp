#include "databaseclient.h"

#include <sstream>
#include <iostream>

#include <algorithm>

DatabaseClient::DatabaseClient(const std::string& connection_details) : 
    connection_(connection_details)
{

}

void DatabaseClient::CreateTable(const std::string& table_name,
                                std::vector< std::pair<std::string, std::string> > columns) {
    std::stringstream query_stream;

    query_stream << "CREATE TABLE " << table_name << "(" << std::endl;
    /*for (const auto& column_def : columns) {
        query_stream << column_def.first << " " << column_def.second << "," << std::endl;
    }*/
    const auto& last_col = std::prev(columns.end());
    std::for_each(columns.begin(), last_col,
        [&query_stream](const std::pair<std::string,std::string>& column_def) {
        query_stream << column_def.first << " " << column_def.second << ',' << std::endl;
    });
    query_stream << last_col->first << " " << last_col->second << std::endl;
    query_stream << ")" << std::endl;

    try {
        pqxx::work transaction(connection_, "CreateTableTransaction");
        transaction.exec(query_stream.str());
        transaction.commit();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        throw;
    }
    
}

void DatabaseClient::InsertValues(const std::string& table_name,
                            std::vector<std::string> columns, std::vector<std::string> values) {

    std::stringstream query_stream;

    query_stream << "INSERT INTO " << table_name;
    if (columns.size() > 0) {
        query_stream << " (";
        for (int i=0; i < columns.size()-1; i++) {
            query_stream << columns.at(i) << ',';
        }
        query_stream << columns.at(columns.size()) << ')';
    }

    query_stream << std::endl << " VALUES (";
    for (int i=0; i<values.size()-1; i++) {
        query_stream << values.at(i) << ',';
    }
    query_stream << values.at(values.size()) << ");" << std::endl;


    try {
        pqxx::work transaction(connection_, "InsertValuesTransaction");
        transaction.exec(query_stream.str());
        transaction.commit();
    } catch (const std::exception& e)  {
        std::cerr << e.what() << std::endl;
    }
}

void DatabaseClient::GetValues(const std::string table_name,
                            std::vector<std::string> columns, const std::string& query) {

    try {
        std::stringstream query_stream;
        pqxx::work transaction(connection_, "GetValuesTransaction");

        query_stream << "SELECT ";
        if (columns.size() > 0) {
            for (int i=0; i < columns.size()-1; i++) {
                query_stream << transaction.quote(columns.at(i)) << ", ";
            }
            query_stream << transaction.quote(columns.at(columns.size()));
        }
        query_stream << std::endl;
        query_stream << " FROM " << table_name << std::endl;
        if (query != "") {
            query_stream << " WHERE " << query;
        }
        query_stream << ";" << std::endl;

        const auto& result = transaction.exec(query_stream.str());
        transaction.commit();

        //result.
    } catch (const std::exception& e)  {
        std::cerr << e.what() << std::endl;
    }
}