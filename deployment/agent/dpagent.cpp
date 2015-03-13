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

#include "dpagent.hpp"

bool echoTest = false;
int traceLevel = 10;
const char* sepstr = "\n---------------\n";

void DPAgent::loadPDMServiceLib(const char * libName)
{
    if (dpAgentLib)
        return;

    StringBuffer fullName;
    fullName.append(SharedObjectPrefix).append(libName).append(SharedObjectExtension);
    HINSTANCE libObj = LoadSharedObject(fullName.str(), true, false);
    if(!libObj)
        throw MakeStringException(-1, "DPAgent: cannot load dpAgent library(%s)", fullName.str());

    newPDAgentLib_t_ xproc = NULL;
    xproc = (newPDAgentLib_t_)GetSharedProcedure(libObj, "newDPAgent");
    if (!xproc)
        throw MakeStringException(-1, "DPAgent: procedure newDPAgent of %s can't be loaded", fullName.str());

    dpAgentLib.setown((IDPAgent*) xproc());
    dpAgentLib->init(traceLevel);

    return;
}

void DPAgent::sendData(MemoryBuffer& data, ISocket* socket)
{
    unsigned len = data.length();
    _WINREV(len);
    socket->write((void*)&len, 4);
    socket->write(data.toByteArray(), data.length());
}

int DPAgent::sendData(MemoryBuffer& data, const char* host, int port)
{
    if(traceLevel >= 5)
        fprintf(logFile, ">>sending out request to %s:%d\n", host, port);

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
        StringBuffer errMsg;
        fprintf(logFile, "Error connecting to %s:%d - %d:%s", host, port, excpt->errorCode(), excpt->errorMessage(errMsg).str());
        return -1;
    }
    catch(...)
    {
        fprintf(logFile, "can't connect to %s:%d", host, port);
        return -1;
    }

    if(socket.get() == NULL)
    {
        StringBuffer urlstr;
        DBGLOG(">>Can't connect to %s", ep.getUrlStr(urlstr).str());
        return -1;
    }

    if(traceLevel >= 5)
        fprintf(logFile, ">>sending out data. Request length=%d\n", data.length());

    sendData(data, socket);

    StringBuffer buf;
    read(socket.get(), buf);
    if(traceLevel >= 10)
        fprintf(logFile, "Response: %s\n", buf.str());

    socket->shutdown();
    socket->close();

    if(traceLevel >= 5)
        fprintf(logFile, "Time taken to send request and receive response: %d milli-seconds\n%s", msTick() - start1, sepstr);

    return 0;
}

__int64 DPAgent::read(ISocket* socket, StringBuffer& buf)
{
    Owned<IBufferedSocket> bSocket = createBufferedSocket(socket);
    if(!bSocket)
    {
        fprintf(logFile, "Can't create buffered socket\n");
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

void DPAgent::echoRequest(const char* request, ISocket* socket)
{
    MemoryBuffer response;
    response.append(request);
    sendData(response, socket);

    if(traceLevel >= 10)
        fprintf(logFile, "\n>>sent back response - \n%s%s%s\n", sepstr, response.toByteArray(), sepstr);
    else if(traceLevel >= 5)
        fprintf(logFile, "\n>>sent back response\n");
}

int DPAgent::start(int port)
{
    loadPDMServiceLib("hpccdpagent");
    if (!dpAgentLib)
    {
        fprintf(logFile, "\nhandleRequest: no dpagent lib loaded\n");
        return -1;
    }

    Owned<ISocket> sSocket = ISocket::create(port);
    fprintf(logFile, "Server started\n");

    for (;;)
    {
        Owned<ISocket> cSocket = sSocket->accept();

        char peerName[256];
        int port = cSocket->peer_name(peerName, 256);
        if(traceLevel >= 5)
            fprintf(logFile, "\n>>received request from %s:%d\n", peerName, port);

        StringBuffer request;
        read(cSocket.get(), request);
        if(traceLevel >= 10)
            fprintf(logFile, "%s%s%s", sepstr, request.str(), sepstr);

        if (echoTest)
            echoRequest(request.str(), cSocket);
        else
        {
            StringBuffer response;
            handleRequest(request.str(), response);
            MemoryBuffer data;
            data.append(response.str());
            sendData(data, cSocket);

            if(traceLevel >= 10)
                fprintf(logFile, "\n>>sent back response - \n%s%s%s\n", sepstr, response.str(), sepstr);
            else if(traceLevel >= 5)
                fprintf(logFile, "\n>>sent back response\n");
        }

        fflush(logFile);
        cSocket->close();
    }

    sSocket->shutdown();
    sSocket->close();

    return 0;
}

void DPAgent::handleRequest(const char* request, StringBuffer& response)
{
    fprintf(logFile, "\nhandleRequest: %s\n", request);
    if (!dpAgentLib)
    {
        fprintf(logFile, "\nhandleRequest: no dpagent lib loaded\n");
        return;
    }

    StringBuffer control;
    const char* options = NULL;
    const char* command = request;
    const char* component = strchr(request, ':');
    if (component)
    {
        control.append(request, 0, component - request);
        command = control.str();
        component++;
        const char* options = strchr(component, ':');
        if (options)
        {
            control.clear().append(component, 0, options - component);
            component = control.str();
            options++;
        }
    }

    bool unknownReq =  false;
    switch (_toupper(command[0]))
    {
    case 'D':
        if (strieq(command, "deploy"))
            dpAgentLib->deploy(component, options, response, false);
        else
            unknownReq = true;
        break;
    case 'R':
        if (strieq(command, "redeploy"))
            dpAgentLib->deploy(component, options, response, true);
        else if (strieq(command, "restart"))
            dpAgentLib->start(component, response, true);
        else
            unknownReq = true;
        break;
    case 'S':
        if (strieq(command, "Start"))
            dpAgentLib->start(component, response, false);
        else if (strieq(command, "Stop"))
            dpAgentLib->stop(component, response);
        else
            unknownReq = true;
        break;
    case 'U':
        if (strieq(command, "undeploy"))
            dpAgentLib->undeploy(response);
        else
            unknownReq = true;
        break;

    default:
        unknownReq = true;
        break;
    }
    if (unknownReq)
        fprintf(logFile, "\nhandleRequest: unknow request: %s\n", request);

    fprintf(logFile, "\nFinish handleRequest: %s\n", request);
}

int DPAgent::runCommand(const char *commandLine, StringBuffer& response)
{
    char   buffer[128];
    FILE   *fp;

    if (traceLevel > 5)
        DBGLOG("command_line=<%s>", commandLine);

    if( (fp = popen( commandLine, "r" )) == NULL )
        return -1;

    while ( !feof(fp) )
        if ( fgets( buffer, 128, fp) )
            response.append( buffer );

    if (traceLevel > 5)
        DBGLOG("response=<%s>", response.str());

    return fclose( fp );
}
