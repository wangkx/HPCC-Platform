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

#if !defined(__PDMSERVICELIB_HPP__)
#define __PDMSERVICELIB_HPP__

#include "pdmlibshare.hpp"

enum DeploymentComponentType
{
    dctDali = 0,
    dctEclServer = 1,
    dctEclCCServer = 2,
    dctEclScheduler = 3,
    dctEclAgent = 4,
    dctDFUServer = 5,
    dctESPServer = 6,
    dctFTSlave = 7,
    dctGenesisServer = 8,
    dctThorMaster = 9,
    dctThorSlave = 10,
    dctRoxieFarmer = 11,
    dctRoxieServer = 12,
    dctSashaServer = 13
};

class CPDMServiceLib;

class CAgentListener : public Thread
{
    CPDMServiceLib* parent;
    int port;

    __int64 read(ISocket* socket, StringBuffer& buf);
    void send(MemoryBuffer& data, ISocket* socket);
public:
    virtual int run();
    void init(CPDMServiceLib* _parent, int _port)
    {
        parent = _parent;
        port = _port;
    }
};

class CDeploymentComponent : public CInterface
{
    StringAttr name;
    DeploymentComponentType type;

public:
    CDeploymentComponent(const char* _name, DeploymentComponentType _type): name(_name), type(_type) {};
    virtual ~CDeploymentComponent(){};
    const char* getName() { return name.get(); }
    DeploymentComponentType getType() { return type; }
};

class CDeploymentHost : public CInterface
{
    StringAttr name;
    StringAttr IP;
    CIArrayOf<CDeploymentComponent> components;

public:
    CDeploymentHost(const char* _name, const char* _IP): name(_name), IP(_IP) {};
    virtual ~CDeploymentHost(){};

    const char* getName() { return name.get(); }
    const char* getIP() { return IP.get(); }
    CIArrayOf<CDeploymentComponent>& getComponents() { return components; }
    void addComponent(CDeploymentComponent& comp)
    {
        components.append(comp);
    }
    void setComponents(CIArrayOf<CDeploymentComponent>& val)
    {
        components.kill();
        ForEachItemIn(idx, val)
        {
            CDeploymentComponent& item = val.item(idx);
            item.Link();
            components.append(item);
        }
    }
    CDeploymentComponent* getComponentByName(const char* name)
    {
        ForEachItemIn(idx, components)
        {
            CDeploymentComponent& item = components.item(idx);
            if (strieq(name, item.getName()))
                return &item;
        }
        return NULL;
    }
};

class CDeploymentPackage : public CInterface
{
    StringAttr name;
    StringAttr build;
    StringAttr buildName;
    StringAttr configuration;
    StringAttr configurationName;
    CIArrayOf<CDeploymentHost> hosts;

    void addComponentToHost(const char* name, DeploymentComponentType type, const char* computer);
public:
    CDeploymentPackage(const char* _name, const char* _build, const char* _buildName,
        const char* _configuration, const char* _configurationName): name(_name), build(_build),
        buildName(_buildName), configuration(_configuration), configurationName(_configurationName) {};
    virtual ~CDeploymentPackage(){};

    const char* getName() { return name.get(); }
    const char* getBuild() { return build.get(); }
    const char* getBuildName() { return buildName.get(); }
    const char* getConfiguration() { return configuration.get(); }
    const char* getConfigurationName() { return configurationName.get(); }
    CIArrayOf<CDeploymentHost>& getHosts() { return hosts; }
    void setHosts(CIArrayOf<CDeploymentHost>& val)
    {
        hosts.kill();
        ForEachItemIn(idx, val)
        {
            CDeploymentHost& item = val.item(idx);
            item.Link();
            hosts.append(item);
        }
    }

    void readDeploymentConfiguration();
    void getHostIPs(const char* component, StringArray& hostIPs);
};

class CPDMServiceLib : public CInterface, implements IPDMServiceLib
{
    StringAttr parentName;
    unsigned dpAgentPort;
    StringAttr packagePath;
    StringAttr scriptPath;
    StringAttr resourcePath;
    CIArrayOf<CDeploymentPackage> packages;
    Owned<CAgentListener> agentListener;

    void readDeploymentPackages();
    CDeploymentPackage* getPackageByName(const char* name);
    void sendRequest(const char* host, int port, const char* request, StringBuffer& response);
    void sendRequest(const char* package, const char* component, const char* request, StringBuffer& response);
    __int64 readFromSocket(ISocket* socket, StringBuffer& buf);
public:
    IMPLEMENT_IINTERFACE;

    CPDMServiceLib(void) {};
    virtual ~CPDMServiceLib(void) {};

    virtual bool init(IPropertyTree* cfg, const char* parent);
    virtual void echoTest(IEspContext &context, IEspEchoTestRequest& req, IEspEchoTestResponse& resp);
    virtual bool createDeploymentPackage(IEspContext &context, IEspCreateDeploymentPackageRequest &req, IEspCreateDeploymentPackageResponse &resp);
    virtual bool updateDeploymentPackage(IEspContext &context, IEspUpdateDeploymentPackageRequest &req, IEspUpdateDeploymentPackageResponse &resp);
    virtual bool deleteDeploymentPackage(IEspContext &context, IEspDeleteDeploymentPackageRequest &req, IEspDeleteDeploymentPackageResponse &resp);
    virtual bool deploy(IEspContext &context, IEspDeployRequest &req, IEspDeployResponse &resp);
    virtual bool undeploy(IEspContext &context, IEspUndeployRequest &req, IEspUndeployResponse &resp);
    virtual bool start(IEspContext &context, IEspStartRequest &req, IEspStartResponse &resp);
    virtual bool stop(IEspContext &context, IEspStopRequest &req, IEspStopResponse &resp);
};

#endif // !defined(__PDMSERVICELIB_HPP__)
