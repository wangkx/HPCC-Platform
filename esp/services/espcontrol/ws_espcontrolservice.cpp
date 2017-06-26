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

#ifdef _USE_OPENLDAP
#include "ldapsecurity.ipp"
#endif

#include "ws_espcontrolservice.hpp"
#include "jlib.hpp"
#include "exception_util.hpp"
#include "dasds.hpp"

#define SDS_LOCK_TIMEOUT (5*60*1000) // 5 mins

const char* CWSESPControlEx::readSessionState(int st)
{
    return (st < NumOfSessionStates) ? SessionStates[st] : "Unknown";
}

const char* CWSESPControlEx::readSessionTimeStamp(int t, StringBuffer& str)
{
    CDateTime time;
    time.set(t);
    return time.getString(str).str();
}

float CWSESPControlEx::readSessionTimeoutMin(int sessionTimeoutMinutes, int lastAccessed)
{
    CDateTime time, timeNow;
    timeNow.setNow();
    time.set(lastAccessed);
    return sessionTimeoutMinutes - (timeNow.getSimple() - time.getSimple())/60;
}

IEspSession* CWSESPControlEx::setSessionInfo(const char* sessionID, IPropertyTree* espSessionTree, const char* domainName, unsigned port, IEspSession* session)
{
    if (espSessionTree == nullptr)
        return nullptr;

    StringBuffer createTimeStr, lastAccessedStr;
    int lastAccessed = espSessionTree->getPropInt(PropSessionLastAccessed, 0);

    session->setAuthDomain(domainName);
    session->setPort(port);
    session->setSessionID(sessionID);
    session->setUserID(espSessionTree->queryProp(PropSessionUserID));
    session->setNetworkAddress(espSessionTree->queryProp(PropSessionNetworkAddress));
    session->setState(readSessionState(espSessionTree->getPropInt(PropSessionState, NumOfSessionStates)));
    session->setCreateTime(readSessionTimeStamp(espSessionTree->getPropInt(PropSessionCreateTime, 0), createTimeStr));
    session->setLastAccessed(readSessionTimeStamp(lastAccessed, lastAccessedStr));
    int* sessionTimeoutMinutes = sessionTimeoutMinutesMap.getValue(domainName);
    if (!sessionTimeoutMinutes)
        throw MakeStringException(ECLWATCH_INVALID_INPUT, "AuthDomain %s not found.", domainName);
    if (*sessionTimeoutMinutes < 0)
    {
        session->setTimeoutMinutes(-1);
        return session;
    }

    float timeout = readSessionTimeoutMin(*sessionTimeoutMinutes, lastAccessed);
    if (timeout < 0.0)
        session->setTimeoutMinutes(0);
    else
        session->setTimeoutMinutes((int) timeout);
    return session;
}

void CWSESPControlEx::init(IPropertyTree *cfg, const char *process, const char *service)
{
    if(cfg == NULL)
        throw MakeStringException(-1, "Can't initialize CWSESPControlEx, cfg is NULL");

    espProcess.set(process);

    VStringBuffer xpath("Software/EspProcess[@name=\"%s\"]", process);
    IPropertyTree* espCFG = cfg->queryPropTree(xpath.str());
    if (!espCFG)
        throw MakeStringException(-1, "Can't find EspBinding for %s", process);

    bool hasDefaultSessionDomain = false;
    Owned<IPropertyTreeIterator> it = espCFG->getElements("AuthDomains/AuthDomain");
    ForEach(*it)
    {
        IPropertyTree& authDomain = it->query();
        StringBuffer name = authDomain.queryProp("@domainName");
        if (name.isEmpty())
        {
            if (hasDefaultSessionDomain)
                throw MakeStringException(-1, ">1 AuthDomains are not named.");

            hasDefaultSessionDomain = true;
            name.set("default");
        }
        sessionTimeoutMinutesMap.setValue(name.str(), authDomain.getPropInt("@sessionTimeoutMinutes", 0));
    }
    if (sessionTimeoutMinutesMap.count() == 0)
        sessionTimeoutMinutesMap.setValue("default", ESP_SESSION_TIMEOUT);
}


