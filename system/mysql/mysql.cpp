/*##############################################################################

    Copyright (C) <2010>  <LexisNexis Risk Data Management Inc.>

    All rights reserved. This program is NOT PRESENTLY free software: you can NOT redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
############################################################################## */

#include <algorithm>
#include <memory.h>
#include <string.h>
#include <time.h>

#include "mysql.hpp"

#include "jexcept.hpp"
#include "jmisc.hpp"
#include "jstring.hpp"
#include "jlog.hpp"


#ifndef _NO_MYSQL_REPOSITORY
using namespace mysql;
const unsigned RECONNECT = 3;
const unsigned RECONNECTSLEEP = 500;
const unsigned SERVER_NO_RESPONSE = 2006;

Connection::Connection(const char* _server, const char* _user, const char* _password, const char* _db): 
server(_server), user(_user), password(_password), db(_db)
{
    create();

    //move it out
    open(server.get(), user.get(), password.get(), db.get());
}

Connection::~Connection()
{
    close();
}
 
Connection::operator MYSQL*()
{
    return &connection;
}

int Connection::check(int res, bool except)
{
    if(res)
    {
        StringBuffer err;

        switch(res=mysql_errno(&connection))
        {
        case 0:
            err.append("MySQL library error");
            break;

        // need a better way to distinguish warnings
        case 1196:
            return res;

        default:                
            err.appendf("MySQL Error %d: %s", res, mysql_error(&connection));
            break;
        }

        DBGLOG("%s",err.str());
        if(except && res!=SERVER_NO_RESPONSE)
            throw MakeStringExceptionDirect(999, err.str());
    }

    return res;
}

void Connection::create()
{
    check(mysql_init(&connection) != &connection);
}

void Connection::setOption(int option, const char* param)
{
    int err = check(mysql_options(&connection, (mysql_option)option, param));   
    if (err == SERVER_NO_RESPONSE)
    {
        if (reopen())
            check(mysql_options(&connection, (mysql_option)option, param));
    }
}

void Connection::setOption(int option, int param)
{
    setOption(option,(const char*)&param);
}

void Connection::open(const char* server, const char* user, const char* password, const char* db)
{
    int err = check(mysql_real_connect(&connection, server, user, password, db, 0, 0, CLIENT_MULTI_STATEMENTS | CLIENT_MULTI_RESULTS) != &connection);
    if (err == SERVER_NO_RESPONSE)
    {
        if (reopen())
            check(mysql_real_connect(&connection, server, user, password, db, 0, 0, CLIENT_MULTI_STATEMENTS | CLIENT_MULTI_RESULTS) != &connection);
    }
}

void Connection::close()
{
    mysql_close(&connection); 
}

void Connection::validate()
{
}

void Connection::ping()
{
   int err = check(mysql_ping(&connection),false); 
    if (err == SERVER_NO_RESPONSE)
    {
        if (reopen())
            check(mysql_ping(&connection),false);
    }
}

bool Connection::reopen()
{
    bool success = true;
    unsigned retry = 0;
    while (retry < RECONNECT)
    {
        mysql_close(&connection); 
       check(mysql_init(&connection) != &connection, false);
        int err = check(mysql_real_connect(&connection, server, user, password, db, 0, 0, CLIENT_MULTI_STATEMENTS | CLIENT_MULTI_RESULTS) != &connection, false);
        if (!err)
        {
            retry = RECONNECT+1; //set the retry a big value so that no next retry 
        }
        else
        {
            sleep(RECONNECTSLEEP);
            retry++;
        }
    }

    if (retry == RECONNECT)
    {
      DBGLOG("MySQL Error failed in Reopen");
        success = false;
    }
    else
    {
      DBGLOG("MySQL connection successfully reopened.");
    }

    return success;
}

bool Connection::cleanup()
{
    MYSQL_RES *res = mysql_store_result(&connection); 
    if(res)
    {
        mysql_free_result(res);
    }
    else if(mysql_error(&connection))
    {
        return false;
    }
    return true;
}

void Connection::exec(const char* query, int len)
{   
   int err = check(mysql_real_query(&connection, query, len));
    if (err == SERVER_NO_RESPONSE)
    {
        if (reopen())
            check(mysql_real_query(&connection, query, len));
    }
}


Query::Query(Connection& _connection, const char* _query): connection(_connection)
{
    connection.ping();
    if(_query)
        buf.append(_query);
}

Query::~Query()
{
    connection.cleanup();
}

void Query::exec() 
{ 
    exec(buf.length()); 
    buf.clear();
}

   
void Query::exec(unsigned length)
{
//    connection.check(!connection.cleanup());
    connection.cleanup();
//    PrintLogDebug("Query: %.*s\n", length, buf.str());
    try
    {
        connection.exec(buf.str(), length);
    }
    catch(...)
    {
        buf.clear();
        throw;
    }
}


int Query::affectedRows()
{
    return mysql_affected_rows(connection); 
}

int Query::lastID()
{
    return mysql_insert_id(connection);
}

bool Query::hasResults()
{
    return mysql_field_count(connection)>0;
}

void Query::appendQuoted(const char* s, int length, const char* d)
{
    if(s)
    {
        unsigned n=length>=0 ? length : strlen(s);
        char* b=(char*)malloc(2*n+1);
        mysql_real_escape_string(connection, b, s, n);
        *this<<"'"<<b<<"'";
        free(b);
    }
    else
        *this<<d;
}

