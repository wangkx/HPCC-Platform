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
#include "ws_pdmservice.hpp"
#ifdef _WIN32
#include "windows.h"
#endif

void CWsPDMServiceEx::init(IPropertyTree *cfg, const char *process, const char *service)
{
    StringBuffer xpath;
    xpath.appendf("Software/EspProcess[@name=\"%s\"]/EspService[@name=\"%s\"]", process, service);
    IPropertyTree * srvcfg = cfg->queryPropTree(xpath.str());
    if (!srvcfg)
        throw MakeStringException(-1, "Could not access EchoTest service configuration: esp process '%s' service name '%s'", process, service);

    IPropertyTree* pdmServiceLibTree = srvcfg->queryPropTree("PDMServiceLib");
    if (!pdmServiceLibTree)
        throw MakeStringException(-1, "ESP Service %s is not attached to any PDMService lib.", service);

    const char* libName = pdmServiceLibTree->queryProp("@name");
    if (!libName || !*libName)
        throw MakeStringException(-1, "ESP Service %s cannot find PDMService name.", service);

    loadPDMServiceLib(libName);
    if (!pdmServiceLib)
        throw MakeStringException(-1, "ESP Service %s failed to load PDMService lib.", service);

    pdmServiceLib->init(pdmServiceLibTree, service);
}

void CWsPDMServiceEx::loadPDMServiceLib(const char * libName)
{
    if (pdmServiceLib)
        return;

    StringBuffer fullName;
    fullName.append(SharedObjectPrefix).append(libName).append(SharedObjectExtension);
    HINSTANCE libObj = LoadSharedObject(fullName.str(), true, false);
    if(!libObj)
        throw MakeStringException(-1, "ESP service WsEchoTest: cannot load PDMService library(%s)", fullName.str());

    newPDMService_t_ xproc = NULL;
    xproc = (newPDMService_t_)GetSharedProcedure(libObj, "newPDMServiceLib");
    if (!xproc)
        throw MakeStringException(-1, "ESP service WsEchoTest: procedure newPDMServiceLib of %s can't be loaded", fullName.str());

    pdmServiceLib.setown((IPDMServiceLib*) xproc());

    return;
}

bool CWsPDMServiceEx::onEchoTest(IEspContext &context, IEspEchoTestRequest &req, IEspEchoTestResponse &resp)
{
    StringBuffer peerStr;
    const char *userid=context.queryUserId();
    DBGLOG("EchoTest received from %s@%s", (userid) ? userid : "unknown", context.getPeer(peerStr).str());

    IEspWsMethodInfo &info = resp.updateMethodInfo();
    info.setName("EchoTest");

    if (!pdmServiceLib)
        throw MakeStringException(-1, "CWsPDMServiceEx::onEchoTest: no PDMService lib!");

    try
    {
        ESPLOG(LogNormal, "CWsPDMServiceEx::onEchoTest: attempting to run EchoTest.");
        pdmServiceLib->echoTest(context, req, resp);
        info.setStatus("Method finished");
    }
    catch (IException *e)
    {
        StringBuffer msg;
        WARNLOG("CWsPDMServiceEx::onEchoTest: Failed to run EchoTest: %s", e->errorMessage(msg).str());
        e->Release();
        info.setStatus(msg.str());
    }

    return true;
}

bool CWsPDMServiceEx::onCreateDeploymentPackage(IEspContext &context, IEspCreateDeploymentPackageRequest &req, IEspCreateDeploymentPackageResponse &resp)
{
    if (!pdmServiceLib)
        throw MakeStringException(-1, "CWsPDMServiceEx::onCreateDeploymentPackage: no PDMService lib!");

    bool bRet = true;
    try
    {
        ESPLOG(LogNormal, "CWsPDMServiceEx::onCreateDeploymentPackage: attempting to run onCreateDeploymentPackage.");
        bRet = pdmServiceLib->createDeploymentPackage(context, req, resp);
    }
    catch (IException *e)
    {
        StringBuffer msg;
        WARNLOG("CWsPDMServiceEx::onCreateDeploymentPackage: Failed to run onCreateDeploymentPackage: %s", e->errorMessage(msg).str());
        e->Release();
    }
    return bRet;
}

