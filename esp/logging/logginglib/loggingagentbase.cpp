/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2016 HPCC Systems.

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
#include "loggingagentbase.hpp"
#include <set>

static const char* const defaultTransactionTable = "transactions";
static const char* const defaultTransactionAppName = "accounting_log";
static const char* const defaultLoggingTransactionAppName = "logging_transaction";

void CLogContentFilter::readAllLogFilters(IPropertyTree* cfg)
{
    bool groupFilterRead = false;
    for (unsigned i = 0; i < ESPLCGAll; i++)
    {
        if (readLogFilters(cfg, i))
            groupFilterRead = true;
    }

    if (!groupFilterRead)
    {
        groupFilters.clear();
        readLogFilters(cfg, ESPLCGAll);
    }
}

bool CLogContentFilter::readLogFilters(IPropertyTree* cfg, unsigned groupID)
{
    StringBuffer xpath;
    if (groupID != ESPLCGAll)
        xpath.appendf("Filters/Filter[@type='%s']", espLogContentGroupNames[groupID]);
    else
        xpath.append("Filters/Filter");

    if ((groupID == ESPLCGBackEndReq) || (groupID == ESPLCGBackEndResp))
    {
        IPropertyTree* filter = cfg->queryBranch(xpath.str());
        if (filter)
        {
            if (filter->hasProp("@value"))
            {
                if (!filter->getPropBool("@value"))
                {
                    Owned<CESPLogContentGroupFilters> espLogContentGroupFilters = new CESPLogContentGroupFilters((ESPLogContentGroup) groupID);
                    groupFilters.append(*espLogContentGroupFilters.getClear());
                }
            }
            else if (filter->hasProp("@removal") && filter->getPropBool("@removal"))
            {
                Owned<CESPLogContentGroupFilters> espLogContentGroupFilters = new CESPLogContentGroupFilters((ESPLogContentGroup) groupID);
                groupFilters.append(*espLogContentGroupFilters.getClear());
            }
        }

        return filter && (filter->hasProp("@value") || filter->hasProp("@removal"));
    }

    Owned<CESPLogContentGroupFilters> espLogContentGroupFilters = new CESPLogContentGroupFilters((ESPLogContentGroup) groupID);
    Owned<IPropertyTreeIterator> filters = cfg->getElements(xpath.str());
    ForEach(*filters)
    {
        IPropertyTree &filter = filters->query();
        StringBuffer value(filter.queryProp("@value"));
        if (!value.length())
            continue;

        if (!espLogContentGroupFilters->checkAndSetRemoval(filter.getPropBool("@removal")))
            throw makeStringExceptionV(-1, "The same filter type (%s) must have the same @removal value.", espLogContentGroupNames[groupID]);

        //clean "//"
        unsigned idx = value.length()-1;
        while (idx)
        {
            if ((value.charAt(idx-1) == '/') && (value.charAt(idx) == '/'))
                value.remove(idx, 1);
            idx--;
        }

        //clean "/*" at the end
        while ((value.length() > 1) && (value.charAt(value.length()-2) == '/') && (value.charAt(value.length()-1) == '*'))
            value.setLength(value.length() - 2);

        if (value.length() && !streq(value.str(), "*") && !streq(value.str(), "/") && !streq(value.str(), "*/"))
        {
            espLogContentGroupFilters->addFilter(value.str());
        }
        else
        {
            if (espLogContentGroupFilters->isRemoval() && streq(value.str(), "*"))
                espLogContentGroupFilters->setRemovalAll(true);
            else
                espLogContentGroupFilters->clearFilters();
            break;
        }
    }

    bool hasFilter = espLogContentGroupFilters->isRemovalAll() || (espLogContentGroupFilters->getFilterCount() > 0);
    if (hasFilter)
        groupFilters.append(*espLogContentGroupFilters.getClear());
    return hasFilter;
}

