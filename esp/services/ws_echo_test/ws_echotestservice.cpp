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

#include "jlib.hpp"
#include "ws_echotestservice.hpp"
#ifdef _WIN32
#include "windows.h"
#endif

void CWsEchoTestEx::init(IPropertyTree *cfg, const char *process, const char *service)
{
    StringBuffer xpath;
    xpath.appendf("Software/EspProcess[@name=\"%s\"]/EspService[@name=\"%s\"]", process, service);
    IPropertyTree * srvcfg = cfg->queryPropTree(xpath.str());
    if (!srvcfg)
        throw MakeStringException(-1, "Could not access EchoTest service configuration: esp process '%s' service name '%s'", process, service);

    IPropertyTree* testlibTree = srvcfg->queryPropTree("TestLib");
    if (!testlibTree)
        throw MakeStringException(-1, "ESP Service %s is not attached to any test lib.", service);

    const char* libName = testlibTree->queryProp("@name");
    if (!libName || !*libName)
        throw MakeStringException(-1, "ESP Service %s cannot find test lib name.", service);

    loadTestLib(libName);
    if (!testLib)
        throw MakeStringException(-1, "ESP Service %s failed to load test lib.", service);

    testLib->init(testlibTree, service);
}

void CWsEchoTestEx::loadTestLib(const char * libName)
{
    if (testLib)
        return;

    StringBuffer fullName;
    fullName.append(SharedObjectPrefix).append(libName).append(SharedObjectExtension);
    HINSTANCE testLibObj = LoadSharedObject(fullName.str(), true, false);
    if(!testLibObj)
        throw MakeStringException(-1, "ESP service WsEchoTest: cannot load test lib library(%s)", fullName.str());

    newLibTest_t_ xproc = NULL;
    xproc = (newLibTest_t_)GetSharedProcedure(testLibObj, "newLibTest");
    if (!xproc)
        throw MakeStringException(-1, "ESP service WsEchoTest: procedure newLibTest of %s can't be loaded", fullName.str());

    testLib.setown((ILibTest*) xproc());

    return;
}

bool CWsEchoTestEx::onEchoTest(IEspContext &context, IEspEchoTestRequest &req, IEspEchoTestResponse &resp)
{
    IEspTestMethodInfo &info = resp.updateMethodInfo();
    info.setName("EchoTest");

    if (!testLib)
        info.setStatus("WsEchoTestEx::onEchoTest: no test lib!");

    try
    {
        ESPLOG(LogNormal, "WsEchoTestEx::onEchoTest: attempting to run EchoTest.");
        testLib->echoTest(context, req, resp);
        info.setStatus("Method finished");
    }
    catch (IException *e)
    {
        StringBuffer msg;
        WARNLOG("WsEchoTestEx::onEchoTest: Failed to run EchoTest: %s", e->errorMessage(msg).str());
        e->Release();
        info.setStatus(msg.str());
    }

    return true;
}
