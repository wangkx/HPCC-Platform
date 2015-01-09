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

#pragma warning (disable : 4786)

#ifndef _MYSQLLOGAGENT_HPP__
#define _MYSQLLOGAGENT_HPP__

#include "mysql.hpp"
#include "loggingcommon.hpp"
#include "loggingagentbase.hpp"
#include "dbfieldmap.hpp"

#ifdef WIN32
    #ifdef MYSQLLOGAGENT_EXPORTS
        #define MYSQLLOGAGENT_API __declspec(dllexport)
    #else
        #define MYSQLLOGAGENT_API __declspec(dllimport)
    #endif
#else
    #define MYSQLLOGAGENT_API
#endif


class CMySQLDbConnection : public CInterface
{
    mysql::Connection  mysqlConn;
    mysql::MultiInsert query;

public:
    IMPLEMENT_IINTERFACE;

    CMySQLDbConnection(const char* server,const char* user, const char* password, const char* db)
        : mysqlConn(server, user, password, db), query(mysqlConn) {};
    virtual ~CMySQLDbConnection() {};

    void execute(const char* query);
    void executeAndGetReturn(const char* query, const char* returnFieldName, StringBuffer& returnVal);
};

class CMySQLLogAgent : public CInterface, implements IEspLogAgent
{
    StringBuffer agentName,  agentType;
    StringBuffer logDB, logDBTable;
    Owned<DBFieldMap> fieldMap;
    Owned<CMySQLDbConnection> dbConnection;

    StringBuffer transactionSeed;
    unsigned uniqueTransactionIDCount;
    unsigned maxTriesGTS;
    CriticalSection uniqueIDCrit;

    bool buildInsertStatement(const char* logRequest, MemoryBuffer& sqlBuffer);
    void appendFieldInfo(const char* name, const char* value, StringBuffer& fields, StringBuffer& values);
    void appendFieldInfo(const char* formatField, MemoryBuffer& formatValue, StringBuffer& fields, StringBuffer& values);
    void appendMissingFields(BoolHash& HandledFields, StringBuffer& fields, StringBuffer& values);
    void generateTransactionSeed(StringBuffer& seedId);
    void getUniqueTransactionID(StringBuffer& uniqueID);

public:
    IMPLEMENT_IINTERFACE;

    CMySQLLogAgent() {};
    virtual ~CMySQLLogAgent() {};

    virtual bool init(const char* name, const char* type, IPropertyTree* cfg, const char* process);
    virtual bool getTransactionSeed(IEspGetTransactionSeedRequest& req, IEspGetTransactionSeedResponse& resp);
    virtual bool updateLog(IEspUpdateLogRequestWrap& req, IEspUpdateLogResponse& resp);
    virtual void filterLogContent(IEspUpdateLogRequestWrap* req);
};

#endif //_SQLLOGAGENT_HPP__
