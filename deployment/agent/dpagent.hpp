/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2015 HPCC Systems.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
############################################################################## */

#ifndef _DPAGENT_HPP__
#define _DPAGENT_HPP__

#include "jstring.hpp"
#include "jptree.hpp"
#include "jstream.ipp"
#include "jmisc.hpp"
#include "securesocket.hpp"
//#include "xslprocessor.hpp"
//#include "xmlvalidator.hpp"

#ifdef _WIN32
#include "winsock.h"
#endif

#include "dpagentbase.hpp"

extern bool echoTest;
extern int traceLevel;
extern const char* sepstr;
/*
class CDPAgentHeartBeat : public Thread
{
    StringAttr server;
    int port;
    bool terminating;

public:
    CDPAgentHeartBeat(const char* _server, int _port) : Thread("CDPAgentHeartBeat"),
        server(_server), port(_port), terminating(false) {};
    //virtual void start();
    virtual int run();
};*/

class DPAgent
{
    Owned<IDPAgent> dpAgentLib;

    FILE*   logFile;
    void echoRequest(const char* request, ISocket* socket);
    __int64 read(ISocket* socket, StringBuffer& buf);
    void sendData(MemoryBuffer& data, ISocket* socket);
    int runCommand(const char *commandLine, StringBuffer& response);
    void loadPDMServiceLib(const char * libName);

public:
    DPAgent(FILE* _logFile)
    {
        if (_logFile)
            logFile = _logFile;
        else
            logFile = stdout;
    }
    int start(int _port);
    int sendData(MemoryBuffer& data, const char* host, int _port);
    void handleRequest(const char* request, StringBuffer& response);
};

#endif
