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

#ifndef _WsOrbiz_HPP__
#define _WsOrbiz_HPP__

#include "jlib.hpp"
#include "jdebug.hpp"
#include "jthread.hpp"
#include "jsocket.hpp"
#include "jcomp.hpp"
#include "jptree.hpp"

#include <vector>

#include "jwrapper.hpp"
#include "respool.hpp"
using namespace esp;

#include "ws_orbiz_esp.ipp"

#include "orbizdatamanager.hpp"

#define REPOSITORY_CONNECTION_WAIT_SECONDS 90

class CWsOrbizEx : public CWsOrbiz
{
public:
    IMPLEMENT_IINTERFACE;
    CWsOrbizEx();

    virtual void init(IPropertyTree *cfg, const char *process, const char *service);
    bool onAddContact(IEspContext &context, IEspAddContactRequest &req, IEspAddContactResponse &resp);
    bool onUpdateContact(IEspContext &context, IEspUpdateContactRequest &req, IEspUpdateContactResponse &resp);
    bool onDeleteContacts(IEspContext &context, IEspDeleteContactsRequest &req, IEspDeleteContactsResponse &resp);
    bool onListContacts(IEspContext &context, IEspListContactsRequest &req, IEspListContactsResponse &resp);

private:
    Linked<OrbizDataManager> getRepository()
    { 
        Linked<OrbizDataManager> conn=connections.get(m_waittimeout*1000);
        conn->ping(); 
        return conn; 
    }  
 /*
    Owned<ISecManager> secManager;
    typedef std::map<std::string,Linked<ISecUser> > UserMap;
    UserMap users;*/
    Linked<IPropertyTree>  globals;
    ResourcePool<OrbizDataManager> connections;
    Owned<IProperties> m_props;
    int m_poolsize;
    int m_waittimeout;
};

#endif 
