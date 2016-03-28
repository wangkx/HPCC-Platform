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

#pragma warning (disable : 4786)

#ifndef _CASSABDRALOGAGENT_HPP__
#define _CASSABDRALOGAGENT_HPP__

#include "jmisc.hpp"
#include "eclhelper.hpp"
#include "cassandra.h"
#include "cassandraembed.hpp"
#include "loggingcommon.hpp"
#include "loggingagentbase.hpp"
#include "dbfieldmap.hpp"

using namespace cassandraembed;

#ifdef WIN32
    #ifdef CASSABDRALOGAGENT_EXPORTS
        #define CASSABDRALOGAGENT_API __declspec(dllexport)
    #else
        #define CASSABDRALOGAGENT_API __declspec(dllimport)
    #endif
#else
    #define CASSABDRALOGAGENT_API
#endif
/*
namespace MySQLErrors
{
    const unsigned int Base = EspCoreErrors::Base+1800;       //2800 -- pls don't change this....the web is coded to expect the following codes
    const unsigned int PrepedStmtAllocHandleFailed  = Base+0; //"Can't allocate a handle for the prepared statement."
    const unsigned int PrepedStmtMissing            = Base+1; //"The prepared statement is NULL."
    const unsigned int PrepedStmtBindParamsFailed   = Base+2; //"Binding of the parameters to the prepared statement failed."
    const unsigned int PrepedStmtBindColumnsFailed  = Base+3; //"Binding of the columns to the prepared statement failed."
    const unsigned int PrepedStmtExecuteFailed      = Base+4; //"Executing the prepared statement failed."
    const unsigned int PrepedStmtStoreResultFailed  = Base+5; //"Storing fetched records on the client for the prepared statement failed."
    const unsigned int MismatchingNumOfParams       = Base+6; //"The number of params to be set is different from the number of params in the prepared statement."
    const unsigned int ConnectionTimedOut           = Base+7; //The connection to mysql timed out
    const unsigned int ExecuteStatementFailed       = Base+8; //Failed to Get Cumulative Timer
    const unsigned int EmptySQLQuery                = Base+9; //Empty SQL query cannot be executed
}

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
*/
class CCassandraLogAgent : public CInterface, implements IEspLogAgent
{
    StringBuffer agentName,  agentType;
    StringBuffer logDB, logDBTable;
    Owned<DBFieldMap> fieldMap;
    //Owned<CMySQLDbConnection> dbConnection;
    Owned<CassandraClusterSession> cluster;
    unsigned dbTraceLevel;

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

    CCassandraLogAgent() {};
    virtual ~CCassandraLogAgent()
    {
        if (!cluster)
            cluster->disconnect();
    };

    virtual bool init(const char* name, const char* type, IPropertyTree* cfg, const char* process);
    virtual bool getTransactionSeed(IEspGetTransactionSeedRequest& req, IEspGetTransactionSeedResponse& resp);
    virtual bool updateLog(IEspUpdateLogRequestWrap& req, IEspUpdateLogResponse& resp);
    virtual void filterLogContent(IEspUpdateLogRequestWrap* req);
};

#endif //_CASSABDRALOGAGENT_HPP__
