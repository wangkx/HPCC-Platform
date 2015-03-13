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

#include "dpagentbase.hpp"

#ifdef _WIN32
#include "winsock.h"
#endif

extern bool echoTest;
extern int traceLevel;
extern const char* sepstr;

class CDeploymentPackage : public CInterface
{
public:
    StringAttr name;
    StringAttr build;
    StringAttr buildName;
    StringAttr configuration;
    StringAttr configurationName;

    CDeploymentPackage(const char* _name, const char* _build, const char* _buildName,
        const char* _configuration, const char* _configurationName): name(_name), build(_build),
        buildName(_buildName), configuration(_configuration), configurationName(_configurationName) {};
    virtual ~CDeploymentPackage(){};
};

class CHPCCDPAgent;

class CDPAHeartBeat : public Thread
{
    CHPCCDPAgent* parent;
    StringAttr serverIP;
    int serverport;
    unsigned dpaHeartBeatTimer;

    void sendHeartBeat(const char* message, StringBuffer& reply);
    __int64 read(ISocket* socket, StringBuffer& buf);
public:
    virtual int run();
    void init(CHPCCDPAgent* _parent, const char* _serverIP, int _port, unsigned _dpaHeartBeatTimer)
    {
        parent = _parent;
        serverIP.set(_serverIP);
        serverport = _port;
        dpaHeartBeatTimer = _dpaHeartBeatTimer;
    }
};

class CHPCCDPAgent : public CInterface, implements IDPAgent
{
    unsigned traceLevel;
    StringAttr packagePath;
    StringAttr resourcePath;
    StringAttr scriptPath;
    CIArrayOf<CDeploymentPackage> packages;
    Owned<CDPAHeartBeat> agentHeartBeat;

    void readDeploymentPackages();
    int runCommand(const char* commandLine, StringBuffer& response);
    CDeploymentPackage* getDeploymentPackageByName(const char* name);
public:
    IMPLEMENT_IINTERFACE;

    CHPCCDPAgent(void) {};
    virtual ~CHPCCDPAgent(void) {};

    virtual void init(int _traceLevel);
    virtual bool deploy(const char* package, const char* options, StringBuffer& results, bool reDeploy);
    virtual bool undeploy(StringBuffer& results);
    virtual bool start(const char* component, StringBuffer& results, bool restart);
    virtual bool stop(const char* component, StringBuffer& results);
};

#endif