bool CWsPDMServiceEx::onUpdateDeploymentPackage(IEspContext &context, IEspUpdateDeploymentPackageRequest &req, IEspUpdateDeploymentPackageResponse &resp)
{
    if (!pdmServiceLib)
        throw MakeStringException(-1, "CWsPDMServiceEx::onUpdateDeploymentPackage: no PDMService lib!");

    bool bRet = true;
    try
    {
        ESPLOG(LogNormal, "CWsPDMServiceEx::onUpdateDeploymentPackage: attempting to run onUpdateDeploymentPackage.");
        bRet = pdmServiceLib->updateDeploymentPackage(context, req, resp);
    }
    catch (IException *e)
    {
        StringBuffer msg;
        WARNLOG("CWsPDMServiceEx::onUpdateDeploymentPackage: Failed to run onUpdateDeploymentPackage: %s", e->errorMessage(msg).str());
        e->Release();
    }
    return bRet;
}

bool CWsPDMServiceEx::onDeleteDeploymentPackage(IEspContext &context, IEspDeleteDeploymentPackageRequest &req, IEspDeleteDeploymentPackageResponse &resp)
{
    if (!pdmServiceLib)
        throw MakeStringException(-1, "CWsPDMServiceEx::onDeleteDeploymentPackage: no PDMService lib!");

    bool bRet = true;
    try
    {
        ESPLOG(LogNormal, "CWsPDMServiceEx::onDeleteDeploymentPackage: attempting to run onDeleteDeploymentPackage.");
        bRet = pdmServiceLib->deleteDeploymentPackage(context, req, resp);
    }
    catch (IException *e)
    {
        StringBuffer msg;
        WARNLOG("CWsPDMServiceEx::onDeleteDeploymentPackage: Failed to run onDeleteDeploymentPackage: %s", e->errorMessage(msg).str());
        e->Release();
    }
    return bRet;
}

bool CWsPDMServiceEx::onDeploy(IEspContext &context, IEspDeployRequest &req, IEspDeployResponse &resp)
{
    if (!pdmServiceLib)
        throw MakeStringException(-1, "CWsPDMServiceEx::onDeploy: no PDMService lib!");

    bool bRet = true;
    try
    {
        ESPLOG(LogNormal, "CWsPDMServiceEx::onDeploy: attempting to run onDeploy.");
        bRet = pdmServiceLib->deploy(context, req, resp);
    }
    catch (IException *e)
    {
        StringBuffer msg;
        WARNLOG("CWsPDMServiceEx::onDeploy: Failed to run onDeploy: %s", e->errorMessage(msg).str());
        e->Release();
    }
    return bRet;
}

bool CWsPDMServiceEx::onUndeploy(IEspContext &context, IEspUndeployRequest &req, IEspUndeployResponse &resp)
{
    if (!pdmServiceLib)
        throw MakeStringException(-1, "CWsPDMServiceEx::onUndeploy: no PDMService lib!");

    bool bRet = true;
    try
    {
        ESPLOG(LogNormal, "CWsPDMServiceEx::onUndeploy: attempting to run onUndeploy.");
        bRet = pdmServiceLib->undeploy(context, req, resp);
    }
    catch (IException *e)
    {
        StringBuffer msg;
        WARNLOG("CWsPDMServiceEx::onUndeploy: Failed to run onUndeploy: %s", e->errorMessage(msg).str());
        e->Release();
    }
    return bRet;
}

bool CWsPDMServiceEx::onStart(IEspContext &context, IEspStartRequest &req, IEspStartResponse &resp)
{
    if (!pdmServiceLib)
        throw MakeStringException(-1, "CWsPDMServiceEx::onStart: no PDMService lib!");

    bool bRet = true;
    try
    {
        ESPLOG(LogNormal, "CWsPDMServiceEx::onStart: attempting to run onStart.");
        bRet = pdmServiceLib->start(context, req, resp);
    }
    catch (IException *e)
    {
        StringBuffer msg;
        WARNLOG("CWsPDMServiceEx::onStart: Failed to run onStart: %s", e->errorMessage(msg).str());
        e->Release();
    }
    return bRet;
}

bool CWsPDMServiceEx::onStop(IEspContext &context, IEspStopRequest &req, IEspStopResponse &resp)
{
    if (!pdmServiceLib)
        throw MakeStringException(-1, "CWsPDMServiceEx::onStop: no PDMService lib!");

    bool bRet = true;
    try
    {
        ESPLOG(LogNormal, "CWsPDMServiceEx::onStop: attempting to run onStop.");
        bRet = pdmServiceLib->stop(context, req, resp);
    }
    catch (IException *e)
    {
        StringBuffer msg;
        WARNLOG("CWsPDMServiceEx::onStop: Failed to run onStop: %s", e->errorMessage(msg).str());
        e->Release();
    }
    return bRet;
}
