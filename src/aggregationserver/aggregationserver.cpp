
#include <pqxx/pqxx>

#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>

#include <signal.h>

#include <boost/program_options.hpp>

#include <google/protobuf/util/json_util.h>

#include <re_common/zmq/protowriter/protowriter.h>
#include <re_common/zmq/protoreceiver/protoreceiver.h>
#include <re_common/proto/modelevent/modelevent.pb.h>

#include <re_common/util/execution.hpp>

#include "aggregationserver.h"
#include "aggregationprotohandler.h"
#include "databaseclient.h"
#include "experimenttracker.h"


Execution execution;

void signal_handler (int signal_value){
    execution.Interrupt();
}


const int success_return_val = 0;
const int error_return_val = 1;

int main(int argc, char** argv) {
    //Handle the SIGINT/SIGTERM signal
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

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


    auto database_client = std::shared_ptr<DatabaseClient>(new DatabaseClient(conn_string_stream.str()));
    auto experiment_tracker = std::shared_ptr<ExperimentTracker>(new ExperimentTracker(database_client));
    auto aggregation_protohandler = std::unique_ptr<AggregationProtoHandler>(new AggregationProtoHandler(database_client, experiment_tracker));

    aggregation_protohandler->BindCallbacks(*receiver);

    
    // Read JSON into protobuf
    std::ifstream json_file("../bin/out.json", std::ifstream::in);
    std::ostringstream json_contents;
    json_contents << json_file.rdbuf();
    auto control_message = new NodeManager::ControlMessage();
    google::protobuf::util::JsonStringToMessage(json_contents.str(), control_message);

    // Send the control message off
    writer->PushMessage(control_message);

    // Create userevents that just contain "TestTable" with 0~4 appended on the end
    /*for(auto i = 0; i < 4; i++){
        auto message = new re_common::UserEvent();
        message->set_message("TestTable" + std::to_string(i));
        writer->PushMessage(message);
    }*/
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    delete control_message;

    execution.Start();

    receiver->Terminate();
    
    std::cout << "Shutting down" << std::endl;

    return 0;
}


void AggregationServer::StimulatePorts(DatabaseClient& database) {
    auto&& port_id_results = database.GetValues(
        "Port",
        {"PortID"}
    );

    auto port_message = new re_common::LifecycleEvent();

    for (const auto& port_id_row : port_id_results) {
        int port_id = port_id_row["PortID"].as<int>();
        port_message->set_type(re_common::LifecycleEvent::CONFIGURED);
        // TODO: Flesh out the rest of the message and send
    }
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