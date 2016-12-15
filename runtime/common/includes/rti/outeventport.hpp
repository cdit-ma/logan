#ifndef RTI_OUTEVENTPORT_H
#define RTI_OUTEVENTPORT_H

#include "../globalinterfaces.h"

#include <string>

#include "helper.hpp"

namespace rti{
     template <class T, class S> class OutEventPort: public ::OutEventPort<T>{
        public:
            OutEventPort(::OutEventPort<T>* port, int domain_id, std::string publisher_name, std::string topic_name);
            void tx_(T* message);
        private:
            dds::pub::DataWriter<S> writer_ = dds::pub::DataWriter<S>(dds::core::null);
            ::OutEventPort<T>* port_;
    }; 
};

template <class T, class S>
void rti::OutEventPort<T, S>::tx_(T* message){
    if(writer_ != dds::core::null){
        auto m = translate(message);
        //De-reference the message and send
        writer_.write(*m);
        delete m;
    }else{
        //No writer
    }
};

template <class T, class S>
rti::OutEventPort<T, S>::OutEventPort(::OutEventPort<T>* port, int domain_id, std::string publisher_name, std::string topic_name){
    this->port_ = port;
    
    //Construct a DDS Participant, Publisher, Topic and Writer
    auto helper = DdsHelper::get_dds_helper();   
    auto participant = helper->get_participant(domain_id);
    auto publisher = helper->get_publisher(participant, publisher_name);
    auto topic = helper->get_topic<S>(participant, topic_name);
    writer_ = helper->get_data_writer<S>(publisher, topic);
};

#endif //RTI_OUTEVENTPORT_H