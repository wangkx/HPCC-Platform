/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2012 HPCC Systems.

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

#ifndef _ESPWIZ_ws_genesis_HPP__
#define _ESPWIZ_ws_genesis_HPP__

#include "MySVN.hpp"
#include "SVNContextListener.hpp"
#include "ws_genesis_esp.ipp"

#define SUBNET "subnet"
#define HOST "hosts"
#define CLUSTER "cluster"
#define USER "user"
#define GROUP "group"
#define ENVIRONMENT "environment"

#define PropDescription "Description"
#define PropDNSServer "DNSServer"
#define PropKernelConfig "KernelConfig"
#define PropNetworkGateway "NetworkGateway"
#define PropNetworkMask "NetworkMask"
#define PropNetworkNTP "NetworkNTP"
#define PropNetworkPool "NetworkPool"
#define PropOSImage "OSImage"

enum SVNEntryType
{
    SVNNONE = 0,
    SVNFILE = 1,
    SVNFOLDER = 2,
};

class CWsGenesisSoapBindingEx : public CWsGenesisSoapBinding
{
    StringBuffer m_url, m_workdir, m_env, m_user, m_password, m_defaultComment, m_svnConfig;
    char m_pathSep;

    int onListAllGSNodes(IEspContext &context, CHttpRequest* request, CHttpResponse* response);
    int onListGSNodes(IEspContext &context, CHttpRequest* request, CHttpResponse* response);
    int onListGSRoots(IEspContext &context, CHttpRequest* request, CHttpResponse* response);

public:
    CWsGenesisSoapBindingEx(IPropertyTree *cfg, const char *name, const char *process, http_soap_log_level llevel=hsl_none) : CWsGenesisSoapBinding(cfg, name, process, llevel)
    {
        Owned<IPropertyTree> pServiceNode = cfg->getPropTree(StringBuffer("Software/EspProcess[@name=\"").append(process).append("\"]/EspBinding[@name=\"").append(name).append("\"]").str());

        if(!pServiceNode || !pServiceNode->hasProp("SVN") ||
            !pServiceNode->hasProp("SVN/@workDir") ||
            !pServiceNode->hasProp("SVN/@rootURL") ||
            !pServiceNode->hasProp("SVN/@user") ||
            !pServiceNode->hasProp("SVN/@password" ))
                throw MakeStringException(0,"Should specify proper <SVN> tag");

        StringBuffer pw;
        m_url.set(pServiceNode->queryProp("SVN/@rootURL"));
        m_workdir.set(pServiceNode->queryProp("SVN/@workDir"));
        m_user.set(pServiceNode->queryProp("SVN/@user"));
        decrypt(pw,pServiceNode->queryProp("SVN/@password"));
        m_password.set(pw.str());

        m_pathSep = '\\';
        m_svnConfig.append("C:\\localtests\\genesis\\esp\\svn_config");

        m_defaultComment.append("No comment");
    }

    virtual void getNavigationData(IEspContext &context, IPropertyTree & data)
    {
        //Add navigation link here
    }

    virtual const char* getRootPage() {return "files/esp_genesis.html";}

    virtual int onGet(CHttpRequest* request, CHttpResponse* response);
};


class CWsGenesisEx : public CWsGenesis
{
    StringBuffer m_url, m_workdir, m_env, m_user, m_password, m_defaultComment, m_svnConfig;
    char m_pathSep;

    std::map<std::string, std::string> m_svn_path_map;

