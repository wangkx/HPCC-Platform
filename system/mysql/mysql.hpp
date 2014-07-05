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


#ifndef __MYSQL_HPP
#define __MYSQL_HPP

#ifdef _WIN32
#ifdef MYSQL_EXPORTS
#define MYSQL_API __declspec(dllexport)
#else
#define MYSQL_API __declspec(dllimport)
#endif
#else
#define MYSQL_API
#endif

#pragma warning(disable: 4786)
#pragma warning(push)
#pragma warning(disable: 4251)
#include "jliball.hpp"
#include "thirdparty.h"

#ifdef SOCKET
#undef SOCKET
#endif

#ifdef _WIN32
typedef UINT_PTR SOCKET;
#else
typedef unsigned int SOCKET;
#endif

#ifndef _NO_MYSQL_REPOSITORY
#include "mysql.h"
#include <vector>

namespace mysql
{

class MYSQL_API Connection : public CInterface
{
public:
    IMPLEMENT_IINTERFACE;

    Connection(const char* server, const char* user, const char* password, const char* db);
    ~Connection();

    operator MYSQL*();

    void    setOption(int option, const char* param);
    void    setOption(int option, int param);
    void    exec(const char* query, int len);
    void    exec(const char* query) { exec(query,strlen(query)); }

    void    begin()     { exec("begin"); }
    void    commit()    { exec("commit"); }
    void    rollback()  { exec("rollback"); }

    void    ping();

    const char* getServer()     { return server; }
    const char* getUser()       { return user; }
    const char* getPassword()   { return password; }
    const char* getDB()         { return db; }
protected:
    int     check(int res, bool except=true);

    void    validate();
    void    create();
    void    open(const char* server, const char* user, const char* password, const char* db);
    void    close();
    bool    cleanup();
private:
     bool reopen();
    MYSQL connection;
    StringAttr server, user, password, db;

    friend class Query;
};

class MYSQL_API Query
{
public:
    Query(Connection& connection, const char* query=0);
    virtual ~Query();

    void exec();

    int affectedRows();
    int lastID();
    bool hasResults();
    const char* preview() { return buf.str(); }

    template<typename T> void append(const T& t) { buf.append(t); }
    void appendQuoted(const char* s, int length, const char* d);
    static MemoryBuffer& escaped(MemoryBuffer& from, MemoryBuffer& to);
    static StringBuffer& escaped(StringBuffer& from);

protected:
    virtual void exec(unsigned len);
    Connection& connection;
    StringBuffer buf;
    friend class DataSet;
};

inline Query& operator<<(Query& q, const char* s) { q.append(s); return q; }
inline Query& operator<<(Query& q, const __int64 i) { q.append(i); return q; }

struct go
{
};
inline Query& operator<<(Query& query, const go&) { query.exec(); return query; }


struct quoted
{
    explicit quoted(const char* _s, const char* _d=""): s(_s), length(-1), d(_d) {}
    quoted(const char* _s, int _length, const char* _d=""): s(_s), length(_length), d(_d) {}
    const char* s;
    int length;
    const char* d;
};

inline Query& operator<<(Query& q, const quoted& s) { q.appendQuoted(s.s, s.length, s.d); return q; }

template<typename T> struct nonzero
{
    explicit nonzero(const T& t, const char* _d="null"): data(t), d(_d) {}
    const T& data;
    const char* d;
};

template<typename T> inline Query& operator<<(Query& q, const nonzero<T>& t) 
{   
    if(t.data)
        q<<t.data;
    else
        q<<t.d;
    return q;
}

template<typename T> inline nonzero<T> value(const T& t) { return nonzero<T>(t,"null"); }                                                                           

inline quoted value(const char* s) { return quoted(s,"null"); } 
inline quoted value(const char* s, int length) { return quoted(s,length,"null"); } 

struct utf8
{
    explicit utf8(const char* _s): s(_s) {}
    const char* s;
};

inline Query& operator<<(Query& q, const utf8& s) 
{ 
    if(s.s)
        q.append("_utf8");

    q.appendQuoted(s.s, strlen(s.s), "null");
    return q; 
}

//inline escaped value(StringBuffer& in, StringBuffer& out){ return escaped(in,out); }
/*
void escape_string(StringBuffer& from, StringBuffer& to)
{
    if(from.length() == 0)
        return;
    char* b=(char*)malloc(2*from.length() +1);
    mysql_escape_string(b, from.toCharArray(), from.length());
    from.append(b);
    free(b);

}
*/
struct format
{
    explicit format(const char* s,...)
    { 
        va_list marker;
        va_start(marker, s);     
        buf.valist_appendf(s, marker); 
        va_end(marker);              
    }
    StringBuffer buf;
};
inline Query& operator<<(Query& q, const format& s) { q.append(s.buf); return q; }


class MYSQL_API DataItem 
{
public:
    DataItem();

    operator const char*()
    {
        return data;
    }

    operator int()
    {
        return data ? atoi(data) : 0;
    }

    const char* getName()
    {
        return field->name;
    }

    const size_t size()
    {
        return field->length;
    }

    void* get() 
    {
        return data; 
    }

    //void getAsString(StringBuffer& s);
    
    bool isUnsigned() { return (field->flags & UNSIGNED_FLAG)!=0; }
    enum_field_types getType() { return field->type; }

private:
    DataItem(MYSQL_FIELD* field, char* data);
  
    MYSQL_FIELD* field;
    char* data;
    friend class DataSet;
};


class MYSQL_API DataSet
{
public:
    DataSet(): res(0), row(0) {}
    DataSet(Query& query,bool bulk=false);
    ~DataSet();
    operator bool();
    bool operator ++(int);

    DataItem operator [](int index);
    DataItem operator [](const char* name);
    bool operator = (Query& query);
    int find(const char* name);
    int numRows();
    int numColumns();

private:
    void init(Query& query,bool bulk=false);
    bool fetch();

    MYSQL_RES* res;
    MYSQL_ROW row;
};


class MYSQL_API Value
{
public:
    Value(Query& query, int column=0);
    operator const char*() const { return buf.str(); }
    operator int() const { return atoi(buf.str()); }

private:
    StringBuffer buf;
};

class MYSQL_API List
{
public:
    List(Query& query, int column=0);
    int length() { return count; }
    operator const char*() const { return buf.str(); }

private:
    StringBuffer buf;
    int count;
};

class MYSQL_API Transaction: public Query
{
public:
    Transaction(Connection& connection,const char* query=0);
    ~Transaction();

    void commit();
    void rollback();


private:
    int status;
};


class MYSQL_API MultiInsert: public Query
{
public:
    MultiInsert(Connection& connection,const char* query=0);

    void mark();
    bool flush(bool force=false);
private:
    int mark_;
};



interface MYSQL_API IDBConnectionPool : implements IInterface
{
    virtual Linked<Connection> getConnection() = 0;
};

#define DEFAULT_MYSQL_DB_POOL_SIZE 5

extern "C"
{
MYSQL_API IDBConnectionPool* createMysqlConnectionPool(const char* server, const char* user, const char* password, const char* db, int poolsize=DEFAULT_MYSQL_DB_POOL_SIZE);
}

}
#endif
#pragma warning(pop)
#endif
