#include "databaseclient.h"

#include <sstream>
#include <iostream>

#include <algorithm>

DatabaseClient::DatabaseClient(const std::string& connection_details) : 
    connection_(connection_details)
{

}

void DatabaseClient::CreateTable(const std::string& table_name,
                                const std::vector< std::pair<std::string, std::string> >& columns) {
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

    std::lock_guard<std::mutex> conn_guard(conn_mutex_);

    try {
        pqxx::work transaction(connection_, "CreateTableTransaction");
        transaction.exec(query_stream.str());
        transaction.commit();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        throw;
    }
    
}

int DatabaseClient::InsertValues(const std::string& table_name,
                            const std::vector<std::string>& columns, const std::vector<std::string>& values) {

    std::stringstream query_stream;
    std::string id_column = table_name + "ID";
    int id_value = -1;

    query_stream << "INSERT INTO " << table_name;
    if (columns.size() > 0) {
        query_stream << " (";
        for (int i=0; i < columns.size()-1; i++) {
            query_stream << columns.at(i) << ',';
        }
        query_stream << columns.at(columns.size()-1) << ')';
    }

    query_stream << std::endl << " VALUES (";
    for (int i=0; i<values.size()-1; i++) {
        query_stream << values.at(i) << ',';
    }
    query_stream << values.at(values.size()-1) << ")" << std::endl;
    query_stream << "RETURNING " << id_column << ";" << std::endl;

    std::lock_guard<std::mutex> conn_guard(conn_mutex_);

    try {
        pqxx::work transaction(connection_, "InsertValuesTransaction");
        auto&& result = transaction.exec(query_stream.str());
        transaction.commit();

        std::string lower_id_column(id_column);
        std::transform(lower_id_column.begin(), lower_id_column.end(), lower_id_column.begin(), ::tolower);
        int id_colnum = result.column_number(lower_id_column);

        for (const auto& row : result) {
            for (int colnum=0; colnum < row.size(); colnum++) {
                if (colnum == id_colnum) {
                    id_value = row[colnum].as<int>();
                    return id_value;
                }
            }
        }
    } catch (const std::exception& e)  {
        std::cerr << e.what() << std::endl;
        throw;
    }
}

int DatabaseClient::InsertValuesUnique(const std::string& table_name,
                            const std::vector<std::string>& columns, const std::vector<std::string>& values, const std::vector<std::string>& unique_cols) {

    std::string id_column = table_name + "ID";
    int id_value = -1;
    std::vector<std::string> unique_vals = std::vector<std::string>(unique_cols.size());

    std::stringstream query_stream;

    query_stream << "WITH i AS (" << std::endl;
    query_stream << "INSERT INTO " << table_name;
    if (columns.size() > 0) {
        query_stream << " (";
        for (int i=0; i < columns.size()-1; i++) {
            query_stream << columns.at(i) << ',';
            for (int j=0; j<unique_cols.size(); j++) {
                if (unique_cols.at(j) == columns.at(i)) {
                    unique_vals.at(j) = values.at(i);
                }
            }
        }
        int last_col = columns.size()-1;
        query_stream << columns.at(last_col) << ')';
        for (int j=0; j<unique_cols.size(); j++) {
            if (unique_cols.at(j) == columns.at(last_col)) {
                unique_vals.at(j) = values.at(last_col);
            }
        }
    }

    query_stream << std::endl << " VALUES (";
    for (int i=0; i<values.size()-1; i++) {
        query_stream << connection_.quote(values.at(i)) << ",";
    }
    query_stream << connection_.quote(values.at(values.size()-1)) << ")" << std::endl;

    query_stream << "ON CONFLICT " << BuildColTuple(unique_cols) << " DO UPDATE" << std::endl;
    //query_stream << "ON CONFLICT  ON CONSTRAINT " << BuildColTuple(unique_cols) << " DO UPDATE" << std::endl;
    query_stream << "SET " << id_column << " = -1 WHERE FALSE" << std::endl;

    query_stream << "RETURNING " << id_column << ")" << std::endl;

    query_stream << "SELECT " << id_column << " FROM i" << std::endl;
    query_stream << "UNION ALL" << std::endl;
    query_stream << "SELECT " << id_column << " FROM " << table_name << " " << BuildWhereClause(unique_cols, unique_vals) << std::endl;
    //query_stream << "SELECT " << id_column << " FROM " << table_name << " WHERE " << unique_col << " = " << connection_.quote(unique_val) << std::endl;
    query_stream << "LIMIT 1" << std::endl;


    //std::cout << query_stream.str() << std::endl;

    std::lock_guard<std::mutex> conn_guard(conn_mutex_);

    try {
        pqxx::work transaction(connection_, "InsertValuesUniqueTransaction");
        auto&& result = transaction.exec(query_stream.str());
        transaction.commit();
        
        std::string lower_id_column(id_column);
        std::transform(lower_id_column.begin(), lower_id_column.end(), lower_id_column.begin(), ::tolower);
        int id_colnum = result.column_number(lower_id_column);

        for (const auto& row : result) {
            for (int colnum=0; colnum < row.size(); colnum++) {
                if (colnum == id_colnum) {
                    id_value = row.at(colnum).as<int>();
                    return id_value;
                }
            }
        }
    } catch (const std::exception& e)  {
        std::cerr << "An exception occurred while inserting values uniquely: " << e.what() << std::endl;
        throw;
    }
    /*

    WITH ins AS (
      INSERT INTO city
            (name_city , country , province , region , cap , nationality ) 
      VALUES(name_city1, country1, province1, region1, cap1, nationality1)
      ON     CONFLICT (name_city) DO UPDATE
      SET    name_city = NULL WHERE FALSE  -- never executed, but locks the row!
      RETURNING id_city
      )
    SELECT id_city FROM ins
    UNION  ALL
    SELECT id_city FROM city WHERE name_city = name_city1  -- only executed if no INSERT
    LIMIT  1;

    */

   return id_value;
}

