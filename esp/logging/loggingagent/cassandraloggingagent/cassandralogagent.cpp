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
#include "esploggingservice_esp.ipp"
#include "cassandralogagent.hpp"

//#ifdef _NO_MYSQL
//#define MYSQL40

using namespace cassandraembed;

const int DefaultMaxTriesGTS = -1;
const char* const PropCassandra = "MySQL"; //Replace "MySQL" after configuration code is ready
const char* const PropServer = "@server";
const char* const PropDBName = "@dbName";
const char* const PropTableName = "@dbTableName";
const char* const PropDBUserID = "@dbUser";
const char* const PropDBPassWord = "@dbPassWord";
const char* const PropMaxTriesGTS = "MaxTriesGTS";
const char* const SQL_INSERT_ERROR_FILE_NAME = "insert.err";
const char* const SQL_INSERT_DUPLICATE_FILE_NAME = "duplicate.err";

static void setCassandraLogAgentOption(StringArray& opts, const char *opt, const char *val)
{
    if (opt && val)
    {
        VStringBuffer optstr("%s=%s", opt, val);
        opts.append(optstr);
    }
}

static const CassValue *getSingleResult(const CassResult *result)
{
    const CassRow *row = cass_result_first_row(result);
    if (row)
        return cass_row_get_column(row, 0);
    else
        return NULL;
}

void testDBConnection(CassandraClusterSession* cluster, const char *st, const char *stWait)
{
    CassandraStatement statement(cluster->prepareStatement(st, true));
    CassandraFuture future(cass_session_execute(cluster->querySession(), statement));
    future.wait(stWait);
    CassandraResult result(cass_future_get_result(future));
    unsigned count = getUnsignedResult(NULL, getSingleResult(result));
    DBGLOG("Test<%d>", count);
    //VStringBuffer selectSt("select get_transaction_id('%s') as next_id", logDBTable.str());
    ///dbConnection->executeAndGetReturn(selectSt.str(),"next_id", transactionSeed);
}

bool CCassandraLogAgent::init(const char* name, const char* type, IPropertyTree* cfg, const char* process)
{
    if (!name || !*name || !type || !*type)
        throw MakeStringException(-1,"Name or type not specified for MySQLLogAgent");

    if (!cfg)
        throw MakeStringException(-1,"Unable to find configuration for log agent %s:%s", name, type);

    IPropertyTree* mysql = cfg->queryBranch(PropCassandra);
    if(!mysql)
        throw MakeStringException(-1,"Unable to find MySQL settings for log agent %s:%s", name, type);

    dbTraceLevel = 5; //TODO: read from config
    const char* logDBServer = mysql->queryProp(PropServer);
    logDB.set(mysql->queryProp(PropDBName));
    logDBTable.set(mysql->queryProp(PropTableName));
    if (!logDBServer || !*logDBServer || !logDB.length() || !logDBTable.length())
        throw MakeStringException(-1,"Unable to find database settings for log agent %s:%s", name, type);

    StringBuffer dbPassword;
    const char* dbUserID = mysql->queryProp(PropDBUserID);
    const char* encodedPassword = mysql->queryProp(PropDBPassWord);
    if(encodedPassword && *encodedPassword)
        decrypt(dbPassword, encodedPassword);
    //if (!dbUserID || !*dbUserID || !dbPassword.length())
    //    throw MakeStringException(-1,"Unable to find database user settings for log agent %s:%s", name, type);

    StringArray opts;
    setCassandraLogAgentOption(opts, "contact_points", logDBServer);
    setCassandraLogAgentOption(opts, "keyspace", logDBTable.str());
    setCassandraLogAgentOption(opts, "user", dbUserID);
    setCassandraLogAgentOption(opts, "password", dbPassword);

    VStringBuffer xPath("//Fieldmap[@name=\"%s\"]", logDBTable.str());
    IPropertyTree* fieldMapping = cfg->queryBranch(xPath.str());
    if(!fieldMapping)
        throw MakeStringException(-1,"Unable to find field mapping settings for log agent %s:%s", name, type);

    fieldMap.setown(new DBFieldMap());
    fieldMap->loadMappings(*fieldMapping);

    agentName.set(name);
    agentType.set(type);
    uniqueTransactionIDCount = 0;
    maxTriesGTS = cfg->getPropInt(PropMaxTriesGTS, DefaultMaxTriesGTS);

    cluster.setown(new CassandraClusterSession(cass_cluster_new()));
    if (!cluster)
        throw MakeStringException(-1,"Unable to create cassandra cluster session");
    cluster->connect();
    cluster->setOptions(opts);

    VStringBuffer buf("SELECT COUNT(*) FROM %s", logDBTable.str());
    testDBConnection(cluster, buf.str(), "select count(*)");
    ///dbConnection.setown(new CMySQLDbConnection(logDBServer, dbUserID, dbPassword.str(), logDB.str()));
    generateTransactionSeed(transactionSeed);
    return true;
}