    bool setReturnStatus(IEspBaseStatus& status, int code, const char *description);
    bool setReturnStatus(IEspBaseStatus& status, IException *e, const char* action, const char* target);
    void setUserAndComment(const char* user, const char* password, const char* comment,
                                     StringBuffer& userStr, StringBuffer& passwordStr, StringBuffer& commentStr);
    svn_revnum_t addFolder(IEspContext &context, const char* fullPath, const char* user, const char* password, const char* comment);
    svn_revnum_t deleteFolder(IEspContext &context, const char* fullPath, const char* user, const char* password, const char* comment);
    void writeFile( StringBuffer& filename, StringBuffer& text );
    svn_revnum_t addFile(IEspContext &context, MySVN& mySVN, StringBuffer& user, StringBuffer& password, StringBuffer& comment, StringBuffer& filePath, const char* content, bool commit = true);
    svn_revnum_t addFile(IEspContext &context, const char* fullPath, const char* content, const char* user, const char* password, const char* comment);
    svn_revnum_t deleteFile(IEspContext &context, MySVN& mySVN, StringBuffer& user, StringBuffer& password, StringBuffer& comment, StringBuffer& filePath, bool commit = true);
    svn_revnum_t deleteFile(IEspContext &context, const char* fullPath, const char* user, const char* password, const char* comment);
    svn_revnum_t addUser(IEspContext &context, const char* userName, const char* user, const char* password, const char* comment, bool group =  false);
    svn_revnum_t removeUser(IEspContext &context, const char* userName, const char* user, const char* password, const char* comment, bool group =  false);
    void setProperties(MySVN& mySVN, const char* targetWithFullPath, StringArray& propertyNames, StringArray& propertyValues);
    void unsetProperties(MySVN& mySVN, const char* targetWithFullPath, const char* comment, const char* user, const char* password,
        svn_revnum_t& rev, const char** unsetupPropertyNames, unsigned numOfUnsetupPropertyNames);
    void unsetProperties(MySVN& mySVN, const char* targetWithFullPath, const char* comment, const char* user,
        const char* password, svn_revnum_t& rev);
    void updateProperties(IEspContext &context, const char* path, const char* target, const char* comment, const char* user,
        const char* password, StringArray& propertyNames, StringArray& propertyValues, svn_revnum_t& rev,
        const char** unsetupPropertyNames, unsigned numOfUnsetupPropertyNames);
    svn_revnum_t updateProperties(IEspContext &context, const char* path, const char* user,
        const char* password, const char* comment, IArrayOf<IConstNamedValue> *properties=NULL);
    bool readPropertyFromPropertiesMap(const svn::PropertiesMap& map, const char* propName, StringBuffer& propValue);
    void collectPropertyRequest(StringArray& propertyNames, StringArray& propertyValues, const char* propName, const char* propValue);
    void collectPropertyRequests(StringArray& propertyNames, StringArray& propertyValues, IArrayOf<IConstGSProperty>& properties);
    void collectSubnetPropertyRequest(StringArray& propertyNames, StringArray& propertyValues, const char* description,
        const char* DNSServer, const char* kernelConfig, const char* networkGateway, const char* networkMask, const char* networkNTP,
        const char* networkPool, const char* OSImage);

    void getNodeSettings(MySVN& mySVN, const char* path, const char* fullPath, IEspGSFolder *folderInfo);
    void getLoginSettings(MySVN& mySVN, const char* fullPath, IEspGSFolder *folderInfo);
    SVNEntryType foundASVNEntry(MySVN& mySVN, const char* fullPath, const char* name);
    unsigned long foundMaxID(MySVN& mySVN, const char* fullPath);
public:
    IMPLEMENT_IINTERFACE;
    virtual ~CWsGenesisEx(){};

    virtual void init(IPropertyTree *cfg, const char *process, const char *service);
    virtual bool onListGSTopNodes(IEspContext &context, IEspListGSTopNodesRequest &req, IEspListGSTopNodesResponse &resp);
    virtual bool onGetGSNodeSettings(IEspContext &context, IEspGetGSNodeSettingsRequest &req, IEspGetGSNodeSettingsResponse &resp);
    virtual bool onUpdateGSNodeProperties(IEspContext &context, IEspUpdateGSNodePropertiesRequest &req, IEspUpdateGSNodePropertiesResponse &resp);
    virtual bool onAddGSFolder(IEspContext &context, IEspAddGSFolderRequest &req, IEspAddGSFolderResponse &resp);
    virtual bool onRemoveGSFolder(IEspContext &context, IEspRemoveGSFolderRequest &req, IEspRemoveGSFolderResponse &resp);
    virtual bool onGetGSFileContent(IEspContext &context, IEspGetGSFileContentRequest &req, IEspGetGSFileContentResponse &resp);
    virtual bool onAddGSFile(IEspContext &context, IEspAddGSFileRequest &req, IEspAddGSFileResponse &resp);
    virtual bool onUpdateGSFileContent(IEspContext &context, IEspUpdateGSFileContentRequest &req, IEspUpdateGSFileContentResponse &resp);
    virtual bool onRemoveGSFile(IEspContext &context, IEspRemoveGSFileRequest &req, IEspRemoveGSFileResponse &resp);
    virtual bool onAddUser(IEspContext &context, IEspAddUserRequest &req, IEspAddUserResponse &resp);
    virtual bool onRemoveUser(IEspContext &context, IEspRemoveUserRequest &req, IEspRemoveUserResponse &resp);
    virtual bool onAddUserGroup(IEspContext &context, IEspAddUserGroupRequest &req, IEspAddUserGroupResponse &resp);
    virtual bool onRemoveUserGroup(IEspContext &context, IEspRemoveUserGroupRequest &req, IEspRemoveUserGroupResponse &resp);

