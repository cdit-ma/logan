
#include <pqxx/pqxx>

#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>

#include <utility>

#include <signal.h>

#include <boost/program_options.hpp>

#include <google/protobuf/util/json_util.h>

#include <zmq/protowriter/protowriter.h>
#include <zmq/protoreceiver/protoreceiver.h>
#include <proto/modelevent/modelevent.pb.h>

#include "utils.h"

#include "aggregationserver.h"
#include "nodemanagerprotohandler.h"
#include "modeleventprotohandler.h"
#include "systemstatusprotohandler.h"
#include "databaseclient.h"
#include "experimenttracker.h"


//Execution execution;
std::unique_ptr<AggregationServer> aggServer;

void signal_handler (int signal_value){
    aggServer->Interrupt();
}


const int success_return_val = 0;
const int error_return_val = 1;

int main(int argc, char** argv) {
    //Handle the SIGINT/SIGTERM signal
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

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

    const std::string connect_address("tcp://127.0.0.1:9000");
    auto writer = std::unique_ptr<zmq::ProtoWriter>(new zmq::ProtoWriter());
    std::cerr << (writer->BindPublisherSocket(connect_address) ? "SUCCESS" : "FAILED") << std::endl;
    //Do me a sleep
    std::this_thread::sleep_for(std::chrono::milliseconds(200));


    aggServer = std::unique_ptr<AggregationServer>(new AggregationServer(connect_address, database_ip, password));
    

    

    
    // Read JSON into protobuf
    std::ifstream json_file("../bin/out.json", std::ifstream::in);
    std::ostringstream json_contents;
    json_contents << json_file.rdbuf();
    auto control_message = std::unique_ptr<NodeManager::ControlMessage>(new NodeManager::ControlMessage());
    google::protobuf::util::JsonStringToMessage(json_contents.str(), control_message.get());

    std::unique_ptr<NodeManager::ControlMessage> cm_copy(new NodeManager::ControlMessage(*control_message));

    // Send the control message off
    writer->PushMessage(std::move(control_message));

    const auto& lifecycleEvents = aggServer->GenerateLifecyclesFromControlMessage(*cm_copy);

    std::cout << "Number of events: " << lifecycleEvents.size() << std::endl;

    std::cout << "stimulating" << std::endl;
    aggServer->StimulatePorts(lifecycleEvents, *writer);
    
    aggServer->Start();

    //receiver->Terminate();
    

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    std::cout << "Shutting down" << std::endl;

    return 0;
}


AggregationServer::AggregationServer(const std::string& receiver_ip,
        const std::string& database_ip,
        const std::string& password) {

    std::stringstream conn_string_stream;
    conn_string_stream << "dbname = postgres user = postgres ";
    conn_string_stream << "password = " << password << " hostaddr = " << database_ip << " port = 5432";
    

    database_client = std::make_shared<DatabaseClient>(conn_string_stream.str());
    auto experiment_tracker = std::make_shared<ExperimentTracker>(database_client);
    nodemanager_protohandler = std::unique_ptr<AggregationProtoHandler>(new NodeManagerProtoHandler(database_client, experiment_tracker));
    modelevent_protohandler = std::unique_ptr<AggregationProtoHandler>(new ModelEventProtoHandler(database_client, experiment_tracker));
    systemstatus_protohandler = std::unique_ptr<AggregationProtoHandler>(new SystemStatusProtoHandler(database_client, experiment_tracker));
    nodemanager_protohandler->BindCallbacks(receiver);
    modelevent_protohandler->BindCallbacks(receiver);
    systemstatus_protohandler->BindCallbacks(receiver);

    receiver.Connect(receiver_ip);
    receiver.Filter("");
    receiver.Start();
}

void AggregationServer::AddLifecycleEventsFromNode(std::vector<re_common::LifecycleEvent>& events,
            const std::string experiment_name, 
            const std::string& hostname,
            const NodeManager::Node& message) {
    
    for (const auto& component : message.components()) {
         AddLifecycleEventsFromComponent(events, experiment_name, hostname, component);
    }

    // Recurse through sub-nodes
    //if (message.nodes_size() == 0) return;
    for (const auto& node : message.nodes()) {
        std::cerr << "We in the loop" << std::endl;
        AddLifecycleEventsFromNode(events, experiment_name, hostname, node);
    }
}

