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
    Owned<IPTreeIterator> agentGroupSettings = serviceCFG->getElements("LoggingAgentGroup");
    ForEach(*agentGroupSettings)
    {
        IPropertyTree& agentGroupTree = agentGroupSettings->query();
        const char* groupName = agentGroupTree.queryProp("@name");
        if (isEmptyString(groupName))
            continue;

        const char* tankFileDir = agentGroupTree.queryProp("FailSafeLogsDir");
        if (isEmptyString(tankFileDir))
            throw MakeStringException(-1, "Can't initialize CWSDecoupledLogEx, FailSafeLogsDir is NULL fo LoggingAgentGroup %s", groupName);

        Owned<WSDecoupledLogAgentGroup> group = new WSDecoupledLogAgentGroup(groupName, tankFileDir, agentGroupTree.queryProp("FailSafeLogsMask"));
        Owned<IPTreeIterator> loggingAgentSettings = agentGroupTree.getElements("LogAgent");
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
            Owned<IUpdateLogThread> logThread = createUpdateLogThread(&loggingAgentTree, service, agentName, tankFileDir, loggingAgent);
            if(!logThread)
                throw MakeStringException(-1, "Failed to create update log thread for %s", agentName);

            CLogRequestReader* logRequestReader = logThread->getLogRequestReader();
            if (!logRequestReader)
                throw MakeStringException(-1, "CLogRequestReader not found for %s.", agentName);

            if (group->getLoggingAgentThread(agentName))
                throw MakeStringException(-1, "%s: >1 logging agents are named as %s.", groupName, agentName);
            group->addLoggingAgentThread(agentName, logThread);
        }
        logGroups.insert({groupName, group});
    }
}

bool CWSDecoupledLogEx::onGetLogAgentSetting(IEspContext& context, IEspGetLogAgentSettingRequest& req, IEspGetLogAgentSettingResponse& resp)
{
    try
    {
        IArrayOf<IEspLogAgentGroupSetting> groupSettingResp;
        IArrayOf<IConstLogAgentGroup>& groups = req.getGroups();
        if (!groups.ordinality())
        {
            for (auto ml : logGroups)
            {
                IArrayOf<IEspLogAgentSetting> agentSettingResp;
                getSettingsForAllLoggingAgentsInGroup(ml.second, agentSettingResp);

                Owned<IEspLogAgentGroupSetting> groupSetting = createLogAgentGroupSetting();
                groupSetting->setGroupName(ml.first.c_str());
                groupSetting->setGroupStatus("Found");
                const char* tankFileDir = ml.second->getTankFileDir();
                const char* tankFileMask = ml.second->getTankFileMask();
                groupSetting->setTankFileDir(tankFileDir);
                if (!isEmptyString(tankFileMask))
                    groupSetting->setTankFileMask(tankFileMask);
                groupSetting->setLogAgentSetting(agentSettingResp);
                groupSettingResp.append(*groupSetting.getClear());
            }
            resp.setSettings(groupSettingResp);
            return true;
        }

        ForEachItemIn(i, groups)
        {
            IConstLogAgentGroup& g = groups.item(i);
            const char* gName = g.getGroupName();
            if (isEmptyString(gName))
                throw MakeStringException(ECLWATCH_INVALID_INPUT, "Group name not specified.");
    
            Owned<IEspLogAgentGroupSetting> groupSetting = createLogAgentGroupSetting();
            groupSetting->setGroupName(gName);
    
            auto match = logGroups.find(gName);
            if (match == logGroups.end())
            {
                groupSetting->setGroupStatus("NotFound");
                groupSettingResp.append(*groupSetting.getClear());
                continue;
            }

            groupSetting->setGroupStatus("Found");
            const char* tankFileDir = match->second->getTankFileDir();
            const char* tankFileMask = match->second->getTankFileMask();
            groupSetting->setTankFileDir(tankFileDir);
            if (!isEmptyString(tankFileMask))
                groupSetting->setTankFileMask(tankFileMask);

            IArrayOf<IEspLogAgentSetting> agentSettingResp;
            StringArray& agentNames = g.getAgentNames();
            if (!agentNames.ordinality())
            {
                getSettingsForAllLoggingAgentsInGroup(match->second, agentSettingResp);
                groupSetting->setLogAgentSetting(agentSettingResp);
                groupSettingResp.append(*groupSetting.getClear());
                continue;
            }

            ForEachItemIn(j, agentNames)
            {
                const char* agentName = agentNames.item(j);
                if (isEmptyString(agentName))
                    throw MakeStringException(ECLWATCH_INVALID_INPUT, "%s: logging agent name not specified.", gName);

                Owned<IEspLogAgentSetting> agentSetting = createLogAgentSetting();
                agentSetting->setAgentName(agentName);
        
                IUpdateLogThread* agentThread = match->second->getLoggingAgentThread(agentName);
                if (!agentThread)
                {
                    agentSetting->setAgentStatus("AgentNotFound");
                    agentSettingResp.append(*agentSetting.getClear());
                    continue;
                }

                CLogRequestReaderSettings* settings = agentThread->getLogRequestReader()->getSettings();
                if (!settings)
                {
                    agentSetting->setAgentStatus("SettingsNotFound");
                    agentSettingResp.append(*agentSetting.getClear());
                    continue;
                }

                agentSetting->setAgentStatus("Found");
                agentSetting->setAckedFileList(settings->ackedFileList);
                agentSetting->setAckedLogRequestFile(settings->ackedLogRequestFile);
                agentSetting->setWaitSeconds(settings->waitSeconds);
                agentSetting->setPendingLogBufferSize(settings->pendingLogBufferSize);

                agentSettingResp.append(*agentSetting.getClear());
            }
            groupSetting->setLogAgentSetting(agentSettingResp);
            groupSettingResp.append(*groupSetting.getClear());
        }
        resp.setSettings(groupSettingResp);
    }
    catch(IException* e)
    {
        FORWARDEXCEPTION(context, e, ECLWATCH_INTERNAL_ERROR);
    }
    return true;
}

