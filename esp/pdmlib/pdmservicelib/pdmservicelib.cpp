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

#include "pdmservicelib.hpp"

#define DEFAULTDPAGENTPORT 8003
const char* DEFAULTSCRIPTPATH = "scripts";
const char* DEFAULTPACKAGEPATH = "../packages";
const char* DEFAULTRESOURCEPATH = "../resources";

void CDeploymentPackage::readDeploymentConfiguration()
{
    IPropertyTree* env = createPTreeFromXMLFile(configuration);

    Owned<IPropertyTreeIterator> computers= env->getElements("Hardware/Computer");
    ForEach(*computers)
    {
        IPropertyTree& computerTree = computers->query();
        const char* name = computerTree.queryProp("@name");
        const char* netAddress = computerTree.queryProp("@netAddress");
        if (!name || !*name || !netAddress || !*netAddress)
            continue;
        Owned<CDeploymentHost> host = new CDeploymentHost(name, netAddress);
        hosts.append(*host.getClear());
    }

    Owned<IPropertyTreeIterator> services = env->getElements("Software/DaliServerProcess");
    ForEach(*services)
    {
        IPropertyTree& serviceTree = services->query();
        const char* name = serviceTree.queryProp("@name");
        if (!name || !*name)
            continue;
        Owned<IPropertyTreeIterator> instances = serviceTree.getElements("Instance");
        ForEach(*instances)
        {
            IPropertyTree& instanceTree = instances->query();
            const char* computer = instanceTree.queryProp("@computer");
            if (!computer || !*computer)
                continue;
            addComponentToHost(name, dctDali, computer);
        }
    }
    //TODO: more
}

void CDeploymentPackage::addComponentToHost(const char* name, DeploymentComponentType type, const char* computer)
{
    ForEachItemIn(i, hosts)
    {
        CDeploymentHost& host = hosts.item(i);
        if (!strieq(computer, host.getName()))
            continue;
        Owned<CDeploymentComponent> comp = new CDeploymentComponent(name, type);
        host.addComponent(*comp.getClear());
        break;
    }
}

void CDeploymentPackage::getHostIPs(const char* component, StringArray& hostIPs)
{
    ForEachItemIn(i, hosts)
    {
        CDeploymentHost& host = hosts.item(i);
        if (!component || !*component || host.getComponentByName(component))
            hostIPs.append(host.getIP());
    }
}

int CAgentListener::run()
{
    Owned<ISocket> sSocket = ISocket::create(port);
    DBGLOG("CAgentListener started");

    for (;;)
    {
        Owned<ISocket> cSocket = sSocket->accept();
        char agentName[256];
        int cPort = cSocket->peer_name(agentName, 256);
        DBGLOG("Received request from %s:%d", agentName, cPort);

        StringBuffer request;
        read(cSocket.get(), request);
        DBGLOG("Received request<%s>", request.str());

        MemoryBuffer response;
        request.insert(0, "echo:");
        response.append(request.str());
        send(response, cSocket);
        DBGLOG("Reply <%s> sent", response.toByteArray());

        cSocket->close();
    }

    sSocket->shutdown();
    sSocket->close();
    return 0;
}

__int64 CAgentListener::read(ISocket* socket, StringBuffer& buf)
{
    Owned<IBufferedSocket> bSocket = createBufferedSocket(socket);
    if(!bSocket)
    {
        DBGLOG("Can't create buffered socket");
        return -1;
    }

    char oneLine[2048];
    bSocket->read(oneLine, 4);
    unsigned int len;
    memcpy((void*)&len, oneLine, 4);
    _WINREV(len);
    if (len & 0x80000000)
        len ^= 0x80000000;

    __int64 totalLen = 0;
    Owned<IByteOutputStream> ostream = createOutputStream(buf);
    while(len > 0)
    {
        unsigned bytesRead = bSocket->read(oneLine, (len > 2047)?2047:len);
        if(bytesRead <= 0)
            break;

        totalLen += bytesRead;
        len -= bytesRead;
        ostream->writeBytes(oneLine, bytesRead);
    }
    return totalLen;
}

void CAgentListener::send(MemoryBuffer& data, ISocket* socket)
{
    unsigned len = data.length();
    _WINREV(len);
    socket->write((void*)&len, 4);
    socket->write(data.toByteArray(), data.length());
}

