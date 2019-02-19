/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2014 HPCC Systems.

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

#include "LoggingErrors.hpp"
#include "loggingcommon.hpp"
#include "loggingmanager.hpp"

CLoggingManager::~CLoggingManager(void)
{
    for (unsigned int x = 0; x < loggingAgentThreads.size(); x++)
    {
        loggingAgentThreads[x]->stop();
        loggingAgentThreads[x]->Release();
    }

    loggingAgentThreads.clear();
}

//Called when Logging manager is created. Create logging agents based on settings
bool CLoggingManager::init(IPropertyTree* cfg, const char* service)
{
    if (!cfg)
    {
        ERRLOG(EspLoggingErrors::ConfigurationFileEntryError, "Logging Manager setting not found for %s", service);
        return false;
    }

    oneTankFile = cfg->queryPropBool("FailSafe");
    if (oneTankFile)
        logFailSafe.setown(createFailSafeLogger(cfg, service, cfg->queryProp("@name")));

    Owned<IPTreeIterator> loggingAgentSettings = cfg->getElements("LogAgent");
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
            ERRLOG(-1, "Failed to create logging agent for %s", agentName);
            continue;
        }
        loggingAgent->init(agentName, agentType, &loggingAgentTree, service);
        IUpdateLogThread* logThread = createUpdateLogThread(&loggingAgentTree, service, agentName, loggingAgent);
        if(!logThread)
            throw MakeStringException(-1, "Failed to create update log thread for %s", agentName);
        loggingAgentThreads.push_back(logThread);
    }

    initialized = true;
    return !loggingAgentThreads.empty();
}

typedef IEspLogAgent* (*newLoggingAgent_t_)();

IEspLogAgent* CLoggingManager::loadLoggingAgent(const char* name, const char* dll, const char* service, IPropertyTree* cfg)
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

IEspLogEntry* CLoggingManager::createLogEntry()
{
    return new CEspLogEntry();
}

bool CLoggingManager::updateLog(IEspLogEntry* entry, StringBuffer& status)
{
    if (entry->getLogContent())
        return updateLog(entry->getEspContext(), entry->getOption(), entry->getLogContent(), status);

    if (entry->getLogInfoTree())
        return updateLog(entry->getEspContext(), entry->getOption(), entry->getLogInfoTree(), entry->getExtraLog(), status);

    return updateLog(entry->getEspContext(), entry->getOption(), entry->getUserContextTree(), entry->getUserRequestTree(),
        entry->getBackEndReq(), entry->getBackEndResp(), entry->getUserResp(), entry->getLogDatasets(), status);
}

bool CLoggingManager::updateLog(IEspContext* espContext, const char* option, const char* logContent, StringBuffer& status)
{
    if (!initialized)
        throw MakeStringException(-1,"LoggingManager not initialized");

    bool bRet = false;
    try
    {
        Owned<IEspUpdateLogRequestWrap> req =  new CUpdateLogRequestWrap(nullptr, option, logContent);
        Owned<IEspUpdateLogResponse> resp =  createUpdateLogResponse();
        bRet = updateLog(espContext, *req, *resp, status);
    }
    catch (IException* e)
    {
        status.set("Failed to update log: ");
        e->errorMessage(status);
        ERRLOG("%s", status.str());
        e->Release();
    }

    return bRet;
}

bool CLoggingManager::updateLog(IEspContext* espContext, const char* option, IPropertyTree* logInfo, IInterface* extraLog, StringBuffer& status)
{
    if (!initialized)
        throw MakeStringException(-1,"LoggingManager not initialized");

    bool bRet = false;
    try
    {
        Owned<IEspUpdateLogRequestWrap> req =  new CUpdateLogRequestWrap(nullptr, option, LINK(logInfo), LINK(extraLog));
        Owned<IEspUpdateLogResponse> resp =  createUpdateLogResponse();
        bRet = updateLog(espContext, *req, *resp, status);
    }
    catch (IException* e)
    {
        status.set("Failed to update log: ");
        e->errorMessage(status);
        ERRLOG("%s", status.str());
        e->Release();
    }

    return bRet;
}