const pqxx::result DatabaseClient::GetValues(const std::string table_name,
                            const std::vector<std::string>& columns, const std::string& query) {
    std::stringstream query_stream;

    query_stream << "SELECT ";
    if (columns.size() > 0) {
        for (int i=0; i < columns.size()-1; i++) {
            query_stream << /*connection_.quote(*/columns.at(i)/*)*/ << ", ";
        }
        query_stream << /*connection_.quote(*/columns.at(columns.size()-1)/*)*/;
    }
    query_stream << std::endl;
    query_stream << " FROM " << table_name << std::endl;
    if (query != "") {
        query_stream << " WHERE (" << query << ")";
    }
    query_stream << ";" << std::endl;

    std::lock_guard<std::mutex> conn_guard(conn_mutex_);

    try {
        pqxx::work transaction(connection_, "GetValuesTransaction");
        const auto& pg_result = transaction.exec(query_stream.str());
        transaction.commit();

        return pg_result;
    } catch (const std::exception& e)  {
        std::cerr << e.what() << std::endl;
        throw;
    }
}

int DatabaseClient::GetID(const std::string& table_name, const std::string& query) {
    std::string&& id_column_name = table_name+"ID";

    const auto& results = GetValues(
        table_name,
        {id_column_name},
        query
    );

    if (results.empty()) {
        throw std::runtime_error("No matching ID found in table "+table_name+" for condition :\n"+query);
    }

    if (results.size() > 1) {
        throw std::runtime_error("More than one "+id_column_name+" appears to satisfy the following condition:\n" + query + "\n" + std::to_string(results.size()));
    }

    int id_column_num = results.column_number(id_column_name);
    for (const auto& row : results) {
        return row.at(id_column_num).as<int>();
    }
}

std::string DatabaseClient::EscapeString(const std::string& str) {
    return connection_.quote(str);
}

const std::string DatabaseClient::BuildWhereClause(const std::vector<std::string>& cols, const std::vector<std::string>& vals) {
    std::stringstream where_stream;

    where_stream << "WHERE (";
    
    auto col = cols.begin();
    auto val = vals.begin();
    while (col != cols.end() || val != vals.end()) {
        where_stream << *col << " = '" << *val << "'";
        col++;
        val++;
        if (col != cols.end()) {
            where_stream << " AND ";
        }
    }

    where_stream << ")";

    return where_stream.str();
}

const std::string DatabaseClient::BuildColTuple(const std::vector<std::string>& cols) {
    std::stringstream tuple_stream;

    tuple_stream << "(";
    
    auto col = cols.begin();
    if (col == cols.end()){
        tuple_stream << ")";
        return tuple_stream.str();
    };

    tuple_stream << *col;
    col++;

    while (col != cols.end()) {
        tuple_stream << ", " << *col;
        col++;
    }

    tuple_stream << ")";

    return tuple_stream.str();
}