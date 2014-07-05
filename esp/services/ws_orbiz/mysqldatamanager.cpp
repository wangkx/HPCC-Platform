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

#pragma warning (disable : 4355)
#pragma warning (disable : 4786)

#include "platform.h"
#include "hqlplugins.hpp"
#include "jlog.hpp"

#include "xslprocessor.hpp"

#include "ws_orbiz_esp.ipp"
#include "mysqldatamanager.hpp"

#define MYSQL40

using namespace mysql;

MySQLDataManager::MySQLDataManager(const char* server,const char* user,const char* password,const char* db, IProperties* props): OrbizDataManager(),
    uc(server, user, password, db),
    query(uc),
    //dateFormat("%Y-%m-%d %H:%i:%s"),
    m_props(props)
{
}

void MySQLDataManager::reset()
{ 
    try
    {
        query << "foo" << go();
    }
    catch(IException *e)
    {
        e->Release();
    }
    catch(...){} 
}

void MySQLDataManager::ping()
{
    uc.ping();
}

void MySQLDataManager::addContact(IEspContact& contact)
{
}

void MySQLDataManager::updateContact(IEspContact& contact)
{
}

void MySQLDataManager::deleteContacts(StringArray& IDs)
{
}

void MySQLDataManager::listContacts(IArrayOf<IEspContact>& contacts)
{
   /*query<<"select HQLLIBRARY_NAME, C.COMPONENT_NAME, "
                         " IF(C.LOCKED_BY is NULL, NULL, if(L.USERID is null,'Unknown', if(L.PERSONNAME!='',L.PERSONNAME,L.USERNAME))) as LOCKED_BY,"
                         " C.LOCKED_DT,"
                         " C.MAX_VERSION as CHECKED_IN_VER,";
            if( g_useAttTypesFlag )
                query << " C.COMP_TYPE,"; //bug 32155   
    query<<go();

    for (mysql::DataSet ds(query,false); ds; ds++)  
    { 
        Owned<IEspContact> contact=loadContact(ds);
        if(contact)
            contacts.append(*contact.getLink());
    }*/          
}


