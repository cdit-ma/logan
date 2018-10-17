
#include "aggregationbroker.h"

#include <util/execution.hpp>

#include <boost/program_options.hpp>

#include <iostream>
#include <string>
#include <signal.h>


Execution execution;

void signal_handler (int signal_value){
    execution.Interrupt();
}

const int success_return_val = 0;
const int error_return_val = 1;


int main(int argc, char** argv) {
    // Handle the SIGINT/SIGTERM signal
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Variables to store the input parameters
    std::string database_ip;
    std::string password;

    // Parse command line options
    boost::program_options::options_description desc("Aggregation Server Options");
    desc.add_options()("ip-address,i", boost::program_options::value<std::string>(&database_ip)->multitoken()->required(), "address of the postgres database (192.168.1.1)");
    desc.add_options()("password,p", boost::program_options::value<std::string>(&password)->default_value(""), "the password for the database");
    desc.add_options()("help,h", "Display help");

    // Construct a variable_map
    boost::program_options::variables_map vm;

    try{
        // Parse Argument variables
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
        boost::program_options::notify(vm);
    }catch(boost::program_options::error& e) {
        std::cerr << "Arg Error: " << e.what() << std::endl << std::endl;
        std::cout << desc << std::endl;
        return 1;
    }

    const std::string connect_address("tcp://192.168.111.249:12345");
    AggServer::AggregationBroker aggserver(connect_address, database_ip, password);
    execution.Start();
    
    return 0;
}