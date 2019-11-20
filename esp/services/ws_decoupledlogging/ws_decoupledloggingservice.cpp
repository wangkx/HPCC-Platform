/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2020 HPCC Systems.

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

#include "ws_decoupledloggingservice.hpp"

#include "jlib.hpp"

#include "LoggingErrors.hpp"
#include "loggingcommon.hpp"
#include "loggingagentbase.hpp"
#include "exception_util.hpp"

typedef IEspLogAgent* (*newLoggingAgent_t_)();

IEspLogAgent* CWSDecoupledLogEx::loadLoggingAgent(const char* name, const char* dll, const char* service, IPropertyTree* cfg)
{
    StringBuffer plugin;
    plugin.append(SharedObjectPrefix).append(dll).append(SharedObjectExtension);
    HINSTANCE loggingAgentLib = LoadSharedObject(plugin.str(), true, false);
    if(!loggingAgentLib)
        throw MakeStringException(EspLoggingErrors::LoadLoggingLibraryError, "can't load library %s", plugin.str());

    newLoggingAgent_t_ xproc = (newLoggingAgent_t_)GetSharedProcedure(loggingAgentLib, "newLoggingAgent");
    if (!xproc)
        throw MakeStringException(EspLoggingErrors::LoadLoggingLibraryError, "procedure newLoggingAgent of %s can't be loaded", plugin.str());

    return (IEspLogAgent*) xproc();
}

void CWSDecoupledLogEx::init(IPropertyTree* cfg, const char* process, const char* service)
{
    if (!cfg)
        throw MakeStringException(-1, "Can't initialize CWSDecoupledLogEx, cfg is NULL");

    espProcess.set(process);

    StringBuffer xpath;
    xpath.setf("Software/EspProcess[@name=\"%s\"]/EspService[@name=\"%s\"]", process, service);
    IPropertyTree *serviceCFG = cfg->queryPropTree(xpath.str());
    tankFileDir.set(serviceCFG->queryProp("TankFileDir"));
    if (!tankFileDir.length())
        throw MakeStringException(-1, "Can't initialize CWSDecoupledLogEx, TankFileDir is NULL");

    Owned<IPTreeIterator> loggingAgentSettings = serviceCFG->getElements("LogAgent");
    ForEach(*loggingAgentSettings)
    {
        IPropertyTree& loggingAgentTree = loggingAgentSettings->query();
        const char* agentName = loggingAgentTree.queryProp("@name");
        const char* agentType = loggingAgentTree.queryProp("@type");
        const char* agentPlugin = loggingAgentTree.queryProp("@plugin");
        if (!agentName || !*agentName || !agentPlugin || !*agentPlugin)
            continue;

        IEspLogAgent* loggingAgent = loadLoggingAgent(agentName, agentPlugin, service, cfg);
        if (!loggingAgent)
        {
            OERRLOG(-1, "Failed to create logging agent for %s", agentName);
            continue;
        }
        loggingAgent->init(agentName, agentType, &loggingAgentTree, service);
        loggingAgent->initVariants(&loggingAgentTree);
        IUpdateLogThread* logThread = createUpdateLogThread(&loggingAgentTree, service, agentName, tankFileDir.get(), loggingAgent);
        if(!logThread)
            throw MakeStringException(-1, "Failed to create update log thread for %s", agentName);
        loggingAgentThreads.push_back(logThread);
    }
}

bool CWSDecoupledLogEx::onGetLogAgentSetting(IEspContext& context, IEspGetLogAgentSettingRequest& req, IEspGetLogAgentSettingResponse& resp)
{
    try
    {
        StringArray& agentNames = req.getAgentNames();
        IArrayOf<IEspLogAgentSetting> logAgentSettings;
        for (auto in : loggingAgentThreads)
        {
            IEspLogAgent* agent = in->getLogAgent();
            if (!agent)
                continue;
            const char* agentName = agent->getName();
            if (isEmptyString(agentName))
                continue;
            if (!checkName(agentName, agentNames, true))
                continue;

            CLogRequestReader* logRequestReader = in->getLogRequestReader();
            if (!logRequestReader)
                throw MakeStringException(ECLWATCH_INTERNAL_ERROR, "CLogRequestReader not found for %s.", agentName);

            CLogRequestReaderSettings* settings = logRequestReader->getSettings();
            if (!settings)
                continue;
            Owned<IEspLogAgentSetting> logAgentSetting = createLogAgentSetting();
            logAgentSetting->setAgentName(agentName);
            logAgentSetting->setAckedFileList(settings->ackedFileList);
            logAgentSetting->setAckedLogRequestFile(settings->ackedLogRequestFile);
            logAgentSetting->setWaitSeconds(settings->waitSeconds);
            logAgentSetting->setPendingLogBufferSize(settings->pendingLogBufferSize);
            logAgentSettings.append(*logAgentSetting.getClear());
        }
        resp.setLogAgentSettings(logAgentSettings);
        resp.setTankFileDir(tankFileDir);
    }
    catch(IException* e)
    {
        FORWARDEXCEPTION(context, e, ECLWATCH_INTERNAL_ERROR);
    }
    return true;
}

bool CWSDecoupledLogEx::checkName(const char* name, StringArray& names, bool defaultValue)
{
    if (!names.length())
        return defaultValue;

    const char* v = names.item(0);
    if (isEmptyString(v))
        return defaultValue;

    ForEachItemIn(i, names)
    {
        const char* aName = names.item(i);
        if (strieq(aName, name))
            return true;
    }
    return false;
}

bool CWSDecoupledLogEx::onPauseLog(IEspContext& context, IEspPauseLogRequest& req, IEspPauseLogResponse& resp)
{
    bool pause = req.getPause();
    StringArray& agentNames = req.getAgentNames();
    for (auto in : loggingAgentThreads)
    {
        IEspLogAgent* agent = in->getLogAgent();
        if (!agent)
            continue;
        const char* agentName = agent->getName();
        if (isEmptyString(agentName))
            continue;
        if (!checkName(agentName, agentNames, true))
            continue;

        CLogRequestReader* logRequestReader = in->getLogRequestReader();
        if (!logRequestReader)
            throw MakeStringException(ECLWATCH_INTERNAL_ERROR, "CLogRequestReader not found for %s.", agentName);

        logRequestReader->setPause(pause);
    }
    return true;
}
