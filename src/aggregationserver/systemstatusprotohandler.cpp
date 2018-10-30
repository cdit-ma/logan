#include "systemstatusprotohandler.h"

#include "utils.h"

#include "databaseclient.h"
#include "experimenttracker.h"

#include <zmq/protoreceiver/protoreceiver.h>

#include <functional>


void SystemStatusProtoHandler::BindCallbacks(zmq::ProtoReceiver& receiver) {
    receiver.RegisterProtoCallback<SystemEvent::StatusEvent>(std::bind(&SystemStatusProtoHandler::ProcessStatusEvent, this, std::placeholders::_1));
    receiver.RegisterProtoCallback<SystemEvent::InfoEvent>(std::bind(&SystemStatusProtoHandler::ProcessInfoEvent, this, std::placeholders::_1));
}