bool CWSDecoupledLogEx::onPauseLog(IEspContext& context, IEspPauseLogRequest& req, IEspPauseLogResponse& resp)
{
    try
    {
        IArrayOf<IEspLogAgentGroupStatus> groupStatusResp;
        bool pause = req.getPause();
        IArrayOf<IConstLogAgentGroup>& groups = req.getGroups();
        if (!groups.ordinality())
        {
            for (auto ml : logGroups)
            {
                IArrayOf<IEspLogAgentStatus> agentStatusResp;
                pauseAllLoggingAgentsInGroup(ml.second, pause, agentStatusResp);

                Owned<IEspLogAgentGroupStatus> groupStatus = createLogAgentGroupStatus();
                groupStatus->setGroupName(ml.first.c_str());
                groupStatus->setGroupStatus("Found");
                groupStatus->setLogAgentStatus(agentStatusResp);
                groupStatusResp.append(*groupStatus.getClear());
            }
            resp.setStatus(groupStatusResp);
            return true;
        }

        ForEachItemIn(i, groups)
        {
            IConstLogAgentGroup& g = groups.item(i);
            const char* gName = g.getGroupName();
            if (isEmptyString(gName))
                throw MakeStringException(ECLWATCH_INVALID_INPUT, "Group name not specified.");
    
            Owned<IEspLogAgentGroupStatus> groupStatus = createLogAgentGroupStatus();
            groupStatus->setGroupName(gName);
    
            auto match = logGroups.find(gName);
            if (match == logGroups.end())
            {
                groupStatus->setGroupStatus("NotFound");
                groupStatusResp.append(*groupStatus.getClear());
                continue;
            }

            groupStatus->setGroupStatus("Found");

            IArrayOf<IEspLogAgentStatus> agentStatusResp;
            StringArray& agentNames = g.getAgentNames();
            if (!agentNames.ordinality())
            {
                pauseAllLoggingAgentsInGroup(match->second, pause, agentStatusResp);
                groupStatus->setLogAgentStatus(agentStatusResp);
                groupStatusResp.append(*groupStatus.getClear());
                continue;
            }

            ForEachItemIn(j, agentNames)
            {
                const char* agentName = agentNames.item(j);
                if (isEmptyString(agentName))
                    throw MakeStringException(ECLWATCH_INVALID_INPUT, "%s: logging agent name not specified.", gName);

                Owned<IEspLogAgentStatus> agentStatus = createLogAgentStatus();
                agentStatus->setAgentName(agentName);
        
                IUpdateLogThread* agentThread = match->second->getLoggingAgentThread(agentName);
                if (agentThread)
                {
                    agentThread->getLogRequestReader()->setPause(pause);
                    agentStatus->setStatus(pause ? "Pausing" : "Resuming");
                }
                else
                {
                    agentStatus->setStatus("NotFound");
                }

                agentStatusResp.append(*agentStatus.getClear());
            }
            groupStatus->setLogAgentStatus(agentStatusResp);
            groupStatusResp.append(*groupStatus.getClear());
        }
        resp.setStatus(groupStatusResp);
    }
    catch(IException* e)
    {
        FORWARDEXCEPTION(context, e, ECLWATCH_INTERNAL_ERROR);
    }
    return true;
}

void CWSDecoupledLogEx::pauseAllLoggingAgentsInGroup(WSDecoupledLogAgentGroup* group, bool pause, IArrayOf<IEspLogAgentStatus>& agentStatusResp)
{
    std::map<std::string, Owned<IUpdateLogThread>>&  agentThreadMap = group->getLoggingAgentThreads();
    for (auto mt : agentThreadMap)
    {
        mt.second->getLogRequestReader()->setPause(pause);

        Owned<IEspLogAgentStatus> agentStatus = createLogAgentStatus();
        agentStatus->setAgentName(mt.first.c_str());
        agentStatus->setStatus(pause ? "Pausing" : "Resuming");
        agentStatusResp.append(*agentStatus.getClear());
    }
}

void CWSDecoupledLogEx::getSettingsForAllLoggingAgentsInGroup(WSDecoupledLogAgentGroup* group, IArrayOf<IEspLogAgentSetting>& agentSettingResp)
{
    std::map<std::string, Owned<IUpdateLogThread>>&  agentThreadMap = group->getLoggingAgentThreads();
    for (auto mt : agentThreadMap)
    {
        CLogRequestReaderSettings* settings = mt.second->getLogRequestReader()->getSettings();

        Owned<IEspLogAgentSetting> agentSetting = createLogAgentSetting();
        agentSetting->setAgentName(mt.first.c_str());
        agentSetting->setAgentStatus("Found");
        agentSetting->setAckedFileList(settings->ackedFileList);
        agentSetting->setAckedLogRequestFile(settings->ackedLogRequestFile);
        agentSetting->setWaitSeconds(settings->waitSeconds);
        agentSetting->setPendingLogBufferSize(settings->pendingLogBufferSize);
        agentSettingResp.append(*agentSetting.getClear());
    }
}

IUpdateLogThread* WSDecoupledLogAgentGroup::getLoggingAgentThread(const char* name)
{
    auto match = loggingAgentThreads.find(name);
    if (match == loggingAgentThreads.end())
        return nullptr;
    return match->second;
}
