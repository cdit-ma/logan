#include "systemeventprotohandler.h"

#include "utils.h"

#include "databaseclient.h"
#include "experimenttracker.h"

#include <zmq/protoreceiver/protoreceiver.h>

#include <functional>


void SystemEventProtoHandler::BindCallbacks(zmq::ProtoReceiver& receiver) {
    receiver.RegisterProtoCallback<SystemEvent::StatusEvent>(std::bind(&SystemEventProtoHandler::ProcessStatusEvent, this, std::placeholders::_1));
    receiver.RegisterProtoCallback<SystemEvent::InfoEvent>(std::bind(&SystemEventProtoHandler::ProcessInfoEvent, this, std::placeholders::_1));
}