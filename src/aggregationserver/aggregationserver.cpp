
#include <pqxx/pqxx>

#include <iostream>
#include <sstream>
#include <memory>

#include <boost/program_options.hpp>

#include <re_common/zmq/protowriter/protowriter.h>
#include <re_common/zmq/protoreceiver/protoreceiver.h>
#include <re_common/proto/modelevent/modelevent.pb.h>

#include "aggregationserver.h"
#include "aggregationprotohandler.h"
#include "databaseclient.h"


const int success_return_val = 0;
const int error_return_val = 1;

int main(int argc, char** argv) {

    std::stringstream conn_string_stream;
    conn_string_stream << "dbname = postgres user = postgres ";

    //Variables to store the input parameters
    std::string database_ip;
    std::string password;
    //std::vector<std::string> client_addresses;

    //Parse command line options
    //boost::program_options::options_description desc = boost::program_options::options_description()
    boost::program_options::options_description desc("Aggregation Server Options");
    desc.add_options()("ip-address,i", boost::program_options::value<std::string>(&database_ip)->multitoken()->required(), "address of the postgres database (192.168.1.1)");
    desc.add_options()("password,p", boost::program_options::value<std::string>(&password)->default_value(""), "the password for the database");
    desc.add_options()("help,h", "Display help");

    //Construct a variable_map
    boost::program_options::variables_map vm;

    try{
        //Parse Argument variables
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
        boost::program_options::notify(vm);
    }catch(boost::program_options::error& e) {
        std::cerr << "Arg Error: " << e.what() << std::endl << std::endl;
        std::cout << desc << std::endl;
        return 1;
    }

    conn_string_stream << "password = " << password << " hostaddr = " << database_ip << " port = 5432";

    const std::string connect_address("tcp://127.0.0.1:9000");
    auto writer = std::unique_ptr<zmq::ProtoWriter>(new zmq::ProtoWriter());
    

    auto receiver = std::unique_ptr<zmq::ProtoReceiver>(new zmq::ProtoReceiver());

    std::cerr << (writer->BindPublisherSocket(connect_address) ? "SUCCESS" : "FAILED") << std::endl;
    receiver->Connect(connect_address);
    receiver->Filter("");
    receiver->Start();

    //Do me a sleep
    std::this_thread::sleep_for(std::chrono::milliseconds(100));


    auto database_client = std::unique_ptr<DatabaseClient>(new DatabaseClient(conn_string_stream.str()));
    auto aggregation_protohandler = std::unique_ptr<AggregationProtoHandler>(new AggregationProtoHandler(*database_client));

    aggregation_protohandler->BindCallbacks(*receiver);

    


    for(auto i = 0; i < 10; i++){
        auto message = new re_common::UserEvent();
        message->set_message("TestTable" + std::to_string(i));
        writer->PushMessage(message);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return 0;




    try {
        pqxx::connection connection(conn_string_stream.str());

        if (connection.is_open()) {
            std::cout << "We got ourselves a database" << std::endl;
        } else {
            std::cout << "Couldnt connect to database" << std::endl;
            return error_return_val;
        }

        pqxx::work transaction(connection);
        transaction.exec(
            "CREATE TABLE TestTable (" \
            "TestTableID INT PRIMARY KEY    NOT NULL," \
            "TestColumn             TEXT    NOT NULL)" 
        );
        transaction.commit();

        std::cout << "All work done!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return error_return_val;
    }

    return success_return_val;
}


void AggregationServer::LogCPUStatus(const std::string& timeofday, const std::string& hostname,
                    int sequence_number, int core_id, double core_utilisation) {

    std::stringstream stmt_stream;

    stmt_stream << "INSERT INTO HWCPUStatus (TimeOfDay, Hostname, SequenceNumber, CoreID, CoreUtilisation) VALUES (" <<
        timeofday << ',' <<
        hostname << ',' <<
        sequence_number << ',' <<
        core_id << ',' <<
        core_utilisation <<
        ");";

    try {

    } catch(const std::exception& e) {
        std::cerr << "Failed to log CPU status:\n" << e.what() << std::endl;
    }
}