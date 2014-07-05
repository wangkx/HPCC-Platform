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

#include "respool.hpp"
#include "mysql.hpp"
#include "jliball.hpp"

#ifndef _NO_MYSQL_REPOSITORY
namespace mysql
{
class CMysqlConnectionManager : public CInterface, implements esp::IResourceFactory<Connection>
{
    StringBuffer               m_dbserver;
    StringBuffer               m_dbuser;
    StringBuffer               m_dbpassword;
    StringBuffer               m_db;

public:
    IMPLEMENT_IINTERFACE;
    
    CMysqlConnectionManager(const char* server, const char* user, const char* password, const char* db)
    {
        m_dbserver.append(server);
        m_dbuser.append(user);
        if(m_dbserver.length() == 0 || m_dbuser.length() == 0)
            throw MakeStringException(-1, "mysql server or user is missing");

        m_dbpassword.append(password);
        m_db.append(db);
    }

    Connection* createResource()
    {
        return new Connection(m_dbserver.str(), m_dbuser.str(), m_dbpassword.str(), m_db.str());
    }
};

class CMysqlConnectionPool : public CInterface, implements IDBConnectionPool
{
private:
    int                        m_poolsize;
    StringBuffer               m_dbserver;
    StringBuffer               m_dbuser;
    StringBuffer               m_dbpassword;
    Owned<esp::ResourcePool<Connection> > m_connections;
public:
    IMPLEMENT_IINTERFACE;

    CMysqlConnectionPool(int poolsize, const char* server, const char* user, const char* password, const char* db)
    {
        m_poolsize = poolsize;
        m_dbserver.append(server);
        m_dbuser.append(user);
        if(m_dbserver.length() == 0 || m_dbuser.length() == 0)
            throw MakeStringException(-1, "mysql server or user is missing");

        m_dbpassword.append(password);

        m_connections.setown(new esp::ResourcePool<Connection>);
        m_connections->init(poolsize, new CMysqlConnectionManager(server, user, password, db));
    }

    Linked<Connection> getConnection()
    {
        return m_connections->get(INFINITE);
    }
};

extern "C"
{
MYSQL_API IDBConnectionPool* createMysqlConnectionPool(const char* server, const char* user, const char* password, const char* db, int poolsize)
{
    return new CMysqlConnectionPool(poolsize, server, user, password, db);
}
}

}
#endif
