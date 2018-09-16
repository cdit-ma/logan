#include "systemstatusprotohandler.h"

#include "utils.h"

#include "databaseclient.h"
#include "experimenttracker.h"

#include <re_common/zmq/protoreceiver/protoreceiver.h>

#include <functional>


void SystemStatusProtoHandler::BindCallbacks(zmq::ProtoReceiver& receiver) {
    receiver.RegisterProtoCallback<re_common::SystemStatus>(std::bind(&SystemStatusProtoHandler::ProcessSystemStatus, this, std::placeholders::_1));
}