//Called when this lib is created.
bool CPDMServiceLib::init(IPropertyTree* cfg, const char* parent)
{
    parentName.set(parent);
    dpAgentPort = DEFAULTDPAGENTPORT; //TODO read from config
    scriptPath.set(DEFAULTSCRIPTPATH);//TODO: read from config
    packagePath.set(DEFAULTPACKAGEPATH);//TODO: read from config
    resourcePath.set(DEFAULTRESOURCEPATH);//TODO: read from config
    readDeploymentPackages();

    int agentListenerPort = 8004;//TODO: read from config
    agentListener.setown(new CAgentListener());
    agentListener->init(this, agentListenerPort);
    agentListener->start();
    return true;
}

void CPDMServiceLib::readDeploymentPackages()
{
    VStringBuffer packageXMLFile("%s/packages.xml", packagePath.get());
    Owned<IPropertyTree> packagesXML=createPTreeFromXMLFile(packageXMLFile.str());
    if(!packagesXML)
        return;
    Owned<IPropertyTreeIterator> ic = packagesXML->getElements("package");
    ForEach(*ic)
    {
        IPropertyTree& packageTree = ic->query();
        const char* packageName = packageTree.queryProp("@name");
        if (!packageName || !*packageName)
            continue;
        IPropertyTree* buildTree = packageTree.queryBranch("build");
        IPropertyTree* configurationTree = packageTree.queryBranch("configuration");
        if (!buildTree || !configurationTree)
            continue;
        const char* buildName = buildTree->queryProp("@name");
        const char* configurationName = configurationTree->queryProp("@name");
        if (!buildName || !*buildName || !configurationName || !*configurationName)
            continue;

        VStringBuffer buildFile("%s/builds/%s", resourcePath.get(), buildName);
        VStringBuffer configurationFile("%s/configurations/%s", resourcePath.get(), configurationName);
        Owned<CDeploymentPackage> package = new CDeploymentPackage(packageName, buildFile.str(),
            buildName, configurationFile.str(), configurationName);
        package->readDeploymentConfiguration();
        packages.append(*package.getClear());
    }
}

void CPDMServiceLib::echoTest(IEspContext &context, IEspEchoTestRequest& req, IEspEchoTestResponse& resp)
{
    const char* inputString = req.getInputString();
    if (!inputString || !*inputString)
        throw MakeStringException(-1, "InputString not specified");

    resp.setEchoString(inputString);
}


bool CPDMServiceLib::createDeploymentPackage(IEspContext &context, IEspCreateDeploymentPackageRequest &req, IEspCreateDeploymentPackageResponse &resp)
{
    return true;
}

bool CPDMServiceLib::updateDeploymentPackage(IEspContext &context, IEspUpdateDeploymentPackageRequest &req, IEspUpdateDeploymentPackageResponse &resp)
{
    return true;
}

bool CPDMServiceLib::deleteDeploymentPackage(IEspContext &context, IEspDeleteDeploymentPackageRequest &req, IEspDeleteDeploymentPackageResponse &resp)
{
    return true;
}

bool CPDMServiceLib::deploy(IEspContext &context, IEspDeployRequest &req, IEspDeployResponse &resp)
{
    const char* package = req.getPackage();
    if (!package || !*package)
        throw MakeStringException(-1, "CPDMServiceLib::start: package not specified");

    StringBuffer request, response;
    bool redeploy = req.getRedeploy();
    if (redeploy)
        request.append("redeploy");
    else
        request.append("deploy");
    request.appendf(":%s", package);
    bool newConfiguration  = req.getNewConfiguration();
    if (newConfiguration)
        request.appendf(":newConfig");

    sendRequest(package, "", request.str(), response);
    if (response.length())
        resp.setResult(response.str());
    return true;
}

bool CPDMServiceLib::undeploy(IEspContext &context, IEspUndeployRequest &req, IEspUndeployResponse &resp)
{
    const char* package = req.getPackage();
    if (!package || !*package)
        throw MakeStringException(-1, "CPDMServiceLib::start: package not specified");

    StringBuffer response;
    sendRequest(package, "", "undeploy", response);
    if (response.length())
        resp.setResult(response.str());
    return true;
}

bool CPDMServiceLib::start(IEspContext &context, IEspStartRequest &req, IEspStartResponse &resp)
{
    const char* package = req.getPackage();
    if (!package || !*package)
        throw MakeStringException(-1, "CPDMServiceLib::start: package not specified");

    StringBuffer request, response;
    const char* component = req.getComponent();
    bool restart = req.getRestart();
    if (restart)
        request.append("restart");
    else
        request.append("start");
    if (component && *component)
        request.appendf(":%s", component);

    sendRequest(package, component, request.str(), response);
    if (response.length())
        resp.setResult(response.str());

    return true;
}