void AggregationServer::AddLifecycleEventsFromComponent(std::vector<re_common::LifecycleEvent>& events,
            const std::string experiment_name, 
            const std::string& hostname,
            const NodeManager::Component& message) {

    std::vector<re_common::LifecycleEvent> new_events;

    auto&& new_lifecycle_event = re_common::LifecycleEvent();
    new_events.emplace_back(new_lifecycle_event);

    for (const auto& port : message.ports()) {
        new_events.emplace_back(GenerateLifecycleEventFromPort(experiment_name, hostname, port));
    }

    for (auto& event : new_events) {
        event.mutable_info()->set_experiment_name(experiment_name);
        FillModelEventComponent(event.mutable_component(), message);
    }
    events.insert(events.end(), new_events.begin(), new_events.end());
}

re_common::LifecycleEvent AggregationServer::GenerateLifecycleEventFromPort(/*std::vector<re_common::LifecycleEvent*>& events,*/
            const std::string experiment_name, 
            const std::string& hostname,
            const NodeManager::Port& message) {

    re_common::LifecycleEvent new_lifecycle_event;

    re_common::Port::Kind kind;
    bool did_parse = re_common::Port::Kind_Parse(message.Kind_Name(message.kind()), &kind);
    if (!did_parse) {
        throw std::runtime_error("Failed to Parse the kind of a port");
    }

    const std::string& middleware = NodeManager::Middleware_Name(message.middleware());
    
    new_lifecycle_event.mutable_port()->set_id(message.info().id());
    new_lifecycle_event.mutable_port()->set_name(message.info().name());
    new_lifecycle_event.mutable_port()->set_type(message.info().type());
    new_lifecycle_event.mutable_port()->set_kind(kind);
    new_lifecycle_event.mutable_port()->set_middleware(middleware);

    return new_lifecycle_event;
}

void AggregationServer::FillModelEventComponent(re_common::Component* component, const NodeManager::Component& nm_component) {
    auto&& location_vec = std::vector<std::string>(nm_component.location().begin(), nm_component.location().end());
    auto&& replication_vec = std::vector<int>(nm_component.replicate_indices().begin(), nm_component.replicate_indices().end());
    std::string full_location = AggServer::GetFullLocation(location_vec, replication_vec);
    
    //component->set_id(nm_component.info().id());
    component->set_id(full_location);   // Needs to be updated to store path more appropriately in ModelEvents
    component->set_name(nm_component.info().name());
    component->set_type(nm_component.info().type());
}

std::vector<re_common::LifecycleEvent> AggregationServer::GenerateLifecyclesFromControlMessage(const NodeManager::ControlMessage& message) {

    std::vector<re_common::LifecycleEvent> events;
    
    for (const auto& node : message.nodes()) {

        const auto& top_level_host_name = node.info().name();

        AddLifecycleEventsFromNode(events, message.experiment_id(), top_level_host_name, node);
    }

    return events;
}

void AggregationServer::StimulatePorts(const std::vector<re_common::LifecycleEvent>& events, zmq::ProtoWriter& writer) {

    try {
        auto&& port_id_results = database_client->GetValues(
            "Port",
            {"PortID", "Name", "Path"}
        );

        // For each port in our database we're going to generate a configured lifecycle event
        for (const auto& port_id_row : port_id_results) {
            int port_id;
            std::string port_name;
            try {
                port_id = port_id_row["PortID"].as<int>();
                port_name = port_id_row["Name"].as<std::string>();
            } catch (const std::exception& e) {
                std::cerr << e.what() << std::endl;
                throw;
            }
            std::string location = port_id_row["Path"].as<std::string>();
            size_t slash_pos = location.find_last_of('/');
            std::string proto_port_id;
            try {
                proto_port_id = location.substr(slash_pos);
            } catch (const std::exception& e) {
                std::cerr << e.what() << std::endl;
                throw;
            }

            for (const auto& event : events) {
                if (!event.has_port()) continue;
                if (event.port().name() == port_name) {
                    auto configured_event = std::unique_ptr<re_common::LifecycleEvent>(new re_common::LifecycleEvent(event));
                    configured_event->set_type(re_common::LifecycleEvent::CONFIGURED);
                    configured_event->mutable_info()->set_timestamp(2.0);
                    bool success = writer.PushMessage(std::move(configured_event));
                    if (!success) {
                        std::cout << "Something went wrong pushing message" << std::endl;
                    }
                }
            }
           
        }
    } catch (const std::exception& e) {
        std::cerr << "Caught an exception retrieving ports: " << e.what() << std::endl;
        throw;
    }
}