bool CLoggingManager::updateLog(IEspContext* espContext, const char* option, IPropertyTree* userContext, IPropertyTree* userRequest,
    const char* backEndReq, const char* backEndResp, const char* userResp, const char* logDatasets, StringBuffer& status)
{
    if (!initialized)
        throw MakeStringException(-1,"LoggingManager not initialized");

    bool bRet = false;
    try
    {
        Owned<IPropertyTree> espContextTree;
        if (espContext)
        {
            espContextTree.setown(createPTree("ESPContext"));

            short port;
            StringBuffer sourceIP, peerStr;
            const char* esdlBindingID = espContext->queryESDLBindingID();
            espContext->getServAddress(sourceIP, port);
            espContextTree->addProp("SourceIP", sourceIP.str());
            espContext->getPeer(peerStr);
            espContextTree->addProp("Peer", peerStr.str());
            if (!isEmptyString(esdlBindingID))
                espContextTree->addProp("ESDLBindingID", esdlBindingID);
            //More information in espContext may be added to the espContextTree later.

            const char* userId = espContext->queryUserId();
            if (userId && *userId)
                espContextTree->addProp("UserName", userId);

            espContextTree->addProp("ResponseTime", VStringBuffer("%.4f", (msTick()-espContext->queryCreationTime())/1000.0));
        }
        Owned<IEspUpdateLogRequestWrap> req =  new CUpdateLogRequestWrap(nullptr, option, espContextTree.getClear(), LINK(userContext), LINK(userRequest),
            backEndReq, backEndResp, userResp, logDatasets);
        Owned<IEspUpdateLogResponse> resp =  createUpdateLogResponse();
        bRet = updateLog(espContext, *req, *resp, status);
    }
    catch (IException* e)
    {
        status.set("Failed to update log: ");
        e->errorMessage(status);
        ERRLOG("%s", status.str());
        e->Release();
    }
    return bRet;
}

bool CLoggingManager::updateLog(IEspContext* espContext, IEspUpdateLogRequestWrap& req, IEspUpdateLogResponse& resp, StringBuffer& status)
{
    bool bRet = updateLog(espContext, req, resp);
    if (bRet)
        status.set("Log request has been sent.");
    else
    {
        const char* statusMsg = resp.getStatusMessage();
        if (statusMsg && *statusMsg)
            status.setf("Failed to update log: %s", statusMsg);
        else
            status.set("Failed to update log");
    }
    return bRet;
}

bool CLoggingManager::updateLog(IEspContext* espContext, IEspUpdateLogRequestWrap& req, IEspUpdateLogResponse& resp)
{
    if (!initialized)
        throw MakeStringException(-1,"LoggingManager not initialized");

    bool bRet = false;
    try
    {
        if (espContext)
            espContext->addTraceSummaryTimeStamp(LogMin, "LMgr:startQLog");

        if (oneTankFile)
        {
            Owned<CLogRequestInFile> reqInFile = new CLogRequestInFile();
            if (!saveToTankFile(req, reqInFile))
                ERRLOG("LoggingManager: failed in saveToTankFile().");
            //The contentInFile will be sent to logging agents for retrieving the log content.
        }

        for (unsigned int x = 0; x < loggingAgentThreads.size(); x++)
        {
            IUpdateLogThread* loggingThread = loggingAgentThreads[x];
            if (loggingThread->hasService(LGSTUpdateLOG))
            {
                loggingThread->queueLog(&req);
                bRet = true;
            }
        }
        if (espContext)
            espContext->addTraceSummaryTimeStamp(LogMin, "LMgr:endQLog");
    }
    catch (IException* e)
    {
        StringBuffer errorStr;
        e->errorMessage(errorStr);
        ERRLOG("Failed to update log: %s",errorStr.str());
        resp.setStatusCode(-1);
        resp.setStatusMessage(errorStr.str());
        e->Release();
    }
    return bRet;
}

bool CLoggingManager::saveToTankFile(IEspUpdateLogRequestWrap& logRequest, CLogRequestInFile* reqInFile)
{
    if (!logFailSafe.get())
    {
        ERRLOG("CLoggingManager::saveToTankFile: logFailSafe not configured.");
        return false;
    }

    unsigned startTime = (getEspLogLevel()>=LogNormal) ? msTick() : 0;

    StringBuffer GUID;
    logFailSafe->GenerateGUID(GUID, NULL);
    reqInFile->setGUID(GUID);
    reqInFile->setOption(logRequest.getOption());

    StringBuffer reqBuf;
    filterLogContent(&logRequest);
    if (!serializeLogRequestContent(&logRequest, GUID, reqBuf))
    {
        ERRLOG("CLoggingManager::saveToTankFile: failed in serializeLogRequestContent().");
        return false;
    }

    logFailSafe->AddACK(GUID);//Ack this logging request since the task will be done as soon as the next line is called.
    logFailSafe->Add(GUID, reqBuf, reqInFile);

    ESPLOG(LogNormal, "LThread:saveToTankFile: %dms\n", msTick() - startTime);
    return true;
}