bool CWSESPControlEx::onSetLogging(IEspContext& context, IEspSetLoggingRequest& req, IEspSetLoggingResponse& resp)
{
    try
    {
#ifdef _USE_OPENLDAP
        CLdapSecManager* secmgr = dynamic_cast<CLdapSecManager*>(context.querySecManager());
        if(secmgr && !secmgr->isSuperUser(context.queryUser()))
            throw MakeStringException(ECLWATCH_SUPER_USER_ACCESS_DENIED, "Failed to change log settings. Permission denied.");
#endif

        if (!m_container)
            throw MakeStringException(ECLWATCH_INTERNAL_ERROR, "Failed to access container.");

        if (!req.getLoggingLevel_isNull())
            m_container->setLogLevel(req.getLoggingLevel());
        if (!req.getLogRequests_isNull())
            m_container->setLogRequests(req.getLogRequests());
        if (!req.getLogResponses_isNull())
            m_container->setLogResponses(req.getLogResponses());
        resp.setStatus(0);
        resp.setMessage("Logging settings are updated.");
    }
    catch(IException* e)
    {
        FORWARDEXCEPTION(context, e, ECLWATCH_INTERNAL_ERROR);
    }
    return true;
}

IRemoteConnection* CWSESPControlEx::querySDSConnection(const char* xpath, unsigned mode, unsigned timeout)
{
    Owned<IRemoteConnection> globalLock = querySDS().connect(xpath, myProcessSession(), RTM_LOCK_READ, SESSION_SDS_LOCK_TIMEOUT);
    if (!globalLock)
        throw MakeStringException(ECLWATCH_INTERNAL_ERROR, "Unable to connect to ESP Session information in dali %s", xpath);
    return globalLock.getClear();
}

bool CWSESPControlEx::onSessionQuery(IEspContext& context, IEspSessionQueryRequest& req, IEspSessionQueryResponse& resp)
{
    try
    {
#ifdef _USE_OPENLDAP
        CLdapSecManager* secmgr = dynamic_cast<CLdapSecManager*>(context.querySecManager());
        if(secmgr && !secmgr->isSuperUser(context.queryUser()))
            throw MakeStringException(ECLWATCH_SUPER_USER_ACCESS_DENIED, "Failed to query session. Permission denied.");
#endif

        StringBuffer fromIP = req.getFromIP();
        CSessionState state = req.getState();
        StringBuffer authDomain = req.getAuthDomain();
        if (authDomain.trim().isEmpty())
            authDomain.set("default");
        unsigned port = 8010;
        if (!req.getPort_isNull())
            port = req.getPort();

        VStringBuffer xpath("/%s/%s[@name='%s']/%s[@name='%s']/%s[@port=\"%d\"]", PathSessionRoot, PathSessionProcess,
            espProcess.get(), PathSessionDomain, authDomain.str(), PathSessionApplication, port);
        Owned<IRemoteConnection> globalLock = querySDSConnection(xpath.str(), RTM_LOCK_READ, SESSION_SDS_LOCK_TIMEOUT);

        IArrayOf<IEspSession> sessions;
        getSessionXPath(0, xpath);
        if (!fromIP.trim().isEmpty())
            xpath.appendf("[%s='%s']", PropSessionNetworkAddress, fromIP.str());
        else if (state != SessionState_Undefined)
            xpath.appendf("[%s='%d']", PropSessionState, state);
        Owned<IPropertyTreeIterator> iter = globalLock->getElements(xpath.str());
        ForEach(*iter)
        {
            IPropertyTree& sessionTree = iter->query();
            Owned<IEspSession> s = createSession();
            setSessionInfo(sessionTree.queryProp(PropSessionID), &sessionTree, authDomain.str(), port, s);
            sessions.append(*s.getLink());
        }
        resp.setSessions(sessions);
    }
    catch(IException* e)
    {
        FORWARDEXCEPTION(context, e, ECLWATCH_INTERNAL_ERROR);
    }
    return true;
}