void CLogContentFilter::addLogContentBranch(StringArray& branchNames, IPropertyTree* contentToLogBranch, IPropertyTree* updateLogRequestTree)
{
    IPropertyTree* pTree = updateLogRequestTree;
    unsigned numOfBranchNames = branchNames.length();
    unsigned i = 0;
    while (i < numOfBranchNames)
    {
        const char* branchName = branchNames.item(i);
        if (branchName && *branchName)
            pTree = ensurePTree(pTree, branchName);
        i++;
    }
    pTree->addPropTree(contentToLogBranch->queryName(), LINK(contentToLogBranch));
}

void CLogContentFilter::filterAndAddLogContentBranch(StringArray& branchNamesInFilter, unsigned idx,
    StringArray& branchNamesInLogContent, IPropertyTree* originalLogContentBranch, IPropertyTree* updateLogRequestTree, bool& logContentEmpty)
{
    Owned<IPropertyTreeIterator> contentItr = originalLogContentBranch->getElements(branchNamesInFilter.item(idx));
    ForEach(*contentItr)
    {
        IPropertyTree& contentToLogBranch = contentItr->query();
        if (idx == branchNamesInFilter.length() - 1)
        {
            addLogContentBranch(branchNamesInLogContent, &contentToLogBranch, updateLogRequestTree);
            logContentEmpty = false;
        }
        else
        {
            branchNamesInLogContent.append(contentToLogBranch.queryName());
            filterAndAddLogContentBranch(branchNamesInFilter, idx+1, branchNamesInLogContent, &contentToLogBranch,
                updateLogRequestTree, logContentEmpty);
            branchNamesInLogContent.remove(branchNamesInLogContent.length() - 1);
        }
    }
}

void CLogContentFilter::filterLogContentTree(CESPLogContentGroupFilters& filtersGroup, IPropertyTree* originalContentTree, IPropertyTree* newLogContentTree, bool& logContentEmpty)
{
    bool removal = filtersGroup.isRemoval();
    StringArray& filters = filtersGroup.getFilters();
    ForEachItemIn(i, filters)
    {
        const char* logContentFilter = filters.item(i);
        if(!logContentFilter || !*logContentFilter)
            continue;

        if (removal)
        {
            bool more;
            do more = newLogContentTree->removeProp(logContentFilter);
            while(more);
            continue;
        }

        StringArray branchNamesInFilter, branchNamesInLogContent;
        branchNamesInFilter.appendListUniq(logContentFilter, "/");
        filterAndAddLogContentBranch(branchNamesInFilter, 0, branchNamesInLogContent, originalContentTree, newLogContentTree, logContentEmpty);
    }
}

