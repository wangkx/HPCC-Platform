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

#ifndef _ORBIZDATAMANAGER_INCL
#define _ORBIZDATAMANAGER_INCL

#include "ws_orbiz.hpp"
//#include "hql.hpp"

class OrbizDataManager: public CInterface
{
public:
    OrbizDataManager();

    //enum  Actions {ActionSave, ActionImport, ActionCheckout, ActionCheckin, ActionRollback, ActionRename, ActionWipe, ActionDelete, ActionDeleteAttributes};

    virtual void ping() {};
    /*virtual void Commit() {}
    virtual void Rollback() {}
    virtual void LookupUser(const char* user, const char* fullname) {};*/
    virtual void AddContact(IEspContact& contact) {};
    virtual void UpdateContact(IEspContact& contact) {};
    virtual void DeleteContacts(StringArray& IDs) {};
    virtual void ListContacts(IArrayOf<IEspContact> &resp) {};
    
private:

};

#endif
