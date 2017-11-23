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
 
#ifndef LOGCONTROLLER_H
#define LOGCONTROLLER_H

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <set>
#include <unordered_map>
#include <vector>
#include <google/protobuf/message_lite.h>

//Forward declare
class SystemInfo;
namespace re_common{
    class SystemInfo;
    class SystemStatus;
};

namespace zmq{
    class Monitor;
    class ProtoWriter;
}

class LogController{
    public:
        LogController();
        ~LogController();

        std::string GetSystemInfoJson();
        bool Start(std::string endpoint, double frequency, std::vector<std::string> processes, bool live_mode = false);
        bool Terminate();
    private:

        void LogThread();
        void WriteThread();
        void TerminateLogger();
        void TerminateWriter();

        re_common::SystemStatus* GetSystemStatus();
        re_common::SystemInfo* GetOneTimeInfo();
        void QueueOneTimeInfo();
        
        void GotNewConnection(int event_type, std::string address);

        SystemInfo* system_info_ = 0;
        bool one_time_flag_ = false;

        zmq::Monitor* monitor_ = 0;
        zmq::ProtoWriter* writer_ = 0;

        std::thread* logging_thread_ = 0;
        std::thread* writer_thread_ = 0;

        std::condition_variable queue_lock_condition_;
        std::mutex queue_mutex_;
        std::queue<google::protobuf::MessageLite*> message_queue_;

        bool running = false;
        int sleep_time_ = 0;

        std::string host_name;

        int message_id_ = 0;

        std::vector<std::string> processes_;
        //set of seen pids
        //don't send onetime info for any contained pids
        std::unordered_map<int, double> pid_updated_times_;

        std::set<std::string> seen_hostnames_;
        std::set<std::string> seen_fs_;
        std::set<std::string> seen_if_;


        bool logger_terminate_ = false;
        bool writer_terminate_ = false;
};

#endif //LOGCONTROLLER_H