IEspUpdateLogRequestWrap* CLogContentFilter::filterLogContent(IEspUpdateLogRequestWrap* req)
{
    bool noFilter = (groupFilters.length() < 1);
    Owned<IPropertyTree> logRequestTree = req->getLogRequestTree();
    const char* logContent = req->getUpdateLogRequest();
    if (logRequestTree || !isEmptyString(logContent))
    {
        if (noFilter)
        {
            if (!logRequestTree)
                return LINK(req);

            //The first version of the filterLogContent() always returns the UpdateLogRequest string.
            StringBuffer updateLogRequestXML;
            toXML(logRequestTree, updateLogRequestXML);
            req->setUpdateLogRequest(updateLogRequestXML);
            return LINK(req);
        }

        if (!logRequestTree)
            logRequestTree.setown(createPTreeFromXMLString(logContent));

        Owned<IPropertyTree> filteredLogRequestTree;
        CESPLogContentGroupFilters& filtersGroup = groupFilters.item(0);
        if (filtersGroup.getGroup() == ESPLCGAll)
        {
            bool logContentEmpty = true;
            if (filtersGroup.isRemoval())
            {
                filteredLogRequestTree.setown(createPTreeFromIPT(logRequestTree));
                filterLogContentTree(filtersGroup, logRequestTree, filteredLogRequestTree, logContentEmpty);
            }
            else
            {
                filteredLogRequestTree.setown(createPTree(logRequestTree->queryName()));
                filterLogContentTree(filtersGroup, logRequestTree, filteredLogRequestTree, logContentEmpty);
                if (logContentEmpty)
                    throw makeStringExceptionV(EspLoggingErrors::UpdateLogFailed, "The filtered content is empty.");
            }
        }
        else
        {
            filteredLogRequestTree.setown(createPTree(logRequestTree->queryName()));
            ensurePTree(filteredLogRequestTree, "LogContent");
            filterLogContentTreeUsingGroupFilters(logRequestTree, filteredLogRequestTree);
        }

        StringBuffer updateLogRequestXML;
        toXML(filteredLogRequestTree, updateLogRequestXML);
        ESPLOG(LogMax, "filtered content and option: <%s>", updateLogRequestXML.str());

        req->setUpdateLogRequest(updateLogRequestXML);
        if (logRequestTree)
            req->setLogRequestTree(filteredLogRequestTree.getClear());
        return LINK(req);
    }

    Owned<IPropertyTree> updateLogRequestTree = createPTree("UpdateLogRequest");
    IPropertyTree* logContentTree = ensurePTree(updateLogRequestTree, "LogContent");
    StringBuffer source;
    if (noFilter || skipFilter)
    {
        readLogRequest(req, source, logContentTree);
    }
    else
    {
        CESPLogContentGroupFilters& filtersGroup = groupFilters.item(0);
        if (filtersGroup.getGroup() == ESPLCGAll)
            readLogRequestWithAllFilter(req, source, logContentTree);
        else
            readLogRequestWithGroupFilters(req, source, logContentTree);
    }

    Owned<IPropertyTree> scriptValues = req->getScriptValuesTree();
    if (scriptValues)
        updateLogRequestTree->addPropTree(scriptValues->queryName(), LINK(scriptValues));

    if (!source.isEmpty())
        updateLogRequestTree->addProp("LogContent/Source", source.str());

    const char* option = req->getOption();
    if (option && *option)
        updateLogRequestTree->addProp("Option", option);

    StringBuffer updateLogRequestXML;
    toXML(updateLogRequestTree, updateLogRequestXML);
    ESPLOG(LogMax, "filtered content and option: <%s>", updateLogRequestXML.str());

    Owned<IEspUpdateLogRequestWrap> newReq = new CUpdateLogRequestWrap(req->getGUID(), req->getOption(), updateLogRequestXML.str());
    if (scriptValues)
        newReq->setScriptValuesTree(scriptValues);
    return newReq.getClear();
}

void CLogContentFilter::filterLogContentTreeUsingGroupFilters(IPropertyTree* originalContentTree, IPropertyTree* newLogContentTree)
{
    bool logContentEmpty = true;
    for (unsigned group = 0; group < ESPLCGAll; group++)
    {
        bool groupNoFilter  = true;
        const char* xpath = espLogContentXPaths[group];
        ForEachItemIn(i, groupFilters)
        {
            CESPLogContentGroupFilters& filtersGroup = groupFilters.item(i);
            if (filtersGroup.getGroup() != group)
                continue;

            groupNoFilter  = false;
            if ((group == ESPLCGBackEndReq) || (group == ESPLCGBackEndResp))
                break; //The group filter for ESPLCGBackEndReq or ESPLCGBackEndResp means removalAll.

            if (filtersGroup.isRemovalAll())
                break;

            IPropertyTree* originalGroup = originalContentTree->queryPropTree(xpath);
            if (!originalGroup)
                break;

            IPropertyTree* newGroupTree = nullptr;
            if (filtersGroup.isRemoval())
            {
                newGroupTree = newLogContentTree->addPropTree(xpath, createPTreeFromIPT(originalGroup));
                //For ESPLCGUserReq and ESPLCGUserResp, the filter xpath is defined based on the data
                //branch inside the originalGroup. The name of the data branch is given by ESDL service.
                if ((group == ESPLCGUserReq) || (group == ESPLCGUserResp))
                    newGroupTree = getFirstBranch(newGroupTree);
                if (!newGroupTree)
                    break;
                logContentEmpty = false;
            }
            else
            {
                newGroupTree = ensurePTree(newLogContentTree, xpath);
                if ((group == ESPLCGUserReq) || (group == ESPLCGUserResp))
                {
                    originalGroup = getFirstBranch(originalGroup);
                    if (!originalGroup)
                        break;
                    newGroupTree = ensurePTree(newGroupTree, originalGroup->queryName());
                }
            }

            filterLogContentTree(filtersGroup, originalGroup, newGroupTree, logContentEmpty);
            break;
        }

        if (groupNoFilter)
        { //add all
            if ((group == ESPLCGBackEndReq) || (group == ESPLCGBackEndResp))
            {
                const char* data = originalContentTree->queryProp(xpath);
                if (!isEmptyString(data))
                {
                    newLogContentTree->addProp(xpath, data);
                    logContentEmpty = false;
                }
            }
            else
            {
                IPropertyTree* originalGroup = originalContentTree->queryPropTree(xpath);
                if (originalGroup)
                {
                    newLogContentTree->addPropTree(xpath, createPTreeFromIPT(originalGroup));
                    logContentEmpty = false;
                }
            }
        }
    }

    if (logContentEmpty)
        throw makeStringExceptionV(EspLoggingErrors::UpdateLogFailed, "The filtered content is empty.");
}

