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

#pragma warning (disable : 4786)
//#include "jencrypt.hpp"
//#include "hqlplugins.hpp"

#include "ws_orbizservice.hpp"
#include "mysqldatamanager.hpp"
//#include "secloader.hpp"
//#include "hqlerror.hpp"
#include "ws_orbiz_esp.ipp"
//#include "AttributeType.hpp"
//#include "hqlplugininfo.hpp"

struct OrbizDataConnection: public CInterface, implements IResourceFactory<OrbizDataManager>
{
    IMPLEMENT_IINTERFACE;
    OrbizDataConnection(IPropertyTree *_cfg, IProperties* props): cfg(_cfg), m_props(props) {}
    OrbizDataManager* createResource() 
    {
#ifndef _NO_MYSQL_REPOSITORY
        if(cfg->hasProp("Mysql") && cfg->hasProp("Mysql/@server"))
        {
            const char* server = cfg->queryProp("Mysql/@server");
            if(server && *server && strcmp(server, "none") != 0 && strcmp(server, "0.0.0.0") != 0)
            {
                if(!cfg->hasProp("Mysql/@user") || !cfg->hasProp("Mysql/@password"))
                    throw MakeStringException(0,"Should specify proper <Mysql> tag");

                StringAttr dbuser, dbpassword, db("orbit");

                dbuser.set(cfg->queryProp("Mysql/@user"));
                ///StringBuffer password;
                ///decrypt(password,cfg->queryProp("Mysql/@password"));
                ///dbpassword.set(password.str());
                dbpassword.set("lexis");
                if(cfg->hasProp("Mysql/@database_name"))
                    db.set(cfg->queryProp("Mysql/@database_name"));

                MySQLDataManager *mysqlrep = new MySQLDataManager(server,dbuser,dbpassword,db, m_props);

                return mysqlrep;
            }
        }
#endif
        return new OrbizDataManager();
    }

    Linked<IPropertyTree> cfg;
    IProperties* m_props;
};

void CWsOrbizEx::init(IPropertyTree *cfg, const char *process, const char *service)
{
    globals.set(cfg->queryPropTree(StringBuffer("Software/EspProcess[@name=\"").append(process).append("\"]/EspService[@name=\"").append(service).append("\"]").str()));

    m_poolsize = 0;
    m_waittimeout = 0;
    IPropertyTree* mysqlcfg = NULL;
    if(globals.get())
        mysqlcfg = globals->getPropTree("mysql");
    if(mysqlcfg)
    {
        m_poolsize = mysqlcfg->getPropInt("@poolSize");
        m_waittimeout = mysqlcfg->getPropInt("@waitTimeout");
    }
    if(m_poolsize <= 0)
        m_poolsize = 10;
    if(m_waittimeout <= 0)
        m_waittimeout = REPOSITORY_CONNECTION_WAIT_SECONDS;

    connections.init(m_poolsize,Owned<OrbizDataConnection>(new OrbizDataConnection(globals, m_props.get())));
    /*plugins.setown(repositoryCommon::loadPlugins(globals->queryProp("Plugins/@path")));
    pluginMap.setown(repositoryCommon::createPluginPropertyTree(plugins, false));*/
}

bool CWsOrbizEx::onListContacts(IEspContext &context, IEspListContactsRequest &req, IEspListContactsResponse &resp)
{
    /*IArrayOf<IEspECLModule> modules;
    GetModules(mapUser(context),req.getModifiedSince(),req.getIncludeDeleted(), req.getGetChecksum(), nullif(req.getLabel()), nullif(req.getEarMark()), modules);
    resp.setOutModules(modules);*/
    return true;
}

bool CWsOrbizEx::onAddContact(IEspContext &context, IEspAddContactRequest &req, IEspAddContactResponse &resp)
{
    /*IArrayOf<IEspECLModule> modules;
    GetModules(mapUser(context),req.getModifiedSince(),req.getIncludeDeleted(), req.getGetChecksum(), nullif(req.getLabel()), nullif(req.getEarMark()), modules);
    resp.setOutModules(modules);*/
    return true;
}

bool CWsOrbizEx::onUpdateContact(IEspContext &context, IEspUpdateContactRequest &req, IEspUpdateContactResponse &resp)
{
    /*IArrayOf<IEspECLModule> modules;
    GetModules(mapUser(context),req.getModifiedSince(),req.getIncludeDeleted(), req.getGetChecksum(), nullif(req.getLabel()), nullif(req.getEarMark()), modules);
    resp.setOutModules(modules);*/
    return true;
}

bool CWsOrbizEx::onDeleteContacts(IEspContext &context, IEspDeleteContactsRequest &req, IEspDeleteContactsResponse &resp)
{
    /*IArrayOf<IEspECLModule> modules;
    GetModules(mapUser(context),req.getModifiedSince(),req.getIncludeDeleted(), req.getGetChecksum(), nullif(req.getLabel()), nullif(req.getEarMark()), modules);
    resp.setOutModules(modules);*/
    return true;
}