void CCassandraLogAgent::generateTransactionSeed(StringBuffer& transactionSeed)
{
    VStringBuffer selectSt("select get_transaction_id('%s') as next_id", logDBTable.str());
    ///dbConnection->executeAndGetReturn(selectSt.str(),"next_id", transactionSeed);
}

bool CCassandraLogAgent::getTransactionSeed(IEspGetTransactionSeedRequest& req, IEspGetTransactionSeedResponse& resp)
{
    bool bRet = false;
    unsigned retry = 1;
    while (1)
    {
        try
        {
            StringBuffer logSeed;
            generateTransactionSeed(logSeed);
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
            ERRLOG("%s -- try %d", errorMessage.str(), retry);
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

void CCassandraLogAgent::filterLogContent(IEspUpdateLogRequestWrap* req)
{
    return; //No filter in CCassandraLogAgent for now
}

bool CCassandraLogAgent::updateLog(IEspUpdateLogRequestWrap& req, IEspUpdateLogResponse& resp)
{
    bool ret = false;
    try
    {
        const char* updateLogRequest = req.getUpdateLogRequest();
        if (!updateLogRequest || !*updateLogRequest)
            throw MakeStringException(EspLoggingErrors::UpdateLogFailed, "Failed to read log request.");

        MemoryBuffer sqlStatement;
        if(!buildInsertStatement(updateLogRequest, sqlStatement))
            throw MakeStringException(EspLoggingErrors::UpdateLogFailed, "Failed in creating SQL statement.");

        ESPLOG(LogMax,"SQL: %s\n", sqlStatement.toByteArray());

        ///dbConnection->execute(sqlStatement.toByteArray());
        resp.setStatusCode(0);
        ret = true;
    }
    catch (IException* e)
    {
        StringBuffer errorStr, errorMessage;
        errorMessage.append("Failed to update log: error code ").append(e->errorCode()).append(", error message ").append(e->errorMessage(errorStr));
        ERRLOG("%s", errorMessage.str());
        e->Release();
        resp.setStatusCode(-1);
        resp.setStatusMessage(errorMessage.str());
    }
    return ret;
}

bool CCassandraLogAgent::buildInsertStatement(const char* logRequest, MemoryBuffer& sqlStatement)
{
    StringBuffer requestBuf, fields, values;
    requestBuf.append("<LogRequest>").append(logRequest).append("</LogRequest>");
    Owned<IPropertyTree> pTree = createPTreeFromXMLString(requestBuf.str());
    if (!pTree)
        throw MakeStringException(EspLoggingErrors::UpdateLogFailed, "Failed to read log request.");

    BoolHash handledFields;
    StringArray& mapNames = fieldMap->getMapNames();
    ForEachItemIn(x, mapNames) //Go through data items to be logged
    {
        const char* path = mapNames.item(x);
        if (!path || !*path)
            continue;

        StringBuffer colName, colValue;
        fieldMap->mappingField(path, colName);
        if(!colName.length() || handledFields.getValue(colName.str()))
            continue;

        if (path[strlen(path) - 1] == ']')
        {//Attr filter
            const char* pTr = strrchr(path, '[');
            if (!pTr)
                continue;

            StringBuffer path0, attr;
            path0.set(path);
            path0.setLength(pTr - path);
            attr.set(pTr+1);
            attr.setLength(attr.length() - 1);
            Owned<IPropertyTreeIterator> itr = pTree->getElements(path0.str());
            ForEach(*itr)
            {
                IPropertyTree& ppTree = itr->query();
                colValue.set(ppTree.queryProp(attr.str()));
                if (colValue.length())
                {
                    appendFieldInfo(colName.str(), colValue.str(), fields, values);
                    handledFields.setValue(colName.str(), true);
                    break;
                }
            }
            continue;
        }

        Owned<IPropertyTreeIterator> itr = pTree->getElements(path);
        ForEach(*itr)
        {
            IPropertyTree& ppTree = itr->query();
            if (ppTree.hasChildren()) //This is a tree branch.
                toXML(&ppTree, colValue);
            else
                colValue.set(ppTree.queryProp(NULL));

            if (colValue.length())
            {
                appendFieldInfo(colName.str(), colValue.str(), fields, values);
                handledFields.setValue(colName.str(), true);
                break;
            }
        }
    }

    //add any default fields that may be required but have not been passed in.
    appendMissingFields(handledFields, fields, values);

    VStringBuffer sqlBuffer("insert into %s.%s %s %s\n", logDB.str(), logDBTable.str(), fields.str(), values.str());
    sqlStatement.append(sqlBuffer.str());

    return true;
}

void CCassandraLogAgent::appendFieldInfo(const char* name, const char* value, StringBuffer& fields, StringBuffer& values)
{
    StringBuffer formattedField;
    MemoryBuffer formattedValue;
    if (fieldMap && fieldMap->formatField(name, value, formattedField, formattedValue))
        appendFieldInfo(formattedField.str(), formattedValue, fields, values);
}

void CCassandraLogAgent::appendFieldInfo(const char* formatField, MemoryBuffer& formatValue, StringBuffer& fields, StringBuffer& values)
{
    if(values.length() != 0)
        values.append(',');
    values.append(formatValue.length(),formatValue.toByteArray());

    if(fields.length() != 0)
        fields.append(',');
    fields.appendf( " %s " , formatField);
}

void CCassandraLogAgent::appendMissingFields(BoolHash& handledFields, StringBuffer& fields, StringBuffer& values)
{
    if (!handledFields.getValue("transaction_id"))
    {
        StringBuffer transactionID;
        getUniqueTransactionID(transactionID);
        appendFieldInfo("transaction_id", transactionID.str(), fields, values);
    }
    if (!handledFields.getValue("record_count"))
    {
        appendFieldInfo("record_count", "-1", fields, values);
    }

    fields.insert(0,"(");
    fields.append(", date_added)");
    values.insert(0,"values (");
    values.append(", now())");

    return;
}

void CCassandraLogAgent::getUniqueTransactionID(StringBuffer& uniqueID)
{
    CriticalBlock b(uniqueIDCrit);
    uniqueID.setf("%s-%u", transactionSeed.str(), ++uniqueTransactionIDCount);
}
/*
//insert into db_name.table_name (col1, col2, ...) values (val1, val2, ...)
void CMySQLDbConnection::execute(const char* sqlQuery)
{
    if(!sqlQuery || !*sqlQuery)
        throw MakeStringException(MySQLErrors::EmptySQLQuery,"Cannot execute a blank query");

    try
    {
        query << sqlQuery << mysql::go();
    }
    catch(IException* e)
    {
        StringBuffer errorMessage;
        VStringBuffer msg("CMySQLDbConnection::execute() exception %d: %s. SQL:%s", e->errorCode(), e->errorMessage(errorMessage).str(), sqlQuery);
        e->Release();
        throw MakeStringException(MySQLErrors::ExecuteStatementFailed, "%s", msg.str());
    }
}

//Used by generateTransactionSeed(), etc
void CMySQLDbConnection::executeAndGetReturn(const char* sqlQuery,const char* returnFieldName, StringBuffer& returnVal)
{
    if(!sqlQuery || !*sqlQuery)
        throw MakeStringException(MySQLErrors::EmptySQLQuery,"Cannot execute a blank query");
    if(!returnFieldName || !*returnFieldName)
        throw MakeStringException(MySQLErrors::ExecuteStatementFailed,"Field name not specified");

    try
    {
        query << sqlQuery << mysql::go();
        mysql::DataSet ds(query,false);

        if(!ds)
            throw MakeStringExceptionDirect(MySQLErrors::ExecuteStatementFailed, "No dataset returned");

        returnVal.appendf("%d", (int)ds[returnFieldName]);
    }
    catch(IException* e)
    {
        StringBuffer errorMessage;
        VStringBuffer msg("CMySQLDbConnection::execute() exception %d: %s. SQL:%s", e->errorCode(), e->errorMessage(errorMessage).str(), sqlQuery);
        e->Release();
        throw MakeStringException(MySQLErrors::ExecuteStatementFailed, "%s", msg.str());
    }
}*/

extern "C"
{
CASSABDRALOGAGENT_API IEspLogAgent* newLoggingAgent()
{
    return new CCassandraLogAgent();
}
}

//#endif