void CLoggingManager::filterLogContent(IEspUpdateLogRequestWrap* req)
{
    const char* logContent = req->getUpdateLogRequest();
    Owned<IPropertyTree> logRequestTree = req->getLogRequestTree();
    Owned<IPropertyTree> updateLogRequestTree = createPTree("UpdateLogRequest");

    StringBuffer source;
    ///if (groupFilters.length() < 1) //TODO: the filters will be re-designed 
    {//No filter
        if (logRequestTree)
        {
            updateLogRequestTree->addPropTree(logRequestTree->queryName(), LINK(logRequestTree));
        }
        else if (!isEmptyString(logContent))
        {
            Owned<IPropertyTree> pTree = createPTreeFromXMLString(logContent);
            source = pTree->queryProp("Source");
            updateLogRequestTree->addPropTree(pTree->queryName(), LINK(pTree));
        }
        else
        {
            Owned<IPropertyTree> espContext = req->getESPContext();
            Owned<IPropertyTree> userContext = req->getUserContext();
            Owned<IPropertyTree> userRequest = req->getUserRequest();
            const char* userResp = req->getUserResponse();
            const char* logDatasets = req->getLogDatasets();
            const char* backEndReq = req->getBackEndRequest();
            const char* backEndResp = req->getBackEndResponse();
            if (!espContext && !userContext && !userRequest && isEmptyString(userResp) && isEmptyString(logDatasets)
                && isEmptyString(backEndResp) && isEmptyString(backEndResp))
                throw MakeStringException(EspLoggingErrors::UpdateLogFailed, "Failed to read log content");
            source = userContext->queryProp("Source");

            StringBuffer espContextXML, userContextXML, userRequestXML;
            IPropertyTree* logContentTree = ensurePTree(updateLogRequestTree, "LogContent");
            if (espContext)
            {
                logContentTree->addPropTree(espContext->queryName(), LINK(espContext));
            }
            if (userContext)
            {
                IPropertyTree* pTree = ensurePTree(logContentTree, espLogContentGroupNames[ESPLCGUserContext]);
                pTree->addPropTree(userContext->queryName(), LINK(userContext));
            }
            if (userRequest)
            {
                IPropertyTree* pTree = ensurePTree(logContentTree, espLogContentGroupNames[ESPLCGUserReq]);
                pTree->addPropTree(userRequest->queryName(), LINK(userRequest));
            }
            if (!isEmptyString(userResp))
            {
                IPropertyTree* pTree = ensurePTree(logContentTree, espLogContentGroupNames[ESPLCGUserResp]);
                Owned<IPropertyTree> userRespTree = createPTreeFromXMLString(userResp);
                pTree->addPropTree(userRespTree->queryName(), LINK(userRespTree));
            }
            if (!isEmptyString(logDatasets))
            {
                IPropertyTree* pTree = ensurePTree(logContentTree, espLogContentGroupNames[ESPLCGLogDatasets]);
                Owned<IPropertyTree> logDatasetTree = createPTreeFromXMLString(logDatasets);
                pTree->addPropTree(logDatasetTree->queryName(), LINK(logDatasetTree));
            }
            if (!isEmptyString(backEndReq))
                logContentTree->addProp(espLogContentGroupNames[ESPLCGBackEndReq], backEndReq);
            if (!isEmptyString(backEndResp))
                logContentTree->addProp(espLogContentGroupNames[ESPLCGBackEndResp], backEndResp);
        }
    }

    if (!source.isEmpty())
        updateLogRequestTree->addProp("LogContent/Source", source.str());

    const char* option = req->getOption();
    if (!isEmptyString(option))
        updateLogRequestTree->addProp("Option", option);

    StringBuffer updateLogRequestXML;
    toXML(updateLogRequestTree, updateLogRequestXML);
    ESPLOG(LogMax, "filtered content and option: <%s>", updateLogRequestXML.str());

    req->clearOriginalContent();
    req->setUpdateLogRequest(updateLogRequestXML.str());
}

