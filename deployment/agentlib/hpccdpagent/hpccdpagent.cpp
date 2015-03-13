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

#pragma warning(disable:4786)

#include "jstring.hpp"
#include "jptree.hpp"
#include "jstream.ipp"
#include "jfile.hpp"
#include "jmisc.hpp"
#include "securesocket.hpp"
#include "hpccdpagent.hpp"

#define DEFAULTDPAHBTIMER 60

const char* DEFAULTSCRIPTPATH = "scripts";
const char* DEFAULTPACKAGEPATH = "../packages";
const char* DEFAULTRESOURCEPATH = "../resources";
//const char* TESTDPSERVER = "localhost";
const char* TESTDPSERVER = "10.176.152.178";

int CDPAHeartBeat::run()
{
    for (;;)
    {
        sleep(dpaHeartBeatTimer);

        StringBuffer reply;
        const char* message = "Hello"; //TODO
        sendHeartBeat(message, reply);
        DBGLOG("Received reply<%s>", reply.str());
    }
    return 0;
}

void CDPAHeartBeat::sendHeartBeat(const char* message, StringBuffer& reply)
{
    SocketEndpoint ep;
    ep.set(serverIP.get(), serverport);
    Owned<ISocket> socket;
    try
    {
        socket.setown(ISocket::connect(ep));
    }
    catch(IException *excpt)
    {
        StringBuffer errMsg, errMsg1;
        errMsg1.setf("Error connecting to %s:%d - %d:%s", serverIP.get(), serverport, excpt->errorCode(), excpt->errorMessage(errMsg).str());
        DBGLOG("%s", errMsg1.str());
    }
    catch(...)
    {
        StringBuffer errMsg;
        errMsg.setf("can't connect to %s:%d", serverIP.get(), serverport);
        DBGLOG("%s", errMsg.str());
    }

    if(!socket.get())
    {
        StringBuffer errMsg;
        errMsg.setf(">>Failed to connect to %s:%d", serverIP.get(), serverport);
        DBGLOG("%s", errMsg.str());
        return;
    }

    unsigned len = strlen(message);
    DBGLOG(">>sending out heart beat message: <%s>", message);
    unsigned len1 = len;
    _WINREV(len1);
    socket->write((void*)&len1, 4);
    socket->write(message, len);

    read(socket.get(), reply);

    socket->shutdown();
    socket->close();
}

__int64 CDPAHeartBeat::read(ISocket* socket, StringBuffer& buf)
{
    Owned<IBufferedSocket> bSocket = createBufferedSocket(socket);
    if(!bSocket)
    {
        DBGLOG("Can't create buffered socket\n");
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

void CHPCCDPAgent::init(int _traceLevel)
{
    traceLevel = _traceLevel;//TODO: read from config
    scriptPath.set(DEFAULTSCRIPTPATH);//TODO: read from config
    packagePath.set(DEFAULTPACKAGEPATH);//TODO: read from config
    resourcePath.set(DEFAULTRESOURCEPATH);//TODO: read from config
    readDeploymentPackages();

    const char* dpServerIP = TESTDPSERVER;//TODO: read from config
    int dpServerHBPort = 8004;//TODO: read from config
    unsigned dpaHeartBeatTimer = DEFAULTDPAHBTIMER;//TODO: read from config
    agentHeartBeat.setown(new CDPAHeartBeat());
    agentHeartBeat->init(this, dpServerIP, dpServerHBPort, dpaHeartBeatTimer);
    agentHeartBeat->start();
}

void CHPCCDPAgent::readDeploymentPackages()
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
        packages.append(*package.getClear());
    }
}

int CHPCCDPAgent::runCommand(const char *commandLine, StringBuffer& response)
{
    char   buffer[1024];
    FILE   *fp;

    if (traceLevel > 5)
        DBGLOG("command_line=<%s>", commandLine);

    if( (fp = popen( commandLine, "r" )) == NULL )
        return -1;

    while ( !feof(fp) )
        if ( fgets( buffer, 1024, fp) )
            response.append( buffer );

    if (traceLevel > 5)
        DBGLOG("response=<%s>", response.str());

    return fclose( fp );
}