bool CWSESPControlEx::onSessionInfo(IEspContext& context, IEspSessionInfoRequest& req, IEspSessionInfoResponse& resp)
{
    try
    {
#ifdef _USE_OPENLDAP
        CLdapSecManager* secmgr = dynamic_cast<CLdapSecManager*>(context.querySecManager());
        if(secmgr && !secmgr->isSuperUser(context.queryUser()))
            throw MakeStringException(ECLWATCH_SUPER_USER_ACCESS_DENIED, "Failed to get session information. Permission denied.");
#endif

        StringBuffer sessionID = req.getSessionID();
        if (sessionID.trim().isEmpty())
            throw MakeStringException(ECLWATCH_INVALID_INPUT, "SessionID not specified.");

        StringBuffer authDomain = req.getAuthDomain();
        if (authDomain.trim().isEmpty())
            authDomain.set("default");
        unsigned port = 8010;
        if (!req.getPort_isNull())
            port = req.getPort();

        StringBuffer sessionXPath;
        unsigned id = (unsigned) atoi(sessionID.str());
        VStringBuffer xpath("/%s/%s[@name='%s']/%s[@name='%s']/%s[@port=\"%d\"]/%s", PathSessionRoot, PathSessionProcess, espProcess.get(),
            PathSessionDomain, authDomain.str(), PathSessionApplication, port, getSessionXPath(id, sessionXPath));
        Owned<IRemoteConnection> globalLock = querySDSConnection(xpath.str(), RTM_LOCK_READ, SESSION_SDS_LOCK_TIMEOUT);
        setSessionInfo(sessionID.str(), globalLock->queryRoot(), authDomain.str(), port, &resp.updateSession());
    }
    catch(IException* e)
    {
        FORWARDEXCEPTION(context, e, ECLWATCH_INTERNAL_ERROR);
    }
    return true;
}

const char* CWSESPControlEx::getSessionXPathByIDorIP(const char* sessionID, const char* fromIP, StringBuffer& xpath)
{
    if (!isEmptyString(sessionID))
        getSessionXPath((unsigned) atoi(sessionID), xpath);
    else
    {
        getSessionXPath(0, xpath);
        xpath.appendf("[%s='%s']", PropSessionNetworkAddress, fromIP);
    }
    return xpath.str();
}

bool CWSESPControlEx::onCleanSession(IEspContext& context, IEspCleanSessionRequest& req, IEspCleanSessionResponse& resp)
{
    try
    {
#ifdef _USE_OPENLDAP
        CLdapSecManager* secmgr = dynamic_cast<CLdapSecManager*>(context.querySecManager());
        if(secmgr && !secmgr->isSuperUser(context.queryUser()))
            throw MakeStringException(ECLWATCH_SUPER_USER_ACCESS_DENIED, "Failed to clean session. Permission denied.");
#endif

        StringBuffer sessionID = req.getSessionID();
        StringBuffer fromIP = req.getFromIP();
        if ((sessionID.trim().isEmpty()) && (fromIP.trim().isEmpty()))
            throw MakeStringException(ECLWATCH_INVALID_INPUT, "SessionID or FromIP has to be specified.");

        StringBuffer authDomain = req.getAuthDomain();
        if (authDomain.trim().isEmpty())
            authDomain.set("default");
        unsigned port = 8010;
        if (!req.getPort_isNull())
            port = req.getPort();

        VStringBuffer xpath("/%s/%s[@name='%s']/%s[@name='%s']/%s[@port=\"%d\"]", PathSessionRoot, PathSessionProcess,
            espProcess.get(), PathSessionDomain, authDomain.str(), PathSessionApplication, port);
        Owned<IRemoteConnection> globalLock = querySDSConnection(xpath.str(), RTM_LOCK_WRITE, SESSION_SDS_LOCK_TIMEOUT);

        IArrayOf<IPropertyTree> toRemove;
        Owned<IPropertyTreeIterator> iter = globalLock->queryRoot()->getElements(getSessionXPathByIDorIP(sessionID.str(), fromIP.str(), xpath));
        ForEach(*iter)
            toRemove.append(*LINK(&iter->query()));
        ForEachItemIn(i, toRemove)
            globalLock->queryRoot()->removeTree(&toRemove.item(i));

        resp.setStatus(0);
        resp.setMessage("Session is cleaned.");
    }
    catch(IException* e)
    {
        FORWARDEXCEPTION(context, e, ECLWATCH_INTERNAL_ERROR);
    }
    return true;
}