unsigned CLoggingManager::serializeLogRequestContent(IEspUpdateLogRequestWrap* request, const char* GUID, StringBuffer& logData)
{
    logData.appendf("<%s>", LOGREQUEST_GUID);
    logData.append(GUID);
    logData.appendf("</%s>", LOGREQUEST_GUID);

    const char* option = request->getOption();
    if (!isEmptyString(option))
    {
        logData.appendf("<%s>", LOGREQUEST_OPTION);
        logData.append(option);
        logData.appendf("</%s>", LOGREQUEST_OPTION);
    }

    StringBuffer buffer;
    const char* logRequest = request->getUpdateLogRequest();
    JBASE64_Encode(logRequest, strlen(logRequest), buffer);

    logData.appendf("<%s>", LOGREQUEST);
    logData.append(buffer);
    logData.appendf("</%s>", LOGREQUEST);

    return logData.length();
}

bool CLoggingManager::getTransactionSeed(StringBuffer& transactionSeed, StringBuffer& status)
{
    if (!initialized)
        throw MakeStringException(-1,"LoggingManager not initialized");

    bool bRet = false;
    try
    {
        Owned<IEspGetTransactionSeedRequest> req =  createGetTransactionSeedRequest();
        Owned<IEspGetTransactionSeedResponse> resp =  createGetTransactionSeedResponse();
        transactionSeed.set("Seed");

        bRet = getTransactionSeed(*req, *resp);
        if (bRet && !resp->getStatusCode())
        {
            const char* seed = resp->getSeedId();
            if (!seed || !*seed)
                status.set("Failed to get Transaction Seed");
            else
            {
                transactionSeed.set(seed);
                status.set("Transaction Seed returned.");
                bRet = true;
            }
        }
        else
        {
            const char* statusMsg = resp->getStatusMessage();
            if (statusMsg && *statusMsg)
                status.setf("Failed to get Transaction Seed: %s", statusMsg);
            else
                status.set("Failed to get Transaction Seed");
        }
    }
    catch (IException* e)
    {
        e->errorMessage(status);
        status.insert(0, "Failed to get Transaction Seed: ");
        ERRLOG("%s",status.str());
        e->Release();
    }

    return bRet;
}


bool CLoggingManager::getTransactionSeed(IEspGetTransactionSeedRequest& req, IEspGetTransactionSeedResponse& resp)
{
    if (!initialized)
        throw MakeStringException(-1,"LoggingManager not initialized");

    bool bRet = false;
    try
    {
        for (unsigned int x = 0; x < loggingAgentThreads.size(); x++)
        {
            IUpdateLogThread* loggingThread = loggingAgentThreads[x];
            if (!loggingThread->hasService(LGSTGetTransactionSeed))
                continue;

            IEspLogAgent* loggingAgent = loggingThread->getLogAgent();
            bRet = loggingAgent->getTransactionSeed(req, resp);
            if (bRet)
                break;
        }
    }
    catch (IException* e)
    {
        StringBuffer errorStr;
        e->errorMessage(errorStr);
        ERRLOG("Failed to get Transaction Seed: %s",errorStr.str());
        resp.setStatusCode(-1);
        resp.setStatusMessage(errorStr.str());
        e->Release();
    }

    return bRet;
}

bool CLoggingManager::getTransactionID(StringAttrMapping* transFields, StringBuffer& transactionID, StringBuffer& status)
{
    if (!initialized)
        throw MakeStringException(-1,"LoggingManager not initialized");

    try
    {
        for (unsigned int x = 0; x < loggingAgentThreads.size(); x++)
        {
            IUpdateLogThread* loggingThread = loggingAgentThreads[x];
            if (!loggingThread->hasService(LGSTGetTransactionID))
                continue;

            IEspLogAgent* loggingAgent = loggingThread->getLogAgent();
            loggingAgent->getTransactionID(transFields, transactionID);
            if (!transactionID.isEmpty())
                ESPLOG(LogMax, "Got TransactionID '%s'", transactionID.str());
            return true;
        }
    }
    catch (IException* e)
    {
        e->errorMessage(status);
        e->Release();
    }

    return false;
}

extern "C"
{
LOGGINGMANAGER_API ILoggingManager* newLoggingManager()
{
    return new CLoggingManager();
}
}