CDeploymentPackage* CHPCCDPAgent::getDeploymentPackageByName(const char* name)
{
    CDeploymentPackage* pPackage = NULL;
    ForEachItemIn(i, packages)
    {
        CDeploymentPackage& package = packages.item(i);
        if (strieq(name, package.name.get()))
        {
            pPackage = &package;
            break;
        }
    }
    return pPackage;
}

//#define TEST_REMOTE_PACKAGE
#ifdef TEST_REMOTE_PACKAGE
//sample fileNameWithPath: "//10.176.152.178/home/lexis/hpcc/test/deploy_hpcc/test.sh"
//This function requests the dafilesrv running on 10.176.152.178.
void getDeploymentPackageFromServer(const char* fileNameWithPath, MemoryBuffer& buf)
{
    Owned<IFile> testFile = createIFile(fileNameWithPath);
    OwnedIFileIO rIO = testFile->openShared(IFOread,IFSHfull);
    if (!rIO)
        return;

    OwnedIFileIOStream ios = createBufferedIOStream(rIO);
    StringBuffer line;
    bool eof = false;
    while (!eof)
    {
        line.clear();
        loop
        {
            char c;
            size32_t numRead = ios->read(1, &c);
            if (!numRead)
            {
                eof = true;
                break;
            }
            line.append(c);
            if (c=='\n')
                break;
        }
        DBGLOG("TestReadRemoteFile<%s>", line.str());
        buf.append(line.length(), line.str());
    }
}
#endif
bool CHPCCDPAgent::deploy(const char* packageName, const char* options, StringBuffer& results, bool reDeploy)
{
    if (!packageName || !*packageName)
        throw MakeStringException(-1, "CHPCCDPAgent::deploy: no package is specified.");

#ifdef TEST_REMOTE_PACKAGE
    MemoryBuffer buf;
    StringBuffer fileName;
    fileName.set("//10.176.152.178/home/lexis/hpcc/test/deploy_hpcc/scppass.sh");
    getDeploymentPackageFromServer(fileName.str(), buf);
    return true;
#else
    CDeploymentPackage* pPackage = getDeploymentPackageByName(packageName);
    if (!pPackage)
        throw MakeStringException(-1, "CHPCCDPAgent::deploy: package %s not found.", packageName);

    VStringBuffer command("python %s/dp_hpcc.py -dp %s", scriptPath.get(), pPackage->build.get());
    if (runCommand(command.str(), results) != 0)
        return false;
    if (!options || !*options)
        return true;
    command.setf("sudo cp %s /etc/HPCCSystems/", pPackage->configuration.get());
    return runCommand(command.str(), results) == 0;
#endif
}

bool CHPCCDPAgent::undeploy(StringBuffer& results)
{
    VStringBuffer command("python %s/dp_hpcc.py -up", scriptPath.get());
    return runCommand(command.str(), results) == 0;
}

bool CHPCCDPAgent::start(const char* component, StringBuffer& results, bool restart)
{
    if (!component || !*component)
        return runCommand("sudo service hpcc-init start", results) == 0;

    VStringBuffer control("sudo service hpcc-init -c %s start", component);
    return runCommand(control.str(), results) == 0;
}

bool CHPCCDPAgent::stop(const char* component, StringBuffer& results)
{
    if (!component || !*component)
        return runCommand("sudo service hpcc-init stop", results) == 0;

    VStringBuffer control("sudo service hpcc-init -c %s stop", component);
    return runCommand(control.str(), results) == 0;
}

extern "C"
{
DPAGENTLIB_API IDPAgent* newDPAgent()
{
    return new CHPCCDPAgent();
}
}