IPropertyTree* CLogContentFilter::getFirstBranch(IPropertyTree* root)
{
    IPropertyTree* branch = nullptr;
    Owned<IPropertyTreeIterator> itr = root->getElements("*");
    ForEach(*itr)
    {
        branch = &itr->query();
        break;
    }
    return branch;
}

void CLogContentFilter::readLogRequest(IEspUpdateLogRequestWrap* req, StringBuffer& source, IPropertyTree* logContentTree)
{
    Owned<IPropertyTree> espContext = req->getESPContext();
    Owned<IPropertyTree> userContext = req->getUserContext();
    Owned<IPropertyTree> userRequest = req->getUserRequest();
    const char* userResp = req->getUserResponse();
    const char* logDatasets = req->getLogDatasets();
    const char* backEndReq = req->getBackEndRequest();
    const char* backEndResp = req->getBackEndResponse();
    if (!espContext && !userContext && !userRequest && (!userResp || !*userResp) && (!backEndResp || !*backEndResp))
        throw makeStringExceptionV(EspLoggingErrors::UpdateLogFailed, "Failed to read log content");

    source = userContext->queryProp("Source");

    StringBuffer espContextXML, userContextXML, userRequestXML;
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

void CLogContentFilter::readLogRequestWithAllFilter(IEspUpdateLogRequestWrap* req, StringBuffer& source, IPropertyTree* logContentTree)
{
    bool logContentEmpty = true;
    CESPLogContentGroupFilters& filtersGroup = groupFilters.item(0);
    if (filtersGroup.isRemoval())
    {
        readLogRequest(req, source, logContentTree);
        filterLogContentTree(filtersGroup, nullptr, logContentTree, logContentEmpty);
    }
    else
    {
        Owned<IPropertyTree> logRequestTree = createPTree("LogContent");
        readLogRequest(req, source, logRequestTree);

        filterLogContentTree(groupFilters.item(0), logRequestTree, logContentTree, logContentEmpty);
        if (logContentEmpty)
            throw makeStringExceptionV(EspLoggingErrors::UpdateLogFailed, "The filtered content is empty.");
    }
}

void CLogContentFilter::readLogRequestWithGroupFilters(IEspUpdateLogRequestWrap* req, StringBuffer& source, IPropertyTree* logContentTree)
{
    bool logContentEmpty = true;
    for (unsigned group = 0; group < ESPLCGAll; group++)
    {
        if ((group == ESPLCGBackEndReq) || (group == ESPLCGBackEndResp))
        {
            bool noFilter = true;
            ForEachItemIn(i, groupFilters)
            {
                CESPLogContentGroupFilters& filtersGroup = groupFilters.item(i);
                if (filtersGroup.getGroup() == group)
                {
                    noFilter = false;
                    break;
                }
            }

            if (noFilter)
            { //No filter for this group (ESPLCGBackEndReq or ESPLCGBackEndResp). Add it to the logContentTree.
                const char* data = (group == ESPLCGBackEndReq) ? req->getBackEndRequest() : req->getBackEndResponse();
                if (!isEmptyString(data))
                {
                    logContentTree->addProp(espLogContentGroupNames[group], data);
                    logContentEmpty = false;
                }
            }
            continue;
        }

        Owned<IPropertyTree> originalContentTree;
        if (group == ESPLCGESPContext)
            originalContentTree.setown(req->getESPContext());
        else if (group == ESPLCGUserContext)
        {
            originalContentTree.setown(req->getUserContext());
            source = originalContentTree->queryProp("Source");
        }
        else if (group == ESPLCGUserReq)
            originalContentTree.setown(req->getUserRequest());
        else if (group == ESPLCGLogDatasets)
        {
            const char* logDatasets = req->getLogDatasets();
            if (logDatasets && *logDatasets)
                originalContentTree.setown(createPTreeFromXMLString(logDatasets));
        }
        else //group = ESPLCGUserResp
        {
            const char* resp = req->getUserResponse();
            if (!resp || !*resp)
                continue;
            originalContentTree.setown(createPTreeFromXMLString(resp));
        }
        if (!originalContentTree)
            continue;

        bool foundGroupFilters  = false;
        ForEachItemIn(i, groupFilters)
        {
            CESPLogContentGroupFilters& filtersGroup = groupFilters.item(i);
            if (filtersGroup.getGroup() == group)
            {
                foundGroupFilters  =  true;
                if (filtersGroup.isRemovalAll())
                    break;

                IPropertyTree* newContentTree = nullptr;
                if (filtersGroup.isRemoval())
                {
                    if (group == ESPLCGESPContext)
                        newContentTree = logContentTree->addPropTree(espLogContentGroupNames[group], createPTreeFromIPT(originalContentTree));
                    else //For non ESPLCGESPContext, we want to keep the root of original tree.
                    {
                        newContentTree = ensurePTree(logContentTree, espLogContentGroupNames[group]);
                        newContentTree = newContentTree->addPropTree(originalContentTree->queryName(), createPTreeFromIPT(originalContentTree));
                    }
                    logContentEmpty = false;
                }
                else
                {
                    newContentTree = ensurePTree(logContentTree, espLogContentGroupNames[group]);
                    if (group != ESPLCGESPContext)//For non ESPLCGESPContext, we want to keep the root of original tree.
                        newContentTree = ensurePTree(newContentTree, originalContentTree->queryName());
                }
                filterLogContentTree(filtersGroup, originalContentTree, newContentTree, logContentEmpty);
                break;
            }
        }

        if (!foundGroupFilters )
        {
            if (group == ESPLCGESPContext)
            {
                //The ESPContext tree itself already has the /ESPContext node
                //as the top tree node. We should not add another /ESPContext
                //node on the top of the ESPContext tree.
                logContentTree->addPropTree(originalContentTree->queryName(), LINK(originalContentTree));
            }
            else
            {
                IPropertyTree* newContentTree = ensurePTree(logContentTree, espLogContentGroupNames[group]);
                newContentTree->addPropTree(originalContentTree->queryName(), LINK(originalContentTree));
            }
            logContentEmpty = false;
        }
    }
    if (logContentEmpty)
        throw makeStringExceptionV(EspLoggingErrors::UpdateLogFailed, "The filtered content is empty.");
}

CLogAgentBase::CVariantIterator::CVariantIterator(const CLogAgentBase& agent)
    : m_agent(&agent)
    , m_variantIt(m_agent->agentVariants.end())
{
}

CLogAgentBase::CVariantIterator::~CVariantIterator()
{
}

bool CLogAgentBase::CVariantIterator::first()
{
    m_variantIt = m_agent->agentVariants.begin();
    return isValid();
}

bool CLogAgentBase::CVariantIterator::next()
{
    if (m_variantIt != m_agent->agentVariants.end())
        ++m_variantIt;
    return isValid();
}

bool CLogAgentBase::CVariantIterator::isValid()
{
    return (m_variantIt != m_agent->agentVariants.end());
}

const IEspLogAgentVariant& CLogAgentBase::CVariantIterator::query()
{
    if (!isValid())
        throw MakeStringException(0, "CVariantIterator::query called in invalid state");
    IEspLogAgentVariant* entry = m_variantIt->get();
    if (nullptr == entry)
        throw MakeStringException(0, "CVariantIterator::query encountered a NULL variant");
    return *entry;
}

CLogAgentBase::CVariant::CVariant(const char* name, const char* type, const char* group)
{
    m_name.setown(normalize(name));
    m_type.setown(normalize(type));
    m_group.setown(normalize(group));
}

String* CLogAgentBase::CVariant::normalize(const char* token)
{
    struct PoolComparator
    {
        bool operator () (const Owned<String>& lhs, const Owned<String>& rhs) const
        {
            String* lPtr = lhs.get();
            String* rPtr = rhs.get();

            if (lPtr == rPtr)
                return false;
            if (nullptr == lPtr)
                return false;
            if (nullptr == rPtr)
                return true;
            return lhs->compareTo(*rhs.get()) < 0;
        }
    };
    using PooledStrings = std::set<Owned<String>, PoolComparator>;
    static PooledStrings pool;
    static CriticalSection poolLock;

    Owned<String> tmp(new String(StringBuffer(token).trim().toLowerCase()));
    CriticalBlock block(poolLock);
    PooledStrings::iterator it = pool.find(tmp);

    if (it != pool.end())
        return it->getLink();
    pool.insert(tmp);
    return tmp.getClear();
}

bool CLogAgentBase::initVariants(IPropertyTree* cfg)
{
    Owned<IPropertyTreeIterator> variantNodes(cfg->getElements("//Variant"));
    ForEach(*variantNodes)
    {
        IPTree& variant = variantNodes->query();
        const char* type = variant.queryProp("@type");
        const char* group = variant.queryProp("@group");
        Owned<CVariant> instance = new CVariant(agentName.get(), type, group);
        Variants::iterator it = agentVariants.find(instance);

        if (agentVariants.end() == it)
            agentVariants.insert(instance.getClear());
    }

    if (agentVariants.empty())
        agentVariants.insert(new CVariant(agentName.get(), nullptr, nullptr));
    return true;
}

IEspLogAgentVariantIterator* CLogAgentBase::getVariants() const
{
    return new CVariantIterator(*this);
}

void CDBLogAgentBase::readDBCfg(IPropertyTree* cfg, StringBuffer& server, StringBuffer& dbUser, StringBuffer& dbPassword)
{
    ensureInputString(cfg->queryProp("@server"), true, server, -1, "Database server required");
    ensureInputString(cfg->queryProp("@dbName"), true, defaultDB, -1, "Database name required");

    transactionTable.set(cfg->hasProp("@dbTableName") ? cfg->queryProp("@dbTableName") : defaultTransactionTable);
    dbUser.set(cfg->queryProp("@dbUser"));
    const char* encodedPassword = cfg->queryProp("@dbPassWord");
    if(encodedPassword && *encodedPassword)
        decrypt(dbPassword, encodedPassword);
}

void CDBLogAgentBase::readTransactionCfg(IPropertyTree* cfg)
{
    //defaultTransactionApp: if no APP name is given, which APP name (TableName) should be used to get a transaction seed?
    //loggingTransactionApp: the TableName used to get a transaction seed for this logging agent
    defaultTransactionApp.set(cfg->hasProp("defaultTransaction") ? cfg->queryProp("defaultTransaction") : defaultTransactionAppName);
    loggingTransactionApp.set(cfg->hasProp("loggingTransaction") ? cfg->queryProp("loggingTransaction") : defaultLoggingTransactionAppName);
    loggingTransactionCount = 0;
}

bool CDBLogAgentBase::getTransactionSeed(IEspGetTransactionSeedRequest& req, IEspGetTransactionSeedResponse& resp)
{
    if (!hasService(LGSTGetTransactionSeed))
        throw MakeStringException(EspLoggingErrors::GetTransactionSeedFailed, "%s: no getTransactionSeed service configured", agentName.get());

    bool bRet = false;
    StringBuffer appName(req.getApplication());
    appName.trim();
    if (appName.length() == 0)
        appName = defaultTransactionApp.get();

    unsigned retry = 1;
    while (1)
    {
        try
        {
            StringBuffer logSeed;
            queryTransactionSeed(appName, logSeed);
            if (!logSeed.length())
                throw MakeStringException(EspLoggingErrors::GetTransactionSeedFailed, "Failed to get TransactionSeed");

            resp.setSeedId(logSeed.str());
            resp.setStatusCode(0);
            bRet = true;
            break;
        }
        catch (IException* e)
        {
            StringBuffer errorStr, errorMessage;
            errorMessage.append("Failed to get TransactionSeed: error code ").append(e->errorCode()).append(", error message ").append(e->errorMessage(errorStr));
            OERRLOG("%s -- try %d", errorMessage.str(), retry);
            e->Release();
            if (retry < maxTriesGTS)
            {
                Sleep(retry*3000);
                retry++;
            }
            else
            {
                resp.setStatusCode(-1);
                resp.setStatusMessage(errorMessage.str());
                break;
            }
        }
    }
    return bRet;
}

bool CDBLogAgentBase::updateLog(IEspUpdateLogRequestWrap& req, IEspUpdateLogResponse& resp)
{
    if (!hasService(LGSTUpdateLOG))
        throw MakeStringException(EspLoggingErrors::UpdateLogFailed, "%s: no updateLog service configured", agentName.get());

    unsigned startTime = (getEspLogLevel()>=LogNormal) ? msTick() : 0;
    bool ret = false;
    try
    {
        const char* updateLogReq = req.getUpdateLogRequest();
        if (!updateLogReq || !*updateLogReq)
            throw MakeStringException(EspLoggingErrors::UpdateLogFailed, "Failed to read log request.");

        StringBuffer requestBuf, logDB, logSource;
        requestBuf.append("<LogRequest>").append(updateLogReq).append("</LogRequest>");
        Owned<IPropertyTree> logRequestTree = createPTreeFromXMLString(requestBuf.length(), requestBuf.str());
        if (!logRequestTree)
            throw MakeStringException(EspLoggingErrors::UpdateLogFailed, "Failed to read log request.");

        CLogGroup* logGroup = checkLogSource(logRequestTree, logSource, logDB);
        if (!logGroup)
            throw MakeStringException(EspLoggingErrors::UpdateLogFailed, "Log Group %s undefined.", logSource.str());

        StringBuffer logID;
        getLoggingTransactionID(logID);

        CIArrayOf<CLogTable>& logTables = logGroup->getLogTables();
        ForEachItemIn(i, logTables)
        {
            CLogTable& table = logTables.item(i);

            StringBuffer updateDBStatement;
            if(!buildUpdateLogStatement(logRequestTree, logDB.str(), table, logID, updateDBStatement))
                throw MakeStringException(EspLoggingErrors::UpdateLogFailed, "Failed in creating SQL statement.");

            ESPLOG(LogNormal, "LAgent UpdateLog BuildStat %d done: %dms\n", i, msTick() -  startTime);
            ESPLOG(LogMax, "UpdateLog: %s\n", updateDBStatement.str());

            executeUpdateLogStatement(updateDBStatement);
            ESPLOG(LogNormal, "LAgent UpdateLog ExecStat %d done: %dms\n", i, msTick() -  startTime);
        }
        resp.setStatusCode(0);
        ret = true;
    }
    catch (IException* e)
    {
        StringBuffer errorStr, errorMessage;
        errorMessage.append("Failed to update log: error code ").append(e->errorCode()).append(", error message ").append(e->errorMessage(errorStr));
        OERRLOG("%s", errorMessage.str());
        e->Release();
        resp.setStatusCode(-1);
        resp.setStatusMessage(errorMessage.str());
    }
    ESPLOG(LogNormal, "LAgent UpdateLog total=%dms\n", msTick() -  startTime);
    return ret;
}

CLogGroup* CDBLogAgentBase::checkLogSource(IPropertyTree* logRequest, StringBuffer& source, StringBuffer& logDB)
{
    if (logSourceCount == 0)
    {//if no log source is configured, use default Log Group and DB
        logDB.set(defaultDB.str());
        source.set(defaultLogGroup.get());
        return logGroups.getValue(defaultLogGroup.get());
    }
    source = logRequest->queryProp(logSourcePath.get());
    if (source.isEmpty())
        throw MakeStringException(EspLoggingErrors::UpdateLogFailed, "Failed to read log Source from request.");
    CLogSource* logSource = logSources.getValue(source.str());
    if (!logSource)
        throw MakeStringException(EspLoggingErrors::UpdateLogFailed, "Log Source %s undefined.", source.str());

    logDB.set(logSource->getDBName());
    return logGroups.getValue(logSource->getGroupName());
}

void CDBLogAgentBase::getLoggingTransactionID(StringBuffer& id)
{
    id.set(loggingTransactionSeed.str()).append("-").append(++loggingTransactionCount);
}

bool CDBLogAgentBase::buildUpdateLogStatement(IPropertyTree* logRequest, const char* logDB,
    CLogTable& table, StringBuffer& logID, StringBuffer& updateDBStatement)
{
    StringBuffer fields, values;
    BoolHash handledFields;
    CIArrayOf<CLogField>& logFields = table.getLogFields();
    ForEachItemIn(i, logFields) //Go through data items to be logged
    {
        CLogField& logField = logFields.item(i);

        StringBuffer colName(logField.getMapTo());
        bool* found = handledFields.getValue(colName.str());
        if (found && *found)
            continue;

        StringBuffer path(logField.getName());
        if (path.charAt(path.length() - 1) == ']')
        {//Attr filter. Separate the last [] from the path.
            const char* pTr = path.str();
            const char* ppTr = strrchr(pTr, '[');
            if (!ppTr)
                continue;

            StringBuffer attr;
            attr.set(ppTr+1);
            attr.setLength(attr.length() - 1);
            path.setLength(ppTr - pTr);

            StringBuffer colValue;
            Owned<IPropertyTreeIterator> itr = logRequest->getElements(path.str());
            ForEach(*itr)
            {//Log the first valid match just in case more than one matches.
                IPropertyTree& ppTree = itr->query();
                colValue.set(ppTree.queryProp(attr.str()));
                if (colValue.length())
                {
                    addField(logField, colName.str(), colValue, fields, values);
                    handledFields.setValue(colName.str(), true);
                    break;
                }
            }
            continue;
        }

        Owned<IPropertyTreeIterator> itr = logRequest->getElements(path.str());
        ForEach(*itr)
        {
            IPropertyTree& ppTree = itr->query();

            StringBuffer colValue;
            if (ppTree.hasChildren()) //This is a tree branch.
                toXML(&ppTree, colValue);
            else
                ppTree.getProp(NULL, colValue);

            if (colValue.length())
            {
                addField(logField, colName.str(), colValue, fields, values);
                handledFields.setValue(colName.str(), true);
                break;
            }
        }
    }

    //add any default fields that may be required but not in request.
    addMissingFields(logFields, handledFields, fields, values);
    if (table.getEnableLogID()) {
        appendFieldInfo("log_id", logID, fields, values, true);
    }

    setUpdateLogStatement(logDB, table.getTableName(), fields.str(), values.str(), updateDBStatement);
    return true;
}

void CDBLogAgentBase::appendFieldInfo(const char* field, StringBuffer& value, StringBuffer& fields, StringBuffer& values, bool quoted)
{
    if(values.length() != 0)
        values.append(',');
    if (quoted)
        values.append('\'').append(value.length(), value.str()).append('\'');
    else
        values.append(value.length(), value.str());

    if(fields.length() != 0)
        fields.append(',');
    fields.append(field);
}

void CDBLogAgentBase::addMissingFields(CIArrayOf<CLogField>& logFields, BoolHash& handledFields, StringBuffer& fields, StringBuffer& values)
{
    ForEachItemIn(i, logFields) //Go through data items to be logged
    {
        CLogField& logField = logFields.item(i);
        const char* colName = logField.getMapTo();
        bool* found = handledFields.getValue(colName);
        if (found && *found)
            continue;
        StringBuffer value(logField.getDefault());
        if (!value.isEmpty())
            addField(logField, colName, value, fields, values);
    }
}

void CDBLogAgentBase::getTransactionID(StringAttrMapping* transFields, StringBuffer& transactionID)
{
    //Not implemented
}

IEspUpdateLogRequestWrap* CDBLogAgentBase::filterLogContent(IEspUpdateLogRequestWrap* req)
{
    //No filter in CDBSQLLogAgent
    return req;
}
