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

#ifndef _MYSQLDATAMANAGER_INCL
#define _MYSQLDATAMANAGER_INCL

#include "orbizdatamanager.hpp"

#include "mysql.hpp"

class MySQLDataManager : public OrbizDataManager
{
public:    
    MySQLDataManager(const char* server,const char* user,const char* password,const char* db, IProperties* props = NULL);

    //IProperties* getProperties();
    
private:
    /*void checkDBFeatures();
    virtual void ping();
    virtual void Commit() { uc.commit(); }
    virtual void Rollback() { uc.rollback(); }

    virtual void LookupUser(const char* user, const char* fullname);*/

    virtual void addContact(IEspContact& contact);
    virtual void updateContact(IEspContact& contact);
    virtual void deleteContacts(StringArray& IDs);
    virtual void listContacts(IArrayOf<IEspContact>& contacts);

    
//    void RenameModules();
    void ping();
    void reset();
//    void CheckErrors();
//    void CheckComponents(IArrayOf<IEspECLAttribute> &results);

//    StringBuffer& toLocalDateTime(const char* date, StringBuffer& dateb);
//    StringBuffer& toGMTDateTime(const char* date, StringBuffer& dateb);

    mysql::Connection  uc;
    mysql::MultiInsert query;

//    const char* dateFormat;

    IProperties* m_props;
/*
    bool g_useAttTypesFlag;

    int m_db_version;
    bool m_new_join;*/

};

#endif