bool CWSESPControlEx::onSetSessionTimeout(IEspContext& context, IEspSetSessionTimeoutRequest& req, IEspSetSessionTimeoutResponse& resp)
{
    try
    {
#ifdef _USE_OPENLDAP
        CLdapSecManager* secmgr = dynamic_cast<CLdapSecManager*>(context.querySecManager());
        if(secmgr && !secmgr->isSuperUser(context.queryUser()))
            throw MakeStringException(ECLWATCH_SUPER_USER_ACCESS_DENIED, "Failed to set session timeout. Permission denied.");
#endif

        StringBuffer sessionID = req.getSessionID();
        StringBuffer fromIP = req.getFromIP();
        if (sessionID.trim().isEmpty() && fromIP.trim().isEmpty())
            throw MakeStringException(ECLWATCH_INVALID_INPUT, "SessionID or FromIP has to be specified.");

        StringBuffer authDomain = req.getAuthDomain();
        if (authDomain.trim().isEmpty())
            authDomain.set("default");
        unsigned port = 8010;
        if (!req.getPort_isNull())
            port = req.getPort();

        int* sessionTimeoutMinutes = sessionTimeoutMinutesMap.getValue(authDomain.str());
        if (!sessionTimeoutMinutes)
            throw MakeStringException(ECLWATCH_INVALID_INPUT, "AuthDomain %s not found for port %u.", authDomain.str(), port);

        VStringBuffer xpath("/%s/%s[@name='%s']/%s[@name='%s']/%s[@port=\"%d\"]", PathSessionRoot, PathSessionProcess,
            espProcess.get(), PathSessionDomain, authDomain.str(), PathSessionApplication, port);
        Owned<IRemoteConnection> globalLock = querySDSConnection(xpath.str(), RTM_LOCK_WRITE, SESSION_SDS_LOCK_TIMEOUT);

        IArrayOf<IPropertyTree> toRemove;
        int timeoutMinutes = req.getTimeoutMinutes_isNull() ? 0 : req.getTimeoutMinutes();
        Owned<IPropertyTreeIterator> iter = globalLock->queryRoot()->getElements(getSessionXPathByIDorIP(sessionID.str(), fromIP.str(), xpath));
        ForEach(*iter)
        {
            IPropertyTree& item = iter->query();
            if (timeoutMinutes <= 0)
                toRemove.append(*LINK(&item));
            else if (*sessionTimeoutMinutes >= 0)
            {
                CDateTime timeNow;
                timeNow.setNow();
                time_t simple = timeNow.getSimple() + timeoutMinutes*60 - (*sessionTimeoutMinutes > 0 ? *sessionTimeoutMinutes*60 : ESP_SESSION_TIMEOUT);
                item.setPropInt64(PropSessionLastAccessed, simple);
                item.setPropInt(PropSessionState, CSessionState_InTimeout);
            }
        }
        ForEachItemIn(i, toRemove)
            globalLock->queryRoot()->removeTree(&toRemove.item(i));

        resp.setStatus(0);
        resp.setMessage("Session timeout is updated.");
    }
    catch(IException* e)
    {
        FORWARDEXCEPTION(context, e, ECLWATCH_INTERNAL_ERROR);
    }
    return true;
}