MemoryBuffer& Query::escaped(MemoryBuffer& from, MemoryBuffer& to)
{
    if(from.length())
    {
        
        char* b=(char*)malloc(2*from.length());
        int i= mysql_escape_string(b, from.toByteArray(), from.length()-1);
        to.append(strlen(b),b);
        //*this<<"'"<<b<<"'";
        free(b);
    }
    return to;
}

StringBuffer& Query::escaped(StringBuffer& from)
{
    if(from.length())
    {
        
        char* b=(char*)malloc(2*from.length());
        int i= mysql_escape_string(b, from.str(), from.length());
        from.clear().append(strlen(b),b);
        free(b);
    }
    return from;
}

DataItem::DataItem(): data(0)
{
}

DataItem::DataItem(MYSQL_FIELD* _field, char* _data): field(_field), data(_data)
{
}

#if 0
void DataItem::getAsString(StringBuffer& s)
{
    enum_field_types type = getType();
    switch(type)
    {
    case FIELD_TYPE_DECIMAL: s.append(atof(data)); break;
    case FIELD_TYPE_TINY: s.append((int)(char)data); break;
    case FIELD_TYPE_SHORT: s.append((short)data); break;
    case FIELD_TYPE_VAR_STRING: s.append(data); break;
    case FIELD_TYPE_DATE: s.append(data); break;
    case FIELD_TYPE_STRING: s.append((char)data[0]); break;
    case FIELD_TYPE_LONG: s.appendlong((long)data); break;
    case FIELD_TYPE_LONGLONG: s.append((__int64)data); break;
    case FIELD_TYPE_TIMESTAMP: s.append((__int64)data); break; // todo: parse time stamp

    // unsupported
    case FIELD_TYPE_FLOAT:
    case FIELD_TYPE_DOUBLE:
    case FIELD_TYPE_NULL:
    case FIELD_TYPE_INT24:
    case FIELD_TYPE_TIME:
    case FIELD_TYPE_DATETIME:
    case FIELD_TYPE_YEAR:
    case FIELD_TYPE_NEWDATE:
    case FIELD_TYPE_ENUM:
    case FIELD_TYPE_SET:

    case FIELD_TYPE_TINY_BLOB:
    case FIELD_TYPE_MEDIUM_BLOB:
    case FIELD_TYPE_LONG_BLOB:
    case FIELD_TYPE_BLOB:
    
    case FIELD_TYPE_GEOMETRY:
    default:  assert(!"Unsupported types");
        break;
    }
}
#endif
    
DataSet::DataSet(Query& _query,bool bulk): row(0)
{
    init(_query,bulk);
}

bool DataSet::operator =(Query& _query)
{
    init(_query,true);
    return *this;
}

DataSet::~DataSet()
{
    if(res)
    {
        while(mysql_fetch_row(res));
        mysql_free_result(res);
    }
}

DataSet::operator bool()
{
    return row!=0;
}

bool DataSet::operator ++(int)
{
    row = mysql_fetch_row(res);
    return row!=0;
}

DataItem DataSet::operator [](int index)
{
    return DataItem(mysql_fetch_field_direct(res,index), row[index]);
}

DataItem DataSet::operator [](const char* name)
{
    int index=find(name);
    return index==0 ? DataItem(NULL,NULL) : (*this)[index-1];
}

void DataSet::init(Query& _query,bool bulk)
{
    row = 0;
    res = bulk ? mysql_store_result(_query.connection) : mysql_use_result(_query.connection);
    if(res)
    {
        (*this)++;
    }
}

struct fmt_find
{
    fmt_find(const char* _name): name(_name) 
        {}
    bool operator ()(const MYSQL_FIELD& fmt) 
        { return stricmp(fmt.name,name)==0; }
    const char* name;
};

int DataSet::find(const char* name)
{
    int pos = std::find_if(res->fields, res->fields+res->field_count, fmt_find(name)) - res->fields;
    return pos < res->field_count ? pos+1 : 0;
}

int DataSet::numRows()
{
    return mysql_num_rows(res);
}

int DataSet::numColumns()
{
    return mysql_num_fields(res);
}


List::List(Query& query, int column): count(0)
{
    bool first=true;
    DataSet ds(query);
    if(ds)
        for(;ds;ds++)
        {
            if(!first)
                buf.append(", ");
            else
                first=false;
            buf.append((const char*)ds[column]);
            count++;
        }
    else
    {
        buf.append("null");
    }
}

Value::Value(Query& query, int column)
{
    DataSet ds(query);
    if(ds)
    {
        buf.set((const char*)ds[column]);
    }
    else
        buf.set("");
}


Transaction::Transaction(Connection& _connection,const char* query): Query(_connection,query), status(-1)
{
    Query(connection,"begin").exec();
}

Transaction::~Transaction()
{
    rollback();
}

void Transaction::commit()
{ 
    if(status<0)
    {
        Query(connection,"commit").exec();
        status=1;
    }
}

void Transaction::rollback()
{
    if(status<0)
    {  
        Query(connection,"rollback").exec(); //from destructor??
        status=0;
    }
}



MultiInsert::MultiInsert(Connection& _connection,const char* query): Query(_connection,query), mark_(0)
{
    if(query)
        mark();
}

void MultiInsert::mark()
{
    mark_=buf.length();
}


bool MultiInsert::flush(bool force)
{
    bool res=!(buf.length()>mark_);

    if(buf.length()>512*1024 || (!res && force))
    {
        exec(buf.length());
        buf.remove(mark_,buf.length()-mark_);
        res=true;
    }
    return res;
}


#endif
