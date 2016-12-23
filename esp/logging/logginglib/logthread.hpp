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

#ifndef _LOGTHREAD_HPP__
#define _LOGTHREAD_HPP__

#include "jthread.hpp"
#include "jqueue.tpp"
#include "loggingagentbase.hpp"
#include "LogFailSafe.hpp"

interface IUpdateLogThread : extends IInterface
{
    virtual int run() = 0;
    virtual void start() = 0;
    virtual void stop() = 0;

    virtual IEspLogAgent* getLogAgent() = 0;

    virtual bool hasService(LOGServiceType service) = 0;
    virtual bool queueLog(IEspUpdateLogRequest* logRequest) = 0;
    virtual bool queueLog(IEspUpdateLogRequestWrap* logRequest) = 0;
    virtual void sendLog() = 0;
};

class CLogThread : public Thread , implements IUpdateLogThread
{
    bool stopping;
    StringAttr agentName;
    int maxLogQueueLength;
    int signalGrowingQueueAt;
    unsigned maxLogRetries;   // Max. # of attempts to send log message

    Owned<IEspLogAgent> logAgent;
    LOGServiceType services[MAXLOGSERVICES];
    SafeQueueOf<IInterface, false> logQueue;
    CriticalSection logQueueCrit;
    Semaphore       m_sem;
    Owned<IThreadPool> workerThreadPool;

    bool failSafeLogging;
    Owned<ILogFailSafe> logFailSafe;
    struct tm         m_startTime;

    unsigned serializeLogRequestContent(IEspUpdateLogRequestWrap* request, StringBuffer& logData);
    IEspUpdateLogRequestWrap* unserializeLogRequestContent(const char* logData);
    bool enqueue(IEspUpdateLogRequestWrap* logRequest);
    void writeJobQueue(IEspUpdateLogRequestWrap* jobToWrite);
    IEspUpdateLogRequestWrap* readJobQueue();

public:
    IMPLEMENT_IINTERFACE;

    CLogThread();
    CLogThread(IPropertyTree* _agentConfig, const char* _service, const char* _agentName, IEspLogAgent* _logAgent = NULL);
    virtual ~CLogThread();

    IEspLogAgent* getLogAgent() {return logAgent;};

    bool hasService(LOGServiceType service)
    {
        unsigned int i = 0;
        while (services[i] != LGSTterm)
        {
            if (services[i] == service)
                return true;
            i++;
        }
        return false;
    }
    void addService(LOGServiceType service)
    {
        unsigned i=0;
        while (services[i] != LGSTterm) i++;
        services[i] = service;
    };

    int run();
    void start();
    void stop();

    bool queueLog(IEspUpdateLogRequest* logRequest);
    bool queueLog(IEspUpdateLogRequestWrap* logRequest);
    void sendLog();
    void sendJobQueueItem(const char* GUID, IEspUpdateLogRequestWrap* logRequest);

    void checkPendingLogs(bool oneRecordOnly=false);
    void checkRollOver();
};

//the following class implements a worker thread
class CUpdateLogParam : public CInterface
{
    StringAttr GUID;
    CLogThread* parent;
    IEspUpdateLogRequestWrap* logRequest;

public:
    IMPLEMENT_IINTERFACE;

    CUpdateLogParam(CLogThread* _parent, const char* _GUID, IEspUpdateLogRequestWrap* _logRequest)
        : GUID(_GUID), logRequest(_logRequest), parent(_parent) { };

    virtual ~CUpdateLogParam() {}

    virtual void doWork()
    {
        parent->sendJobQueueItem(GUID, logRequest);
    }
};

class CUpdateLogWorker : public CInterface, implements IPooledThread
{
public:
    IMPLEMENT_IINTERFACE;

    CUpdateLogWorker()
    {
    }
    virtual ~CUpdateLogWorker()
    {
    }

    void init(void *_param)
    {
        param.setown((CUpdateLogParam*) _param);
    }
    void main()
    {
        param->doWork();
    }

    bool canReuse()
    {
        return true;
    }
    bool stop()
    {
        return true;
    }

private:
    Owned<CUpdateLogParam> param;
};

//---------------------------------------------------------------------------------------------

class CUpdateLogWorkerFactory : public CInterface, public IThreadFactory
{
public:
    IMPLEMENT_IINTERFACE;
    IPooledThread *createNew()
    {
        return new CUpdateLogWorker();
    }
};

extern LOGGINGCOMMON_API IUpdateLogThread* createUpdateLogThread(IPropertyTree* _cfg, const char* _service, const char* _agentName, IEspLogAgent* _logAgent);

#endif // _LOGTHREAD_HPP__