bool CPDMServiceLib::stop(IEspContext &context, IEspStopRequest &req, IEspStopResponse &resp)
{
    const char* package = req.getPackage();
    if (!package || !*package)
        throw MakeStringException(-1, "CPDMServiceLib::start: package not specified");

    StringBuffer request, response;
    const char* component = req.getComponent();
    if (component && *component)
        request.appendf("stop:%s", component);
    else
        request.appendf("stop");

    sendRequest(package, component, request.str(), response);
    if (response.length())
        resp.setResult(response.str());

    return true;
}

void CPDMServiceLib::sendRequest(const char* package, const char* component, const char* request, StringBuffer& response)
{
    StringArray hostIPs;
    CDeploymentPackage* pPackage = getPackageByName(package);
    if (!pPackage)
        throw MakeStringException(-1, "CPDMServiceLib::sendRequest: package %s not found", package);

    pPackage->getHostIPs(component, hostIPs);
    ForEachItemIn(i, hostIPs)
    {
        const char* host = hostIPs.item(i);
        sendRequest(host, dpAgentPort, request, response);
    }
}

void CPDMServiceLib::sendRequest(const char* host, int port, const char* request, StringBuffer& response)
{
    DBGLOG(">>sending out request to %s:%d", host, port);

    unsigned start1 = msTick();

    SocketEndpoint ep;
    ep.set(host, port);
    Owned<ISocket> socket;
    try
    {
        socket.setown(ISocket::connect(ep));
    }
    catch(IException *excpt)
    {
        StringBuffer errMsg, errMsg1;
        errMsg1.setf("Error connecting to %s:%d - %d:%s", host, port, excpt->errorCode(), excpt->errorMessage(errMsg).str());
        DBGLOG("%s", errMsg1.str());
        throw MakeStringException(-1, "CPDMServiceLib::sendRequest: %s", errMsg1.str());
    }
    catch(...)
    {
        StringBuffer errMsg;
        errMsg.setf("can't connect to %s:%d", host, port);
        DBGLOG("%s", errMsg.str());
        throw MakeStringException(-1, "CPDMServiceLib::sendRequest: %s", errMsg.str());
    }

    if(socket.get() == NULL)
    {
        StringBuffer errMsg;
        errMsg.setf(">>Failed to connect to %s:%d", host, port);
        DBGLOG("%s", errMsg.str());
        throw MakeStringException(-1, "CPDMServiceLib::sendRequest: %s", errMsg.str());
    }

    unsigned len = strlen(request);
    DBGLOG(">>sending out request: <%s>", request);
    unsigned len1 = len;
    _WINREV(len1);
    socket->write((void*)&len1, 4);
    socket->write(request, len);

    readFromSocket(socket.get(), response);

    socket->shutdown();
    socket->close();

    DBGLOG("Time taken to send request and receive response: %d milli-seconds", msTick() - start1);
}

__int64 CPDMServiceLib::readFromSocket(ISocket* socket, StringBuffer& buf)
{
    Owned<IBufferedSocket> bSocket = createBufferedSocket(socket);
    if(!bSocket)
    {
        DBGLOG("Can't create buffered socket");
        return -1;
    }

    char oneLine[2048];
    bSocket->read(oneLine, 4);
    unsigned int len;
    memcpy((void*)&len, oneLine, 4);
    _WINREV(len);
    if (len & 0x80000000)
        len ^= 0x80000000;

    __int64 totalLen = 0;
    Owned<IByteOutputStream> ostream = createOutputStream(buf);
    while(len > 0)
    {
        unsigned bytesRead = bSocket->read(oneLine, (len > 2047)?2047:len);
        if(bytesRead <= 0)
            break;

        totalLen += bytesRead;
        len -= bytesRead;
        ostream->writeBytes(oneLine, bytesRead);
    }
    return totalLen;
}

CDeploymentPackage* CPDMServiceLib::getPackageByName(const char* name)
{
    ForEachItemIn(i, packages)
    {
        CDeploymentPackage& package = packages.item(i);
        if (strieq(name, package.getName()))
            return &package;
    }
    return NULL;
}

extern "C"
{
PDMSERVICELIB_API IPDMServiceLib* newPDMServiceLib()
{
    return new CPDMServiceLib();
}
}
