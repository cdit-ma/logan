/* logan
 * Copyright (C) 2016-2017 The University of Adelaide
 *
 * This file is part of "logan"
 *
 * "logan" is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * "logan" is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */
 
#ifndef LOGAN_SERVER_LOGPROTOHANDLER_H
#define LOGAN_SERVER_LOGPROTOHANDLER_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include "sqlite3.h"

#include <google/protobuf/message_lite.h>
class Table;
class ZMQReceiver;
class SQLiteDatabase;
namespace google { namespace protobuf { class MessageLite; } }
namespace zmq{
    class ProtoReceiver;
}
class LogProtoHandler{
    public:
        LogProtoHandler(std::string database_file, std::vector<std::string> addresses);
        ~LogProtoHandler();
    private:
        void ProcessSystemStatus(google::protobuf::MessageLite* status);
        void ProcessOneTimeSystemInfo(google::protobuf::MessageLite* info);

        void ProcessLifecycleEvent(google::protobuf::MessageLite* message);
        void ProcessMessageEvent(google::protobuf::MessageLite* message);
        void ProcessUserEvent(google::protobuf::MessageLite* message);
        void ProcessWorkloadEvent(google::protobuf::MessageLite* message);
        void ProcessClientEvent(std::string client_endpoint);

        zmq::ProtoReceiver* receiver_;
        SQLiteDatabase* database_;

        std::map<std::string, Table*> table_map_;

        std::set<std::string> registered_nodes_;

        void CreateSystemStatusTable();
        void CreateSystemInfoTable();
        void CreateCpuTable();
        void CreateFileSystemTable();
        void CreateFileSystemInfoTable();
        void CreateInterfaceTable();
        void CreateInterfaceInfoTable();
        void CreateProcessTable();
        void CreateProcessInfoTable();
        void CreatePortEventTable();
        void CreateComponentEventTable();
        void CreateMessageEventTable();
        void CreateUserEventTable();
        void CreateWorkloadEventTable();
        void CreateClientTable();
};
#endif //LOGAN_SERVER_LOGPROTOHANDLER_H