    virtual bool onListGSNavNodes(IEspContext &context, IEspListGSNavNodesRequest &req, IEspListGSNavNodesResponse &resp);
    virtual bool onListGSRoots(IEspContext &context, IEspListGSRootsRequest &req, IEspListGSRootsResponse &resp);

    virtual bool onAddHost(IEspContext &context, IEspAddHostRequest &req, IEspAddHostResponse &resp);
    virtual bool onDeleteHost(IEspContext &context, IEspDeleteHostRequest &req, IEspDeleteHostResponse &resp);
    virtual bool onUpdateHost(IEspContext &context, IEspUpdateHostRequest &req, IEspUpdateHostResponse &resp);
    virtual bool onGetHostInfo(IEspContext &context, IEspGetHostInfoRequest &req, IEspGetHostInfoResponse &resp);
    virtual bool onListHosts(IEspContext &context, IEspListHostsRequest &req, IEspListHostsResponse &resp);
    virtual bool onAddSubnet(IEspContext &context, IEspAddSubnetRequest &req, IEspAddSubnetResponse &resp);
    virtual bool onDeleteSubnet(IEspContext &context, IEspDeleteSubnetRequest &req, IEspDeleteSubnetResponse &resp);
    virtual bool onUpdateSubnet(IEspContext &context, IEspUpdateSubnetRequest &req, IEspUpdateSubnetResponse &resp);
    virtual bool onGetSubnetInfo(IEspContext &context, IEspGetSubnetInfoRequest &req, IEspGetSubnetInfoResponse &resp);
    virtual bool onListSubnets(IEspContext &context, IEspListSubnetsRequest &req, IEspListSubnetsResponse &resp);
    virtual bool onAddCluster(IEspContext &context, IEspAddClusterRequest &req, IEspAddClusterResponse &resp);
    virtual bool onDeleteCluster(IEspContext &context, IEspDeleteClusterRequest &req, IEspDeleteClusterResponse &resp);
    virtual bool onUpdateCluster(IEspContext &context, IEspUpdateClusterRequest &req, IEspUpdateClusterResponse &resp);
    virtual bool onGetClusterInfo(IEspContext &context, IEspGetClusterInfoRequest &req, IEspGetClusterInfoResponse &resp);
    virtual bool onListClusters(IEspContext &context, IEspListClustersRequest &req, IEspListClustersResponse &resp);
    virtual bool onAddEnvironment(IEspContext &context, IEspAddEnvironmentRequest &req, IEspAddEnvironmentResponse &resp);
    virtual bool onDeleteEnvironment(IEspContext &context, IEspDeleteEnvironmentRequest &req, IEspDeleteEnvironmentResponse &resp);
    virtual bool onUpdateEnvironment(IEspContext &context, IEspUpdateEnvironmentRequest &req, IEspUpdateEnvironmentResponse &resp);
    virtual bool onGetEnvironmentInfo(IEspContext &context, IEspGetEnvironmentInfoRequest &req, IEspGetEnvironmentInfoResponse &resp);
    virtual bool onListEnvironments(IEspContext &context, IEspListEnvironmentsRequest &req, IEspListEnvironmentsResponse &resp);
    virtual bool onGetGlobalSettings(IEspContext &context, IEspGetGlobalSettingsRequest &req, IEspGetGlobalSettingsResponse &resp);
    virtual bool onUpdateGlobalSettings(IEspContext &context, IEspUpdateGlobalSettingsRequest &req, IEspUpdateGlobalSettingsResponse &resp);
};

#endif //_ESPWIZ_ws_genesis_HPP__
