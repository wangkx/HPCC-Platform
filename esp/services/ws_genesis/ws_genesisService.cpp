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

#pragma warning (disable : 4786)

#include "ws_genesisService.hpp"
//#include "genesis_errors.h"

const unsigned numOfSubnetProperties = 8;
const char* subnetProperties[numOfSubnetProperties] = {PropDescription, PropDNSServer, PropKernelConfig, PropNetworkGateway,
PropNetworkMask, PropNetworkNTP, PropNetworkPool, PropOSImage};

static inline bool isPathSeparator(char sep)
{
    return (sep=='\\')||(sep=='/');
}

static inline const char *skipPathNodes(const char *&s, int skip)
{
    if (s) {
        while (*s) {
            if (isPathSeparator(*s++))
                if (!skip--)
                    return s;
        }
    }
    return NULL;
}

static inline const char *nextPathNode(const char *&s, StringBuffer &node, int skip=0)
{
    if (skip)
        skipPathNodes(s, skip);
    if (s) while (*s) {
        if (isPathSeparator(*s))
            return s++;
        node.append(*s++);
    }
    return NULL;
}

static inline const char *firstPathNode(const char *&s, StringBuffer &node)
{
    if (s && isPathSeparator(*s))
        s++;
    return nextPathNode(s, node);
}

void getTime(StringBuffer &outTime, apr_time_t inTime )
{
    apr_time_exp_t result;
    apr_status_t status = apr_time_exp_tz  	(  	
        &result,
		inTime,     //input,
		0           //gmt offset in seconds 
	);
    char buf[50];
    sprintf( buf, 
        "%02d-%02d-%02dT%02d:%02d:%02dZ", 
        result.tm_year+1900,
        result.tm_mon+1,
        result.tm_mday,
        result.tm_hour,
        result.tm_min,
        result.tm_sec);

    outTime.clear().append( buf );
}

inline void indenter(StringBuffer &s, int count)
{
    s.appendN(count*3, ' ');
}


inline const char *jsonNewline(bool ESCAPEFORMATTERS){return ((ESCAPEFORMATTERS) ? "\\n" : "\n");}

void addJsonNode(StringBuffer& json, const char* ref, const char* name, const char* path, const char* description, unsigned depth, bool children =  true)
{
    indenter(json, depth);
    if (!description || !*description)
        json.appendf("{ $ref: '%s', name:'%s', path:'%s'", ref, name, path);
    else
        json.appendf("{ $ref: '%s', name:'%s', path:'%s', description:'%s'", ref, name, path, description);
    if (children)
        json.appendf(", children:true},%s", jsonNewline(false));
    else
        json.appendf("},%s", jsonNewline(false));
}

void buildTopNodeJsonString(StringBuffer& json, IArrayOf<IEspGSNode>& nodes )
{
    json.appendf("[", jsonNewline(false));
    ForEachItemIn(i, nodes)
    {
        IConstGSNode& node = nodes.item(i);
        addJsonNode(json, node.getRef(), node.getName(), node.getSVNPath(), node.getDescription(), 1, !node.getIsLeaf());
    }
    json.append("]");
}

void buildNodeJsonString(StringBuffer& json, const char* id, const char* name, const char* description, IArrayOf<IEspGSNode>& nodes )
{
    if (!description || !*description)
        json.appendf("{ id: '%s', name:'%s', children:[%s", id, name, jsonNewline(false));
    else
        json.appendf("{ id: '%s', name:'%s', description:'%s', children:[", id, name, description, jsonNewline(false));
    ForEachItemIn(i, nodes)
    {
        IConstGSNode& node = nodes.item(i);
        addJsonNode(json, node.getRef(), node.getName(), node.getParent(), node.getDescription(), 2, !node.getIsLeaf());
    }
    json.append("]}");
}

void addNodeToArray(IArrayOf<IEspGSNode>& nodes, const char* id, const char* name, const char* parent, const char* description, bool children =  true)
{
    Owned<IEspGSNode> node = createGSNode();
    node->setRef(id); //For now, Ref/ID and name are the same
    node->setName(name);
    node->setParent(parent);
    node->setIsLeaf(!children);
    if (description && !description)
        node->setDescription(description);
    nodes.append(*node.getLink());
}

void addJsonNode2(StringBuffer& json, const char* ref, const char* name, const char* parent, const char* description, unsigned depth, int nodeCount, bool children =  true)
{
    if (nodeCount > 0)
        json.appendf(",%s", jsonNewline(false));

    indenter(json, depth);
    if (!description || !*description)
        json.appendf("{ 'id': '%s', 'name': '%s', 'parent': '%s'}", ref, name, parent);
    else
        json.appendf("{ 'id': '%s', 'name': '%s', 'parent:' '%s', 'description': '%s'}", ref, name, parent, description);

    if (children)
    {
        StringBuffer dummyName, dummyID;
        dummyName.appendf("Dummy%s", name);
        dummyID.appendf("dummy_%s", ref);

        json.appendf(",%s", jsonNewline(false));
        indenter(json, depth+1);
        json.appendf("{ 'id': '%s', 'name': '%s', 'parent': '%s'}", dummyID.str(), dummyName.str(), ref);
    }
}

void addJsonNode4(StringBuffer& json, const char* ref, const char* name, const char* parent, const char* description, bool leaf =  true)
{
    if (json.length() > 0)
        json.appendf(",%s", jsonNewline(false));

    indenter(json, 1);
    json.appendf("{ 'id': '%s', 'name': '%s', 'parent': '%s'", ref, name, parent);
    if (leaf)
        json.append(", 'type': 'file'");
    else
        json.append(", 'type': 'folder'");

    if (description && *description)
        json.appendf("'description': '%s'}", description);
    json.append("}");
}

void buildNodeJsonString2(StringBuffer& json, const char* id, const char* name, const char* description, IArrayOf<IEspGSNode>& nodes )
{
    json.appendf("[%s", jsonNewline(false));
    indenter(json, 1);
    if (!description || !*description)
        json.appendf("{ id: '%s', name:'%s' },%s", id, name, jsonNewline(false));
    else
        json.appendf("{ id: '%s', name:'%s', description:'%s' },%s", id, name, description, jsonNewline(false));
    ForEachItemIn(i, nodes)
    {
        IConstGSNode& node = nodes.item(i);
        addJsonNode2(json, node.getRef(), node.getName(), node.getParent(), node.getDescription(), 2, i, !node.getIsLeaf());
    }
    json.appendf("%s]", jsonNewline(false));
}

void buildNodeJsonString3(StringBuffer& json, const char* id, const char* name, const char* description, IArrayOf<IEspGSNode>& nodes )
{
    json.appendf("{%s", jsonNewline(false));
    indenter(json, 1);
    json.appendf("\"Nodes\": [%s", jsonNewline(false));
    ForEachItemIn(i, nodes)
    {
        IConstGSNode& node = nodes.item(i);
        addJsonNode2(json, node.getRef(), node.getName(), node.getParent(), node.getDescription(), 2, i, !node.getIsLeaf());
    }
    json.append(jsonNewline(false));
    indenter(json, 1);
    json.appendf("]%s", jsonNewline(false));
    json.append("}");
}

void buildNodeJsonString4(StringBuffer& json, const char* id, const char* name, const char* pane, const char* description, IArrayOf<IEspGSNode>& nodes )
{
    addJsonNode4(json, id, name, pane, description, false);
    ForEachItemIn(i, nodes)
    {
        IConstGSNode& node = nodes.item(i);
        addJsonNode4(json, node.getRef(), node.getName(), node.getParent(), node.getDescription());
    }
}

void addRootNodeToJSONString(StringBuffer& json, const char* id, const char* name)
{
    StringBuffer buf;
    buf.appendf("[%s", jsonNewline(false));
    indenter(buf, 1);
    buf.appendf("{ 'id': '%s', 'name': '%s', 'type': 'folder'},%s", id, name, jsonNewline(false));
    json.insert(0, buf.str());
    json.appendf("%s]", jsonNewline(false));
}

void CWsGenesisEx::init(IPropertyTree *cfg, const char *process, const char *service)
{
    Owned<IPropertyTree> pServiceNode = cfg->getPropTree(StringBuffer("Software/EspProcess[@name=\"").append(process).append("\"]/EspService[@name=\"").append(service).append("\"]").str());

    if(!pServiceNode->hasProp("SVN") ||
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

    m_svn_path_map["Global"]="global";
    m_svn_path_map["Roles"]="roles";
    m_svn_path_map["Subnets"]="subnet";
    m_svn_path_map["Hosts"]="host";
    m_svn_path_map["Clusters"]="cluster";
    m_svn_path_map["Environments"]="environments";
    m_svn_path_map["Users"]="logins/users/name";
    m_svn_path_map["Groups"]="logins/groups/name";
}

bool CWsGenesisEx::setReturnStatus(IEspBaseStatus& status, int code, const char *description)
{
    status.setDescription(description);
    status.setCode(code);
    return true;
}

bool CWsGenesisEx::setReturnStatus(IEspBaseStatus& status, IException *e, const char* action, const char* target)
{
    if (!e || !action || !*action)
        return false;

    StringBuffer eMsg, msg;
    if (target && *target)
        msg.appendf("Unsuccessfully %s %s: %s", action, target, e->errorMessage(eMsg).str());
    else
        msg.appendf("Unsuccessfully %s: %s", action, e->errorMessage(eMsg).str());
    DBGLOG(0, msg.str());
    setReturnStatus(status, e->errorCode(), msg.str());
    e->Release();
    return false;
}

void CWsGenesisEx::setUserAndComment(const char* user, const char* password, const char* comment,
                                     StringBuffer& userStr, StringBuffer& passwordStr, StringBuffer& commentStr)
{
    if (!user || !*user)
        userStr = m_user;
    else
        userStr.set(user);

    if (!password || !*password)
        passwordStr = m_password;
    else
        passwordStr.set(password);

    if (!comment || !*comment)
        commentStr = m_defaultComment;
    else
        commentStr.set(comment);
}

void CWsGenesisEx::writeFile( StringBuffer& filename, StringBuffer& text )
{
    recursiveCreateDirectoryForFile( filename.str() );

    Owned<IFile> ifile = createIFile( filename.str() );
    OwnedIFileIO ifileio = ifile->open(IFOcreaterw);
    if (!ifileio)
        throw MakeStringException(0, "Failed to open: %s", ifile->queryFilename()  );

    size32_t nBytes = ifileio->write( 0, text.length(), text.str() );
    if( nBytes != text.length() )
        throw MakeStringException(-1, "Wrote <%d> bytes out of <%d>.", nBytes,  text.length() );
}

svn_revnum_t CWsGenesisEx::addFolder(IEspContext &context, const char* fullPath, const char* user, const char* password, const char* comment)
{
    //set path
    StringBuffer folderPath, folderURL;
    folderURL.appendf("%s/%s", m_url.str(), fullPath);
    folderPath.appendf("%s%c%s", m_workdir.str(), m_pathSep, fullPath);

    recursiveCreateDirectory( folderPath.str() );
    DBGLOG("Folder: <%s> created", folderPath.str() );

    StringBuffer userStr, passwordStr, commentStr;
    setUserAndComment(user, password, comment, userStr, passwordStr, commentStr);

    MySVN mySVN(m_svnConfig.str(), userStr.str(), passwordStr.str(), commentStr.str());       
    mySVN.svnCheckout( m_url.str(), m_workdir.str() );

    //svn add
    mySVN.svnAdd( folderPath.str() );
    svn_revnum_t rev = mySVN.svnCommit( folderPath.str(), commentStr.str(), NULL, userStr.str(), passwordStr.str());
    DBGLOG("Folder: <%s> added to SVN: revision <%d>", folderPath.str(), rev );
    return rev;
}

svn_revnum_t CWsGenesisEx::deleteFolder(IEspContext &context, const char* fullPath, const char* user, const char* password, const char* comment)
{
    //set path
    StringBuffer folderPath, folderURL;
    folderURL.appendf("%s/%s", m_url.str(), fullPath);
    folderPath.appendf("%s%c%s", m_workdir.str(), m_pathSep, fullPath);

    StringBuffer userStr, passwordStr, commentStr;
    setUserAndComment(user, password, comment, userStr, passwordStr, commentStr);

    MySVN mySVN(m_svnConfig.str(), userStr.str(), passwordStr.str(), commentStr.str());       
    mySVN.svnCheckout( m_url.str(), m_workdir.str() );

    //svn delete
    mySVN.svnRemove( folderPath.str() );
    svn_revnum_t rev = mySVN.svnCommit( folderPath.str(), commentStr.str(), NULL, userStr.str(), passwordStr.str());

    DBGLOG("Folder: <%s> deleted from SVN: revision <%d>", folderPath.str(), rev );

    RemoveDirectory( folderPath.str() );
    DBGLOG("Folder: <%s> deleted", folderPath.str() );
    return rev;
}

svn_revnum_t CWsGenesisEx::addFile(IEspContext &context, MySVN& mySVN, StringBuffer& user, StringBuffer& password,
                                   StringBuffer& comment, StringBuffer& filePath, const char* content, bool commit)
{
    if(!content || !*content)
        content = "";

    StringBuffer textSB( content );

    // save file
    writeFile( filePath, textSB );
    DBGLOG( "File %s added", filePath.str() );

    // svn add
    mySVN.svnAdd( filePath.str() );
    if (!commit)
        return 0;

    svn_revnum_t rev = mySVN.svnCommit( filePath.str(), comment.str(), NULL, user.str(), password.str());
    DBGLOG("File: <%s> added to SVN: revision <%d>", filePath.str(), rev );
    return rev;
}

svn_revnum_t CWsGenesisEx::addFile(IEspContext &context, const char* fullPath, const char* content, const char* user, const char* password, const char* comment)
{
    StringBuffer userStr, passwordStr, commentStr;
    setUserAndComment(user, password, comment, userStr, passwordStr, commentStr);
    MySVN mySVN(m_svnConfig.str(), userStr.str(), passwordStr.str(), commentStr.str()); 

    //set path
    StringBuffer filePath;
    filePath.appendf("%s%c%s", m_workdir.str(), m_pathSep, fullPath);

    return addFile(context, mySVN, userStr, passwordStr, commentStr, filePath, content);
}

svn_revnum_t CWsGenesisEx::deleteFile(IEspContext &context, MySVN& mySVN, StringBuffer& user, StringBuffer& password,
                                   StringBuffer& comment, StringBuffer& filePath, bool commit)
{
    // svn delete
    svn_revnum_t rev = 0;
    mySVN.svnRemove( filePath.str() );
    if (commit)
        rev = mySVN.svnCommit( filePath.str(), comment.str(), NULL, user.str(), password.str());

    DBGLOG("File: <%s> deleted from SVN: revision <%d>", filePath.str(), rev );

    // delete file
    DeleteFile( filePath );
    DBGLOG( "File %s deleted", filePath.str() );
    return rev;
}

svn_revnum_t CWsGenesisEx::deleteFile(IEspContext &context, const char* fullPath, const char* user, const char* password, const char* comment)
{
    //set path
    StringBuffer filePath;
    filePath.appendf("%s%c%s", m_workdir.str(), m_pathSep, fullPath);

    StringBuffer userStr, passwordStr, commentStr;
    setUserAndComment(user, password, comment, userStr, passwordStr, commentStr);

    MySVN mySVN(m_svnConfig.str(), userStr.str(), passwordStr.str(), commentStr.str());       
    return deleteFile(context, mySVN, userStr, passwordStr, commentStr, filePath);
}

svn_revnum_t CWsGenesisEx::addUser(IEspContext &context, const char* userName, const char* user, const char* password, 
                                   const char* comment, bool group)
{
    StringBuffer userStr, passwordStr, commentStr;
    setUserAndComment(user, password, comment, userStr, passwordStr, commentStr);

    MySVN mySVN(m_svnConfig.str(), userStr.str(), passwordStr.str(), commentStr.str());     

    const char* folderName = "users";
    const char* idPropName = "uid";
    const char* namePropName = "user_name";
    if (group)
    {
        folderName = "groups";
        idPropName = "gid";
        namePropName = "group_name";
    }

    StringBuffer idStr, svnPath, svnPath1, svnPath2;
    svnPath.appendf("%s%clogins%c%s", m_workdir.str(), m_pathSep, m_pathSep, folderName);
    svnPath1.appendf("%s%cnumeric", svnPath.str(), m_pathSep);
    svnPath2.appendf("%s%cname", svnPath.str(), m_pathSep);
    if (foundASVNEntry(mySVN, svnPath2.str(), userName) != SVNNONE)
        throw MakeStringException(-1, "User(group) %s always exists", userName);

    unsigned long maxID = foundMaxID(mySVN, svnPath1.str());

    idStr.appendf("%ld", maxID+1);
    svnPath1.appendf("%c%d", m_pathSep, maxID+1);
    svnPath2.appendf("%c%s", m_pathSep, userName);

    addFile(context, mySVN, userStr, passwordStr, commentStr, svnPath1, NULL, false);
    addFile(context, mySVN, userStr, passwordStr, commentStr, svnPath2, NULL, false);
    mySVN.svnSetProperty(  svnPath1.str(), namePropName,  userName);
    mySVN.svnSetProperty(  svnPath2.str(), idPropName,  idStr.str());

    svn_revnum_t rev = mySVN.svnCommit( svnPath.str(), commentStr.str(), NULL, userStr.str(), passwordStr.str());
    DBGLOG("File: <%s> added to SVN: revision <%d>", svnPath2.str(), rev );
    return rev;
}

svn_revnum_t CWsGenesisEx::removeUser(IEspContext &context, const char* userName, const char* user, const char* password,
                                      const char* comment, bool group)
{
    StringBuffer userStr, passwordStr, commentStr;
    setUserAndComment(user, password, comment, userStr, passwordStr, commentStr);

    MySVN mySVN(m_svnConfig.str(), userStr.str(), passwordStr.str(), commentStr.str());  

    const char* folderName = "users";
    const char* idPropName = "uid";
    if (group)
    {
        folderName = "groups";
        idPropName = "gid";
    }

    //set path
    StringBuffer idStr, svnPath, svnPath1, svnPath2;
    svnPath.appendf("%s%clogins%c%s", m_workdir.str(), m_pathSep, m_pathSep, folderName);
    svnPath2.appendf("%s%cname%c%s", svnPath.str(), m_pathSep, m_pathSep, userName);
    
    mySVN.svnGetProperty(svnPath2.str(), idPropName, idStr );
    if (idStr.length() < 1)
        throw MakeStringException(-1, "ID not found for %s", userName);

    svnPath1.appendf("%s%cnumeric%c%s", svnPath.str(), m_pathSep, m_pathSep, idStr.str());
    deleteFile(context, mySVN, userStr, passwordStr, commentStr, svnPath1, false);
    deleteFile(context, mySVN, userStr, passwordStr, commentStr, svnPath2, false);
    svn_revnum_t rev = mySVN.svnCommit( svnPath.str(), commentStr.str(), NULL, userStr.str(), passwordStr.str());
    DBGLOG("File: <%s> added to SVN: revision <%d>", svnPath2.str(), rev );
    return rev;
}

void CWsGenesisEx::setProperties(MySVN& mySVN, const char* targetWithFullPath, StringArray& propertyNames, StringArray& propertyValues)
{
    if (!targetWithFullPath || !*targetWithFullPath)
        throw MakeStringException(-1, "path or folder not specified" );

    //set requested properties
    ForEachItemIn(i, propertyNames)
    {
        const char* propertyName = propertyNames.item(i);
        const char* propertyValue = propertyValues.item(i);
        if ((!propertyName || !*propertyName) || (!propertyValue || !*propertyValue))
            continue;

        mySVN.svnSetProperty(  targetWithFullPath, propertyName,  propertyValue);
    }
}

void CWsGenesisEx::unsetProperties(MySVN& mySVN, const char* targetWithFullPath, const char* comment, const char* user,
    const char* password, svn_revnum_t& rev, const char** unsetupPropertyNames, unsigned numOfUnsetupPropertyNames)
{
    bool needCommit = false;
    svn::PathPropertiesMapList propertyEntries;
    mySVN.svnListProperties(propertyEntries, targetWithFullPath);
    const svn::PathPropertiesMapEntry &propEntry = (*propertyEntries.begin ());
    const svn::PropertiesMap& propMap = propEntry.second;
    for (unsigned i = 0; i < numOfUnsetupPropertyNames; i++)
    {
        const char* propertyName = unsetupPropertyNames[i];
        svn::PropertiesMap::const_iterator it = propMap.find(std::string(propertyName));
        if (it == propMap.end())
            continue;

        mySVN.svnUnsetProperty(  targetWithFullPath, propertyName);
        needCommit = true;
    }

    if (needCommit)
        rev = mySVN.svnCommit( targetWithFullPath, comment, NULL, user, password);
}

void CWsGenesisEx::unsetProperties(MySVN& mySVN, const char* targetWithFullPath, const char* comment, const char* user,
    const char* password, svn_revnum_t& rev)
{
    bool needCommit = false;
    svn::PathPropertiesMapList propertyEntries;
    mySVN.svnListProperties(propertyEntries, targetWithFullPath);
    svn::PathPropertiesMapList::const_iterator it;

    for (it=propertyEntries.begin (); it!=propertyEntries.end (); it++)
    {
        const svn::PathPropertiesMapEntry &propEntry = (*it);
        const svn::PropertiesMap& propMap = propEntry.second;

        svn::PropertiesMap::const_iterator iter;
        for (iter = propMap.begin(); iter != propMap.end(); ++iter)
        {
            mySVN.svnUnsetProperty(  targetWithFullPath, iter->first.c_str());
            needCommit = true;
        }
    }

    if (needCommit)
        rev = mySVN.svnCommit( targetWithFullPath, comment, NULL, user, password);
}

void CWsGenesisEx::updateProperties(IEspContext &context, const char* path, const char* target, const char* comment, const char* user,
    const char* password, StringArray& propertyNames, StringArray& propertyValues, svn_revnum_t& rev, 
    const char** unsetupPropertyNames, unsigned numOfUnsetupPropertyNames)
{
    if (!target || !*target)
        throw MakeStringException(-1, "path or folder not specified" );

    if(!comment || !*comment)
        comment = "no checkin comment";

    //set path
    StringBuffer folderPath;
    folderPath.appendf("%s%c%s%c%s", m_workdir.str(), m_pathSep, path, m_pathSep, target);

    MySVN mySVN(m_svnConfig.str(), m_user.str(), m_password.str(), comment);

    mySVN.svnCheckout( m_url.str(), m_workdir.str() );

    unsetProperties(mySVN, folderPath.str(), comment, user, password, rev, unsetupPropertyNames, numOfUnsetupPropertyNames);

    //set requested properties
    ///setProperties(mySVN, folderPath.str(), propertyNames, propertyValues, update);
    ForEachItemIn(i, propertyNames)
    {
        const char* propertyName = propertyNames.item(i);
        const char* propertyValue = propertyValues.item(i);
        if ((!propertyName || !*propertyName) || (!propertyValue || !*propertyValue))
            continue;

        mySVN.svnSetProperty(  folderPath.str(), propertyName,  propertyValue);
    }

    rev = mySVN.svnCommit( folderPath.str(), comment, NULL, user, password);
    DBGLOG("Properties updated for: <%s>: revision <%d>", folderPath.str(), rev );
}

svn_revnum_t CWsGenesisEx::updateProperties(IEspContext &context, const char* path, const char* user,
    const char* password, const char* comment, IArrayOf<IConstNamedValue> *properties)
{
    if (!path || !*path)
        throw MakeStringException(-1, "Node not specified" );

    if(!comment || !*comment)
        comment = "no checkin comment";

    //set path
    StringBuffer folderPath;
    folderPath.appendf("%s%c%s", m_workdir.str(), m_pathSep, path);

    MySVN mySVN(m_svnConfig.str(), user, password, comment);

    mySVN.svnCheckout( m_url.str(), m_workdir.str() );

    svn_revnum_t rev;
    unsetProperties(mySVN, folderPath.str(), comment, user, password, rev);

    //set requested properties
    if (!properties || !properties->length())
        return 0;

    bool needCommit = false;
    ForEachItemIn(i, *properties)
    {
        IConstNamedValue &item = properties->item(i);
        const char *name = item.getName();
        const char *value = item.getValue();
        if (!name || !*name || !value || !*value)
            continue;

        mySVN.svnSetProperty(  folderPath.str(), name,  value);
        needCommit = true;
    }

    if (needCommit)
        rev = mySVN.svnCommit( folderPath.str(), comment, NULL, user, password);
    DBGLOG("Properties updated for: <%s>: revision <%d>", folderPath.str(), rev );
    return rev;
}

bool CWsGenesisEx::readPropertyFromPropertiesMap(const svn::PropertiesMap& propMap, const char* propName, StringBuffer& propValue)
{
    svn::PropertiesMap::const_iterator it = propMap.find(std::string(propName));
    if (it == propMap.end())
        return false;

    std::string s = (*it).second;
    if (s.empty())
        return false;

    propValue.append(s.c_str());
    return true;
}

void CWsGenesisEx::collectPropertyRequest(StringArray& propertyNames, StringArray& propertyValues, const char* propName, const char* propValue)
{
    if (!propName || !*propName || !propValue || !*propValue)
        return;

    propertyNames.append(propName);
    propertyValues.append(propValue);
}

/* Keep!!
void CWsGenesisEx::collectPropertyRequests(StringArray& propertyNames, StringArray& propertyValues, IArrayOf<IConstGSProperty>& properties)
{
    ForEachItemIn(i, properties)
    {
        IConstGSProperty& item= properties.item(i);
        const char* propName = item.getName();
        if (!propName || !*propName)
            continue;
        StringArray& propValues = item.getValues();
        ForEachItemIn(ii, propValues)
        {
            const char* propValue = propValues.item(ii);
            if (!propValue || !*propValue)
                continue;
            propertyNames.append(propName);
            propertyValues.append(propValue);
        }
    }
}

void CWsGenesisEx::collectSubnetPropertyRequest(StringArray& propertyNames, StringArray& propertyValues, const char* description, 
    const char* DNSServer, const char* kernelConfig, const char* networkGateway, const char* networkMask, const char* networkNTP, 
    const char* networkPool, const char* OSImage)
{
    collectPropertyRequest(propertyNames, propertyValues, PropDNSServer, DNSServer);
    collectPropertyRequest(propertyNames, propertyValues, PropNetworkGateway, networkGateway);
    collectPropertyRequest(propertyNames, propertyValues, PropNetworkMask, networkMask);
    collectPropertyRequest(propertyNames, propertyValues, PropNetworkNTP, networkNTP);
    collectPropertyRequest(propertyNames, propertyValues, PropNetworkPool, networkPool);
    collectPropertyRequest(propertyNames, propertyValues, PropKernelConfig, kernelConfig);
    collectPropertyRequest(propertyNames, propertyValues, PropOSImage, OSImage);
    collectPropertyRequest(propertyNames, propertyValues, PropDescription, description);
}
*/

SVNEntryType CWsGenesisEx::foundASVNEntry(MySVN& mySVN, const char* fullPath, const char* name)
{
    SVNEntryType ret = SVNNONE;
    svn::DirEntries dirEntries;
    mySVN.svnList( dirEntries, fullPath );
    svn::DirEntries::const_iterator it;
    for (it=dirEntries.begin (); it!=dirEntries.end (); it++)
    {
        const svn::DirEntry &dir_entry = (*it);
        const char* aName = dir_entry.name();
        if (!aName || !streq(aName, name))
            continue;

        svn_node_kind_t nodeKind = dir_entry.kind();
        if (nodeKind == svn_node_dir)
            ret = SVNFOLDER;
        else
            ret = SVNFILE;
    }

    return ret;
}

unsigned long CWsGenesisEx::foundMaxID(MySVN& mySVN, const char* fullPath)
{
    unsigned long ret = 0;
    svn::DirEntries dirEntries;
    mySVN.svnList( dirEntries, fullPath );
    svn::DirEntries::const_iterator it;
    for (it=dirEntries.begin (); it!=dirEntries.end (); it++)
    {
        const svn::DirEntry &dir_entry = (*it);
        const char* name = dir_entry.name();
        if (!name || !*name)
            continue;
        
        char *epTr;
        unsigned long id = strtoul(name, &epTr, 10);
        if (!epTr)
            continue;

        if (ret < id)
            ret = id;
    }

    return ret;
}

void CWsGenesisEx::getNodeSettings(MySVN& mySVN, const char* path, const char* fullPath, IEspGSFolder *folderInfo)
{
    svn::DirEntries dirEntries;
    mySVN.svnList( dirEntries, fullPath );

    IArrayOf<IEspNamedValue> properties;
    IArrayOf<IEspGSFolder> subFolders;
    IArrayOf<IEspGSFile> files;
    svn::DirEntries::const_iterator it;
    for (it=dirEntries.begin (); it!=dirEntries.end (); it++)
    {
        const svn::DirEntry &dir_entry = (*it);
        const char* name = dir_entry.name();
        if (!name || !*name)
            continue;

        StringBuffer svnPath, svnFullPath;
        svnPath.appendf("%s%c%s", path, m_pathSep, name);
        svnFullPath.appendf("%s%c%s", fullPath, m_pathSep, name);

        svn_node_kind_t nodeKind = dir_entry.kind();
        if (nodeKind == svn_node_dir)
        {
            Owned<IEspGSFolder> aFolder = createGSFolder();
            aFolder->setName(name);
            aFolder->setSVNPath(svnPath.str());
            getNodeSettings(mySVN, svnPath.str(), svnFullPath.str(), aFolder);
            subFolders.append(*aFolder.getLink());
        }
        else
        {
            Owned<IEspGSFile> aFile = createGSFile();
            aFile->setName(name);
            aFile->setSVNPath(svnPath.str());
            files.append(*aFile.getLink());
        }
    }

    svn::PathPropertiesMapList propertyEntries;
    mySVN.svnListProperties(propertyEntries, fullPath);
    svn::PathPropertiesMapList::const_iterator itr;

    for (itr=propertyEntries.begin (); itr!=propertyEntries.end (); itr++)
    {
        const svn::PathPropertiesMapEntry &propEntry = (*itr);
        const svn::PropertiesMap& propMap = propEntry.second;

        svn::PropertiesMap::const_iterator iter;
        for (iter = propMap.begin(); iter != propMap.end(); ++iter)
        {
            Owned<IEspNamedValue> aProperty = createNamedValue();
            aProperty->setName(iter->first.c_str());
            aProperty->setValue(iter->second.c_str());
            properties.append(*aProperty.getLink());
        }
    }

    if (!properties.empty())
        folderInfo->setProperties(properties);
    if (!files.empty())
        folderInfo->setFiles(files);
    if (!subFolders.empty())
        folderInfo->setSubFolders(subFolders);
}

void CWsGenesisEx::getLoginSettings(MySVN& mySVN, const char* fullPath, IEspGSFolder *folderInfo)
{
    IArrayOf<IEspNamedValue> properties;
    svn::PathPropertiesMapList propertyEntries;
    mySVN.svnListProperties(propertyEntries, fullPath);
    svn::PathPropertiesMapList::const_iterator itr;

    for (itr=propertyEntries.begin (); itr!=propertyEntries.end (); itr++)
    {
        const svn::PathPropertiesMapEntry &propEntry = (*itr);
        const svn::PropertiesMap& propMap = propEntry.second;

        svn::PropertiesMap::const_iterator iter;
        for (iter = propMap.begin(); iter != propMap.end(); ++iter)
        {
            Owned<IEspNamedValue> aProperty = createNamedValue();
            aProperty->setName(iter->first.c_str());
            aProperty->setValue(iter->second.c_str());
            properties.append(*aProperty.getLink());
        }
    }

    if (!properties.empty())
        folderInfo->setProperties(properties);
}

void addToGSFolderArray(const char* id, const char* name, const char* parent, const char* svnPath,
                        const char* type, const char* description, IArrayOf<IEspGSFolder>& subFolders)
{
    Owned<IEspGSFolder> aFolder = createGSFolder();
    aFolder->setID(id);
    aFolder->setName(name);
    if (svnPath && *svnPath)
        aFolder->setSVNPath(svnPath);
    if (type && *type)
        aFolder->setType(type);
    if (parent && *parent)
        aFolder->setParent(parent);
    if (description && *description)
        aFolder->setDescription(description);

    subFolders.append(*aFolder.getLink());
}

bool CWsGenesisEx::onListGSNavNodes(IEspContext &context, IEspListGSNavNodesRequest &req, IEspListGSNavNodesResponse &resp)
{
    StringBuffer msg;
    const char* navPane = NULL;
    try
    {
        navPane = req.getNavPane();
        if (!navPane || !*navPane)
            throw MakeStringException(-1, "Parent node not specified");

        StringArray& dirsForDisplay = req.getDirNames();

        IArrayOf<IEspGSFolder> folders;
        const char* navID = "root";
        addToGSFolderArray(navID, navPane, NULL, NULL, NULL, NULL, folders);

        MySVN mySVN(m_svnConfig.str(), m_user.str(), m_password.str(), m_defaultComment.str());
        ForEachItemIn(i, dirsForDisplay)
        {
            const char* parentName = dirsForDisplay.item(i);
            if (!parentName || !*parentName)
                continue;

            std::string svnPath = m_svn_path_map[parentName];
            if(svnPath.size() == 0)
            {
                DBGLOG("Cannot find svn path for %s", parentName);
                continue;
            }

            if (strieq(parentName, "global"))
            {
                addToGSFolderArray(parentName, parentName, navID, svnPath.c_str(), "top_node", NULL, folders);
                continue;
            }
            else
                addToGSFolderArray(parentName, parentName, navID, "", "top_node", NULL, folders);

            StringBuffer fullPath;
            fullPath.appendf("%s%c%s", m_workdir.str(), m_pathSep, svnPath.c_str());

            svn::DirEntries dirEntries;
            mySVN.svnList( dirEntries, fullPath.str() );

            IArrayOf<IEspGSNode> nodes;
            svn::DirEntries::const_iterator it;
            for (it=dirEntries.begin (); it!=dirEntries.end (); it++)
            {
                const svn::DirEntry &dir_entry = (*it);
                const char* name = dir_entry.name();
                if (!name || !*name)
                    continue;

                StringBuffer pathBuf;
                pathBuf.appendf("%s%c%s", svnPath.c_str(), m_pathSep, name);
                addToGSFolderArray(name, name, parentName, pathBuf.str(), "leaf", NULL, folders);
            }
        }

        if (!folders.empty())
            resp.setFolders(folders);
    }
    catch (IException *e)
    {
        return setReturnStatus(resp.updateStatus(), e, "onListGSRoots", navPane);
    }

    return setReturnStatus(resp.updateStatus(), 0, msg.str());
}

//retrieve contents for a node
bool CWsGenesisEx::onGetGSNodeSettings(IEspContext &context, IEspGetGSNodeSettingsRequest &req, IEspGetGSNodeSettingsResponse &resp)
{
    const char* svnPath = NULL;
    try
    {
        StringBuffer fullPath;
        const char* svnPath = req.getSVNPath();
        if (!svnPath || !*svnPath)
            fullPath.appendf("%s%cglobal", m_workdir.str(), m_pathSep);
        else
            fullPath.appendf("%s%c%s", m_workdir.str(), m_pathSep, svnPath);

        StringBuffer userStr, passwordStr, commentStr;
        setUserAndComment(req.getUser(), req.getPassword(), req.getComment(), userStr, passwordStr, commentStr);

        MySVN mySVN(m_svnConfig.str(), userStr.str(), passwordStr.str(), commentStr.str());
        if (svnPath && (strlen(svnPath) > 6) && !strnicmp(svnPath, "logins", 6))
            getLoginSettings(mySVN, fullPath, &resp.updateNodeSettings());
        else
            getNodeSettings(mySVN, svnPath, fullPath.str(), &resp.updateNodeSettings());
    }
    catch (IException *e)
    {
        if (svnPath && *svnPath)
            return setReturnStatus(resp.updateStatus(), e, "GetGSNodeSettings", svnPath);
        else
            return setReturnStatus(resp.updateStatus(), e, "GetGSNodeSettings", NULL);
    }

    StringBuffer msg;
    msg.append("Successfully returns GetGSNodeSettings");
    return setReturnStatus(resp.updateStatus(), 0, msg.str());
}

//update node properties
bool CWsGenesisEx::onUpdateGSNodeProperties(IEspContext &context, IEspUpdateGSNodePropertiesRequest &req, IEspUpdateGSNodePropertiesResponse &resp)
{
    StringBuffer msg;
    const char* svnPath = NULL;
    try
    {
        svnPath = req.getSVNPath();
        if (!svnPath || !*svnPath)
            throw MakeStringException(-1, "Node not specified");

        StringBuffer userStr, passwordStr, commentStr;
        setUserAndComment(req.getUser(), req.getPassword(), req.getComment(), userStr, passwordStr, commentStr);

        MySVN mySVN(m_svnConfig.str(), userStr.str(), passwordStr.str(), commentStr.str());
        resp.setRevision(updateProperties(context, svnPath, userStr.str(), passwordStr.str(), commentStr.str(), &req.getProperties()));
    }
    catch (IException *e)
    {
        return setReturnStatus(resp.updateStatus(), e, "UpdateGSNodeProperties", svnPath);
    }

    msg.append("Successfully returns UpdateGSNodeProperties");
    return setReturnStatus(resp.updateStatus(), 0, msg.str());
}

//add a folder
bool CWsGenesisEx::onAddGSFolder(IEspContext &context, IEspAddGSFolderRequest &req, IEspAddGSFolderResponse &resp)
{
    StringBuffer msg;
    const char* svnPath = NULL;
    try
    {
        svnPath = req.getSVNPath();
        if (!svnPath || !*svnPath)
            throw MakeStringException(-1, "Folder not specified");

        resp.setRevision(addFolder(context, svnPath, req.getUser(), req.getPassword(), req.getComment()));
    }
    catch (IException *e)
    {
        return setReturnStatus(resp.updateStatus(), e, "AddGSFolder", svnPath);
    }

    msg.append("Successfully returns AddGSFolder");
    return setReturnStatus(resp.updateStatus(), 0, msg.str());
}

//remove a folder
bool CWsGenesisEx::onRemoveGSFolder(IEspContext &context, IEspRemoveGSFolderRequest &req, IEspRemoveGSFolderResponse &resp)
{
    StringBuffer msg;
    const char* svnPath = NULL;
    try
    {
        svnPath = req.getSVNPath();
        if (!svnPath || !*svnPath)
            throw MakeStringException(-1, "Folder not specified");

        resp.setRevision(deleteFolder(context, svnPath, req.getUser(), req.getPassword(), req.getComment()));
    }
    catch (IException *e)
    {
        return setReturnStatus(resp.updateStatus(), e, "RemoveGSFolder", svnPath);
    }

    msg.append("Successfully returns RemoveGSFolder");
    return setReturnStatus(resp.updateStatus(), 0, msg.str());
}

//retrieve file content
bool CWsGenesisEx::onGetGSFileContent(IEspContext &context, IEspGetGSFileContentRequest &req, IEspGetGSFileContentResponse &resp)
{
    StringBuffer msg;
    const char* svnPath = NULL;
    try
    {
        svnPath = req.getSVNPath();
        if (!svnPath || !*svnPath)
            throw MakeStringException(-1, "File not specified");
            
        StringBuffer fullPath;
        fullPath.appendf("%s%c%s", m_workdir.str(), m_pathSep, svnPath);

        StringBuffer userStr, passwordStr, commentStr;
        setUserAndComment(req.getUser(), req.getPassword(), req.getComment(), userStr, passwordStr, commentStr);

        MySVN mySVN(m_svnConfig.str(), userStr.str(), passwordStr.str(), commentStr.str());
        
        MemoryBuffer buf;
        eclrepository::SvnInfo info;
        mySVN.svnReadFile(fullPath.str(), NULL, true, -1, info);
        std::string content = info.text;
        if (!content.empty())
            buf.append(content.length(), content.c_str());

        if (!buf.length())
        {
            msg.appendf("File %s is empty,", svnPath);
            return setReturnStatus(resp.updateStatus(), 0, msg.str());
        }
        
        resp.setThefile(buf);
        resp.setThefile_mimetype(HTTP_TYPE_TEXT_PLAIN);
    }
    catch (IException *e)
    {
        return setReturnStatus(resp.updateStatus(), e, "GetGSFileContent", svnPath);
    }

    msg.append("Successfully returns GetGSFileContent");
    return setReturnStatus(resp.updateStatus(), 0, msg.str());
}

//add a file
bool CWsGenesisEx::onAddGSFile(IEspContext &context, IEspAddGSFileRequest &req, IEspAddGSFileResponse &resp)
{
    StringBuffer msg;
    const char* svnPath = NULL;
    try
    {
        svnPath = req.getSVNPath();
        if (!svnPath || !*svnPath)
            throw MakeStringException(-1, "File not specified");

        resp.setRevision(addFile(context, svnPath, req.getContent(), req.getUser(), req.getPassword(), req.getComment()));
    }
    catch (IException *e)
    {
        return setReturnStatus(resp.updateStatus(), e, "AddGSFile", svnPath);
    }

    msg.append("Successfully returns AddGSFile");
    return setReturnStatus(resp.updateStatus(), 0, msg.str());
}

//update file content
bool CWsGenesisEx::onUpdateGSFileContent(IEspContext &context, IEspUpdateGSFileContentRequest &req, IEspUpdateGSFileContentResponse &resp)
{
    StringBuffer msg;
    const char* svnPath = NULL;
    try
    {
        svnPath = req.getSVNPath();
        if (!svnPath || !*svnPath)
            throw MakeStringException(-1, "File name not specified");
        const char* content = req.getContent();
        if (!content || !*content)
            throw MakeStringException(-1, "File content not specified");

        StringBuffer path;
        const char* pName = strrchr(svnPath, '/');           
        if (pName && *pName)
        {
            path.appendf("%s%c", m_workdir.str(), m_pathSep);
            path.append(svnPath, 0, pName - svnPath);
            pName++;
        }
        else
        {
            path.append(m_workdir.str());
            pName = svnPath;
        }

        StringBuffer userStr, passwordStr, commentStr;
        setUserAndComment(req.getUser(), req.getPassword(), req.getComment(), userStr, passwordStr, commentStr);

        MySVN mySVN(m_svnConfig.str(), userStr.str(), passwordStr.str(), commentStr.str());
        resp.setRevision((unsigned)mySVN.svnSave(userStr.str(), path.str(), NULL, pName, commentStr.str(), content, NULL, NULL));
    }
    catch (IException *e)
    {
        return setReturnStatus(resp.updateStatus(), e, "UpdateGSFileContent", svnPath);
    }

    msg.append("Successfully returns UpdateGSFileContent");
    return setReturnStatus(resp.updateStatus(), 0, msg.str());
}

//remove a file
bool CWsGenesisEx::onRemoveGSFile(IEspContext &context, IEspRemoveGSFileRequest &req, IEspRemoveGSFileResponse &resp)
{
    StringBuffer msg;
    const char* svnPath = NULL;
    try
    {
        svnPath = req.getSVNPath();
        if (!svnPath || !*svnPath)
            throw MakeStringException(-1, "File not specified");

        resp.setRevision(deleteFile(context, svnPath, req.getUser(), req.getPassword(), req.getComment()));
    }
    catch (IException *e)
    {
        return setReturnStatus(resp.updateStatus(), e, "RemoveGSFile", svnPath);
    }

    msg.append("Successfully returns RemoveGSFile");
    return setReturnStatus(resp.updateStatus(), 0, msg.str());
}

//add a user
bool CWsGenesisEx::onAddUser(IEspContext &context, IEspAddUserRequest &req, IEspAddUserResponse &resp)
{
    StringBuffer msg;
    const char* name = NULL;
    try
    {
        name = req.getUserName();
        if (!name || !*name)
            throw MakeStringException(-1, "User not specified");

        resp.setRevision(addUser(context, name, req.getUser(), req.getPassword(), req.getComment()));
    }
    catch (IException *e)
    {
        return setReturnStatus(resp.updateStatus(), e, "AddUser", name);
    }

    msg.append("Successfully returns AddUser");
    return setReturnStatus(resp.updateStatus(), 0, msg.str());
}

//remove a user
bool CWsGenesisEx::onRemoveUser(IEspContext &context, IEspRemoveUserRequest &req, IEspRemoveUserResponse &resp)
{
    StringBuffer msg;
    const char* name = NULL;
    try
    {
        name = req.getUserName();
        if (!name || !*name)
            throw MakeStringException(-1, "User not specified");

        resp.setRevision(removeUser(context, name, req.getUser(), req.getPassword(), req.getComment()));
    }
    catch (IException *e)
    {
        return setReturnStatus(resp.updateStatus(), e, "RemoveUser", name);
    }

    msg.append("Successfully returns RemoveUser");
    return setReturnStatus(resp.updateStatus(), 0, msg.str());
}

//add a user group
bool CWsGenesisEx::onAddUserGroup(IEspContext &context, IEspAddUserGroupRequest &req, IEspAddUserGroupResponse &resp)
{
    StringBuffer msg;
    const char* name = NULL;
    try
    {
        name = req.getGroupName();
        if (!name || !*name)
            throw MakeStringException(-1, "UserGroup not specified");

        resp.setRevision(addUser(context, name, req.getUser(), req.getPassword(), req.getComment(), true));
    }
    catch (IException *e)
    {
        return setReturnStatus(resp.updateStatus(), e, "AddUserGroup", name);
    }

    msg.append("Successfully returns AddUserGroup");
    return setReturnStatus(resp.updateStatus(), 0, msg.str());
}

//remove a user group
bool CWsGenesisEx::onRemoveUserGroup(IEspContext &context, IEspRemoveUserGroupRequest &req, IEspRemoveUserGroupResponse &resp)
{
    StringBuffer msg;
    const char* name = NULL;
    try
    {
        name = req.getGroupName();
        if (!name || !*name)
            throw MakeStringException(-1, "UserGroup not specified");

        resp.setRevision(removeUser(context, name, req.getUser(), req.getPassword(), req.getComment(), true));
    }
    catch (IException *e)
    {
        return setReturnStatus(resp.updateStatus(), e, "RemoveUserGroup", name);
    }

    msg.append("Successfully returns RemoveUserGroup");
    return setReturnStatus(resp.updateStatus(), 0, msg.str());
}


bool CWsGenesisEx::onListGSRoots(IEspContext &context, IEspListGSRootsRequest &req, IEspListGSRootsResponse &resp)
{
    StringBuffer msg;
    const char* navPane = NULL;
    try
    {
        navPane = req.getNavPane();
        if (!navPane || !*navPane)
            throw MakeStringException(-1, "Parent node not specified");

        StringBuffer roots;
        IArrayOf<IEspGSNode> nodes;
        if (streq(navPane, "System"))
        {
            StringBuffer nodesJSONString;
            addNodeToArray(nodes, "subnet", "Subnet", "SystemRoot", NULL, true);
            addNodeToArray(nodes, "host", "Host", "SystemRoot", NULL, true);
            buildNodeJsonString2(roots, "SystemRoot", "System", NULL, nodes);
        }
        else if (streq(navPane, "Logins"))
        {
            addNodeToArray(nodes, "user", "User", "LoginsRoot", NULL, true);
            addNodeToArray(nodes, "group", "Group", "LoginsRoot", NULL, true);
            buildNodeJsonString2(roots, "LoginsRoot", "Logins", NULL, nodes);
        }
        else
            throw MakeStringException(-1, "Invalid navigation pane");

        if (roots.length() > 0)
            resp.setRoots(roots.str());
    }
    catch (IException *e)
    {
        return setReturnStatus(resp.updateStatus(), e, "onListGSRoots", navPane);
    }

    return setReturnStatus(resp.updateStatus(), 0, msg.str());
}

//list top folders:
bool CWsGenesisEx::onListGSTopNodes(IEspContext &context, IEspListGSTopNodesRequest &req, IEspListGSTopNodesResponse &resp)
{
    StringBuffer msg;
    try
    {
        StringArray& names = req.getNames();
        if (names.empty())
            return setReturnStatus(resp.updateStatus(), 0, "Node name not specified");

        StringBuffer userStr, passwordStr, commentStr;
        setUserAndComment(req.getUser(), req.getPassword(), req.getComment(), userStr, passwordStr, commentStr);

        IArrayOf<IEspGSFolderNode> folders;

        MySVN mySVN(m_svnConfig.str(), userStr.str(), passwordStr.str(), commentStr.str());
        ForEachItemIn(i, names)
        {
            const char* parentName = names.item(i);
            if (!parentName || !*parentName)
                continue;

            std::string svnPath = m_svn_path_map[parentName];
            if(svnPath.size() == 0)
            {
                DBGLOG("Cannot find svn path for %s", parentName);
                continue;
            }

            Owned<IEspGSFolderNode> folder = createGSFolderNode();
            folder->setID(parentName);

            StringBuffer fullPath;
            fullPath.appendf("%s%c%s", m_workdir.str(), m_pathSep, svnPath.c_str());

            svn::DirEntries dirEntries;
            mySVN.svnList( dirEntries, fullPath.str() );

            IArrayOf<IEspGSFolderNode> subFolders;
            svn::DirEntries::const_iterator it;
            for (it=dirEntries.begin (); it!=dirEntries.end (); it++)
            {
                const svn::DirEntry &dir_entry = (*it);
                const char* name = dir_entry.name();
                if (!name || !*name)
                    continue;

                StringBuffer path;
                path.appendf("%s%c%s", svnPath.c_str(), m_pathSep, name);

                Owned<IEspGSFolderNode> subFolder = createGSFolderNode();
                subFolder->setID(name);
                subFolder->setSVNPath(path.str());
                subFolders.append(*subFolder.getLink());
            }
            if (!subFolders.empty())
                folder->setSubFolderNodes(subFolders);

            folders.append(*folder.getLink());
        }

        if (!folders.empty())
            resp.setTopNodes(folders);
    }
    catch (IException *e)
    {
        return setReturnStatus(resp.updateStatus(), e, "onListGSTopNodes", NULL);
    }

    return setReturnStatus(resp.updateStatus(), 0, msg.str());
}

bool CWsGenesisEx::onAddSubnet(IEspContext &context, IEspAddSubnetRequest &req, IEspAddSubnetResponse &resp)
{
    svn_revnum_t rev;
    const char* subnetName = NULL;
    try
    {
        IConstGSFolder& subnet=req.getSubnet();
        subnetName = subnet.getName();
        if (!subnetName || !*subnetName)
            throw MakeStringException(-1, "Subnet name not specified");

        StringArray propertyNames, propertyValues;
        //collectPropertyRequests(propertyNames, propertyValues, subnet.getProperties());
        //collectSubnetPropertyRequest(propertyNames, propertyValues, req.getDescription(), req.getDNSServer(), req.getKernelConfig(),
        //    req.getNetworkGateway(), req.getNetworkMask(), req.getNetworkNTP(), req.getNetworkPool(), req.getOSImage());

        //addFolder(context, SUBNET, subnetName, req.getComment(), m_user.str(), m_password.str(), propertyNames, propertyValues, rev);

        //TODO: this is test code
        //StringBuffer newPath;
        //newPath.appendf("%s%c%s", SUBNET, m_pathSep, subnetName);
        //addFile(context, newPath.str(), "test_file.txt", "content", comment, m_user.str(), m_password.str(), propertyNames, propertyValues);
    }
    catch (IException *e)
    {
        return setReturnStatus(resp.updateStatus(), e, "AddSubnet", subnetName);
    }

    StringBuffer msg;
    msg.appendf("Successfully added %s; revision number: %d", subnetName, rev);
    return setReturnStatus(resp.updateStatus(), 0, msg.str());
}

bool CWsGenesisEx::onDeleteSubnet(IEspContext &context, IEspDeleteSubnetRequest &req, IEspDeleteSubnetResponse &resp)
{
    svn_revnum_t rev;
    const char* subnetName = NULL;
    try
    {
        subnetName = req.getName();
        if (!subnetName || !*subnetName)
            throw MakeStringException(-1, "Subnet name not specified");

        //deleteFolder(context, SUBNET, subnetName, req.getComment(), m_user.str(), m_password.str(), rev);
    }
    catch (IException *e)
    {
        return setReturnStatus(resp.updateStatus(), e, "DeleteSubnet", subnetName);
    }

    StringBuffer msg;
    msg.appendf("Successfully deleted %s; revision number: %d", subnetName, rev);
    return setReturnStatus(resp.updateStatus(), 0, msg.str());
}

bool CWsGenesisEx::onUpdateSubnet(IEspContext &context, IEspUpdateSubnetRequest &req, IEspUpdateSubnetResponse &resp)
{
    svn_revnum_t rev;
    const char* subnetName = NULL;
    try
    {
        IConstGSFolder& subnet=req.getSubnet();
        subnetName = subnet.getName();
        if (!subnetName || !*subnetName)
            throw MakeStringException(-1, "Subnet name not specified");

        StringArray propertyNames, propertyValues;
        //collectSubnetPropertyRequest(propertyNames, propertyValues, req.getDescription(), req.getDNSServer(), req.getKernelConfig(),
        //    req.getNetworkGateway(), req.getNetworkMask(), req.getNetworkNTP(), req.getNetworkPool(), req.getOSImage());
        //collectPropertyRequests(propertyNames, propertyValues, subnet.getProperties());

        updateProperties(context, SUBNET, subnetName, req.getComment(), m_user.str(), m_password.str(), propertyNames, 
            propertyValues, rev, subnetProperties, numOfSubnetProperties);
    }
    catch (IException *e)
    {
        return setReturnStatus(resp.updateStatus(), e, "UpdateSubnet", subnetName);
    }

    StringBuffer msg;
    msg.appendf("Successfully updated %s; revision number: %d", subnetName, rev);
    return setReturnStatus(resp.updateStatus(), 0, msg.str());
}

bool CWsGenesisEx::onListSubnets(IEspContext &context, IEspListSubnetsRequest &req, IEspListSubnetsResponse &resp)
{
    StringBuffer msg;
    try
    {
        MySVN mySVN(m_svnConfig.str(), m_user.str(), m_password.str(), m_defaultComment.str());

        StringBuffer path;
        svn::DirEntries dirEntries;
        path.appendf("%s%c%s", m_workdir.str(), m_pathSep, SUBNET);
        mySVN.svnList( dirEntries, path.str() );

        IArrayOf<IEspGSFolder> subnets;
        svn::DirEntries::const_iterator it;

        for (it=dirEntries.begin (); it!=dirEntries.end (); it++)
        {
            const svn::DirEntry &dir_entry = (*it);
            const char* name = dir_entry.name();
            if (!name || !*name)
                continue;

            StringBuffer datetime, descStr;
            svn_filesize_t size = dir_entry.size();
            getTime(datetime, dir_entry.time());

            descStr.appendf("Time=%s", datetime.str());
            descStr.appendf(";Size=%s", size);

            if (dir_entry.hasProps())
            {
                StringBuffer path1, value;
                path1.appendf("%s%c%s", path.str(), m_pathSep, name);

                mySVN.svnGetProperty(path1.str(), "prop1", value );
                descStr.appendf("; prop1=%s;", value.str());
                mySVN.svnGetProperty(path1.str(), "prop3", value.clear() );
                if (value.length() > 0)
                    descStr.appendf("; prop1=%s;", value.str());
            }

            Owned<IEspGSFolder> subnet = createGSFolder();
            subnet->setName(name);
            subnet->setDescription(descStr.str()); //FOR now
            subnets.append(*subnet.getLink());
        }

        if (subnets.empty())
            msg.append("Successfully executed ListSubnets. No subnet returns.");
        else
        {
            resp.setSubnets(subnets);
            msg.append("Successfully returns a subnet list.");
        }
    }
    catch (IException *e)
    {
        return setReturnStatus(resp.updateStatus(), e, "ListSubnets", NULL);
    }

    return setReturnStatus(resp.updateStatus(), 0, msg.str());
}

bool CWsGenesisEx::onGetSubnetInfo(IEspContext &context, IEspGetSubnetInfoRequest &req, IEspGetSubnetInfoResponse &resp)
{
    const char* subnetName = NULL;
    try
    {
        subnetName = req.getName();
        if (!subnetName || !*subnetName)
            throw MakeStringException(-1, "Subnet name not specified");

        MySVN mySVN(m_svnConfig.str(), m_user.str(), m_password.str(), m_defaultComment.str());

        StringBuffer path;
        path.appendf("%s%c%s%c%s", m_workdir.str(), m_pathSep, SUBNET, m_pathSep, subnetName);

        StringBuffer propValue;
        IEspGSFolder& subnet = resp.updateSubnet();
        IEspGSSVNInfo& svnInfo = resp.updateSVNInfo();
/*
        svn::PathPropertiesMapList propertyEntries;
        mySVN.svnListProperties(propertyEntries, path.str());
        const svn::PathPropertiesMapEntry &prop_entry = (*propertyEntries.begin ());
        if (readPropertyFromPropertiesMap(prop_entry.second, PropDNSServer, propValue))
            subnet.setDNSServer(propValue.str());
        if (readPropertyFromPropertiesMap(prop_entry.second, PropDescription, propValue.clear()))
            subnet.setDescription(propValue.str());
        if (readPropertyFromPropertiesMap(prop_entry.second, PropKernelConfig, propValue.clear()))
            subnet.setKernelConfig(propValue.str());
        if (readPropertyFromPropertiesMap(prop_entry.second, PropNetworkGateway, propValue.clear()))
            subnet.setNetworkGateway(propValue.str());
        if (readPropertyFromPropertiesMap(prop_entry.second, PropNetworkMask, propValue.clear()))
            subnet.setNetworkMask(propValue.str());
        if (readPropertyFromPropertiesMap(prop_entry.second, PropNetworkNTP, propValue.clear()))
            subnet.setNetworkNTP(propValue.str());
        if (readPropertyFromPropertiesMap(prop_entry.second, PropNetworkPool, propValue.clear()))
            subnet.setNetworkPool(propValue.str());
        if (readPropertyFromPropertiesMap(prop_entry.second, PropOSImage, propValue.clear()))
            subnet.setOSImage(propValue.str());
*/
        svn::InfoVector infoVector;
        mySVN.svnInfo(infoVector, path.str(), 0, false);
        if( infoVector.size() > 0 )
        {
            svn::Info &svninfo = infoVector[0];
            StringBuffer revTime;
            getTime(revTime, svninfo.lastChangedDate());
            const char * revAuthor = svninfo.lastChangedAuthor();
            svnInfo.setLastChangedRev(svninfo.lastChangedRev());
            svnInfo.setLastChangedDate(revTime.str());
            if (revAuthor && *revAuthor)
                svnInfo.setLastChangedAuthor(revAuthor);
        }

        StringBuffer comment;
        mySVN.svnLog( path.str(), comment, -1);
        if (comment.length() > 0)
            svnInfo.setLastComment(comment);
    }
    catch (IException *e)
    {
        return setReturnStatus(resp.updateStatus(), e, "GetSubnetInfo", NULL);
    }

    StringBuffer msg;
    msg.appendf("Successfully returns information about %s", subnetName);
    return setReturnStatus(resp.updateStatus(), 0, msg.str());
}

bool CWsGenesisEx::onAddHost(IEspContext &context, IEspAddHostRequest &req, IEspAddHostResponse &resp)
{
    IConstGSFolder& host=req.getHost();
    const char* hostName = host.getName();
    if (!hostName || !*hostName)
        setReturnStatus(resp.updateStatus(), -1, "Unsuccessfully AddHost: Host name not specified");

    StringBuffer msg;
    msg.append("Successfully added ").append(hostName);
    return setReturnStatus(resp.updateStatus(), 0, msg.str());
}

bool CWsGenesisEx::onDeleteHost(IEspContext &context, IEspDeleteHostRequest &req, IEspDeleteHostResponse &resp)
{
    const char* hostName = req.getName();
    if (!hostName || !*hostName)
        setReturnStatus(resp.updateStatus(), -1, "Unsuccessfully DeleteHost: Host name not specified");

    StringBuffer msg;
    msg.append("Successfully deleted ").append(hostName);
    return setReturnStatus(resp.updateStatus(), 0, msg.str());
}

bool CWsGenesisEx::onUpdateHost(IEspContext &context, IEspUpdateHostRequest &req, IEspUpdateHostResponse &resp)
{
    IConstGSFolder& host=req.getHost();
    const char* hostName = host.getName();
    if (!hostName || !*hostName)
        setReturnStatus(resp.updateStatus(), -1, "Unsuccessfully UpdateHost: Host name not specified");

    StringBuffer msg;
    msg.append("Successfully updated ").append(hostName);
    return setReturnStatus(resp.updateStatus(), 0, msg.str());
}

bool CWsGenesisEx::onListHosts(IEspContext &context, IEspListHostsRequest &req, IEspListHostsResponse &resp)
{
    StringBuffer msg;
    msg.append("Successfully returns a host list.");
    return setReturnStatus(resp.updateStatus(), 0, msg.str());
}

bool CWsGenesisEx::onGetHostInfo(IEspContext &context, IEspGetHostInfoRequest &req, IEspGetHostInfoResponse &resp)
{
    const char* hostName = req.getName();
    if (!hostName || !*hostName)
        setReturnStatus(resp.updateStatus(), -1, "Unsuccessfully GetHostInfo: Host name not specified");

    StringBuffer msg;
    msg.append("Successfully returns information about ").append(hostName);
    return setReturnStatus(resp.updateStatus(), 0, msg.str());
}

bool CWsGenesisEx::onAddCluster(IEspContext &context, IEspAddClusterRequest &req, IEspAddClusterResponse &resp)
{
    const char* clusterName = req.getName();
    if (!clusterName || !*clusterName)
        setReturnStatus(resp.updateStatus(), -1, "Unsuccessfully AddCluster: Cluster name not specified");

    StringBuffer msg;
    msg.append("Successfully added ").append(clusterName);
    return setReturnStatus(resp.updateStatus(), 0, msg.str());
}

bool CWsGenesisEx::onDeleteCluster(IEspContext &context, IEspDeleteClusterRequest &req, IEspDeleteClusterResponse &resp)
{
    const char* clusterName = req.getName();
    if (!clusterName || !*clusterName)
        setReturnStatus(resp.updateStatus(), -1, "Unsuccessfully DeleteCluster: Cluster name not specified");

    StringBuffer msg;
    msg.append("Successfully deleted ").append(clusterName);
    return setReturnStatus(resp.updateStatus(), 0, msg.str());
}

bool CWsGenesisEx::onUpdateCluster(IEspContext &context, IEspUpdateClusterRequest &req, IEspUpdateClusterResponse &resp)
{
    const char* clusterName = req.getName();
    if (!clusterName || !*clusterName)
        setReturnStatus(resp.updateStatus(), -1, "Unsuccessfully UpdateCluster: Cluster name not specified");

    StringBuffer msg;
    msg.append("Successfully updated ").append(clusterName);
    return setReturnStatus(resp.updateStatus(), 0, msg.str());
}

bool CWsGenesisEx::onListClusters(IEspContext &context, IEspListClustersRequest &req, IEspListClustersResponse &resp)
{
    StringBuffer msg;
    msg.append("Successfully returns a cluster list.");
    return setReturnStatus(resp.updateStatus(), 0, msg.str());
}

bool CWsGenesisEx::onGetClusterInfo(IEspContext &context, IEspGetClusterInfoRequest &req, IEspGetClusterInfoResponse &resp)
{
    const char* clusterName = req.getName();
    if (!clusterName || !*clusterName)
        setReturnStatus(resp.updateStatus(), -1, "Unsuccessfully GetClusterInfo: Cluster name not specified");

    StringBuffer msg;
    msg.append("Successfully returns information about ").append(clusterName);
    return setReturnStatus(resp.updateStatus(), 0, msg.str());
}

bool CWsGenesisEx::onAddEnvironment(IEspContext &context, IEspAddEnvironmentRequest &req, IEspAddEnvironmentResponse &resp)
{
    const char* environmentName = req.getName();
    if (!environmentName || !*environmentName)
        setReturnStatus(resp.updateStatus(), -1, "Unsuccessfully AddEnvironment: Environment name not specified");

    StringBuffer msg;
    msg.append("Successfully added ").append(environmentName);
    return setReturnStatus(resp.updateStatus(), 0, msg.str());
}

bool CWsGenesisEx::onDeleteEnvironment(IEspContext &context, IEspDeleteEnvironmentRequest &req, IEspDeleteEnvironmentResponse &resp)
{
    const char* environmentName = req.getName();
    if (!environmentName || !*environmentName)
        setReturnStatus(resp.updateStatus(), -1, "Unsuccessfully DeleteEnvironment: Environment name not specified");

    StringBuffer msg;
    msg.append("Successfully deleted ").append(environmentName);
    return setReturnStatus(resp.updateStatus(), 0, msg.str());
}

bool CWsGenesisEx::onUpdateEnvironment(IEspContext &context, IEspUpdateEnvironmentRequest &req, IEspUpdateEnvironmentResponse &resp)
{
    const char* environmentName = req.getName();
    if (!environmentName || !*environmentName)
        setReturnStatus(resp.updateStatus(), -1, "Unsuccessfully UpdateEnvironment: Environment name not specified");

    StringBuffer msg;
    msg.append("Successfully updated ").append(environmentName);
    return setReturnStatus(resp.updateStatus(), 0, msg.str());
}

bool CWsGenesisEx::onListEnvironments(IEspContext &context, IEspListEnvironmentsRequest &req, IEspListEnvironmentsResponse &resp)
{
    StringBuffer msg;
    msg.append("Successfully returns a environment list.");
    return setReturnStatus(resp.updateStatus(), 0, msg.str());
}

bool CWsGenesisEx::onGetEnvironmentInfo(IEspContext &context, IEspGetEnvironmentInfoRequest &req, IEspGetEnvironmentInfoResponse &resp)
{
    const char* environmentName = req.getName();
    if (!environmentName || !*environmentName)
        setReturnStatus(resp.updateStatus(), -1, "Unsuccessfully GetEnvironmentInfo: Environment name not specified");

    StringBuffer msg;
    msg.append("Successfully returns information about ").append(environmentName);
    return setReturnStatus(resp.updateStatus(), 0, msg.str());
}

bool CWsGenesisEx::onUpdateGlobalSettings(IEspContext &context, IEspUpdateGlobalSettingsRequest &req, IEspUpdateGlobalSettingsResponse &resp)
{
    StringBuffer msg;
    msg.append("Successfully updated GlobalSettings");
    return setReturnStatus(resp.updateStatus(), 0, msg.str());
}

bool CWsGenesisEx::onGetGlobalSettings(IEspContext &context, IEspGetGlobalSettingsRequest &req, IEspGetGlobalSettingsResponse &resp)
{
    try
    {
        StringBuffer fullPath;
        const char* svnPath = req.getSVNPath();
        if (!svnPath || !*svnPath)
            fullPath.appendf("%s%cglobal", m_workdir.str(), m_pathSep);
        else
            fullPath.appendf("%s%c%s", m_workdir.str(), m_pathSep, svnPath);

        MySVN mySVN(m_svnConfig.str(), m_user.str(), m_password.str(), m_defaultComment.str());
        getNodeSettings(mySVN, svnPath, fullPath.str(), &resp.updateGlobalSettings());
    }
    catch (IException *e)
    {
        return setReturnStatus(resp.updateStatus(), e, "GetSubnetInfo", NULL);
    }

    StringBuffer msg;
    msg.append("Successfully returns GlobalSettings");
    return setReturnStatus(resp.updateStatus(), 0, msg.str());
}

int CWsGenesisSoapBindingEx::onGet(CHttpRequest* request, CHttpResponse* response)
{
    Owned<IMultiException> me = MakeMultiException("WsGenesis");

    try
    {
        IEspContext *context = request->queryContext();
        IProperties *parms = request->queryParameters();

        ///TODO  if (!context->validateFeatureAccess(WSECL_ACCESS, SecAccess_Full, false))
        ///    throw MakeStringException(-1, "WsGenesis access permission denied.");

        const char *thepath = request->queryPath();

        StringBuffer serviceName;
        firstPathNode(thepath, serviceName);

        if (stricmp(serviceName.str(), "WsGenesis"))
            return EspHttpBinding::onGet(request, response);

        StringBuffer methodName;
        nextPathNode(thepath, methodName);

        /*if (!stricmp(methodName.str(), "ListAllGSNodes"))
        {
            return onListAllGSNodes(*context, request, response);
        }*/
        if (!stricmp(methodName.str(), "ListGSNodes"))
        {
            return onListGSNodes(*context, request, response);
        }
        if (!stricmp(methodName.str(), "ListGSRoots"))
        {
            return onListGSRoots(*context, request, response);
        }
    }
    catch (IMultiException* mex)
    {
        me->append(*mex);
        mex->Release();
    }
    catch (IException* e)
    {
        me->append(*e);
    }
    catch (...)
    {
        me->append(*MakeStringExceptionDirect(-1, "Unknown Exception"));
    }

    return CWsGenesisSoapBinding::onGet(request,response);
}

int CWsGenesisSoapBindingEx::onListGSRoots(IEspContext &context, CHttpRequest* request, CHttpResponse* response)
{
    IProperties *httpparms=request->queryParameters();
    const char* navPane = httpparms->queryProp("NavPane");
    if (!navPane || !*navPane)
        throw MakeStringException(-1, "Navigation pane not specified");

    StringBuffer nodesJSONString;
    IArrayOf<IEspGSNode> nodes;
    if (streq(navPane, "System"))
    {
        StringBuffer nodesJSONString;
        addNodeToArray(nodes, "subnet", "Subnet", "SystemRoot", NULL, true);
        addNodeToArray(nodes, "host", "Host", "SystemRoot", NULL, true);
        if (!nodes.empty())
            buildNodeJsonString2(nodesJSONString, "SystemRoot", "System", NULL, nodes);
    }
    else if (streq(navPane, "Logins"))
    {
        addNodeToArray(nodes, "user", "User", "LoginsRoot", NULL, true);
        addNodeToArray(nodes, "group", "Group", "LoginsRoot", NULL, true);
        if (!nodes.empty())
            buildNodeJsonString2(nodesJSONString, "LoginsRoot", "Logins", NULL, nodes);
    }
    else
        throw MakeStringException(-1, "Invalid navigation pane: %s", navPane);

    if (nodesJSONString.length() < 1)
        throw MakeStringException(-1, "No data for %s", navPane);

    DBGLOG("JSON sent: <%s>", nodesJSONString.str());
    response->setContent(nodesJSONString.str());
    response->setContentType("application/x-json; charset=utf-8");
    response->setStatus("200 OK");
    response->send();

    return 0;
}

int CWsGenesisSoapBindingEx::onListGSNodes(IEspContext &context, CHttpRequest* request, CHttpResponse* response)
{
    IProperties *httpparms=request->queryParameters();
    const char* parentName = httpparms->queryProp("Name");
    const char* svnPath = httpparms->queryProp("SVNPath");
    if (!parentName || !*parentName)
        throw MakeStringException(-1, "Parent node not specified");

    MySVN mySVN(m_svnConfig.str(), m_user.str(), m_password.str(), m_defaultComment.str());

    StringBuffer relPath, fullPath;
    svn::DirEntries dirEntries;
    if (!svnPath || !*svnPath) //root
        relPath.append(parentName);
    else
        relPath.appendf("%s%c%s", svnPath, m_pathSep, parentName);
    fullPath.appendf("%s%c%s", m_workdir.str(), m_pathSep, relPath.str());
    mySVN.svnList( dirEntries, fullPath.str() );

    IArrayOf<IEspGSNode> nodes;
    svn::DirEntries::const_iterator it;
    for (it=dirEntries.begin (); it!=dirEntries.end (); it++)
    {
        const svn::DirEntry &dir_entry = (*it);
        const char* name = dir_entry.name();
        if (!name || !*name)
            continue;

        bool isLeaf = false;
        svn_node_kind_t nodeKind = dir_entry.kind();
        if (nodeKind != svn_node_dir)
            isLeaf = true;

        Owned<IEspGSNode> node = createGSNode();
        node->setRef(name); //For now, Ref/ID and name are the same
        node->setName(name);
        node->setParent(parentName);
        node->setSVNPath(relPath.str());
        node->setIsLeaf(isLeaf);
        nodes.append(*node.getLink());
    }

    StringBuffer nodesJSONString;
    if (!nodes.empty())
        buildNodeJsonString3(nodesJSONString, parentName, parentName, NULL, nodes);

    if (nodesJSONString.length() < 1)
        throw MakeStringException(-1, "No node data for %s", parentName);

    DBGLOG("JSON sent: <%s>", nodesJSONString.str());
    response->setContent(nodesJSONString.str());
    response->setContentType("application/x-json; charset=utf-8");
    response->setStatus("200 OK");
    response->send();

    return 0;
}

int CWsGenesisSoapBindingEx::onListAllGSNodes(IEspContext &context, CHttpRequest* request, CHttpResponse* response)
{
    IProperties *httpparms=request->queryParameters();
    const char* navPane = httpparms->queryProp("NavPane");
    if (!navPane || !*navPane)
        throw MakeStringException(-1, "Pane not specified");

    const char* navID = "root";
    StringArray folderNames, svnRoots;
    if (streq(navPane, "Pane1"))
    {
        folderNames.append("Subnets");
        folderNames.append("Hosts");
        svnRoots.append("subnet");
        svnRoots.append("host");
    }
    else if (streq(navPane, "Pane2"))
    {
        folderNames.append("Clusters");
        folderNames.append("Environments");
        svnRoots.append("cluster");
        svnRoots.append("environments");
    }
    else if (streq(navPane, "Pane3"))
    {
        folderNames.append("Users");
        folderNames.append("UserGroups");
        svnRoots.append("logins/users/name");
        svnRoots.append("logins/groups/name");
    }
    else
    {
        folderNames.append("Roles");
        svnRoots.append("roles");
    }

    StringBuffer nodesJSONString;
    MySVN mySVN(m_svnConfig.str(), m_user.str(), m_password.str(), m_defaultComment.str());
    ForEachItemIn(i, svnRoots)
    {
        StringBuffer fullPath;
        const char* parentName = folderNames.item(i);
        const char* relPath = svnRoots.item(i);
        fullPath.appendf("%s%c%s", m_workdir.str(), m_pathSep, relPath);

        svn::DirEntries dirEntries;
        mySVN.svnList( dirEntries, fullPath.str() );

        IArrayOf<IEspGSNode> nodes;
        svn::DirEntries::const_iterator it;
        for (it=dirEntries.begin (); it!=dirEntries.end (); it++)
        {
            const svn::DirEntry &dir_entry = (*it);
            const char* name = dir_entry.name();
            if (!name || !*name)
                continue;

            bool isLeaf = false;
            svn_node_kind_t nodeKind = dir_entry.kind();
            if (nodeKind != svn_node_dir)
                isLeaf = true;

            Owned<IEspGSNode> node = createGSNode();
            node->setRef(name); //For now, Ref/ID and name are the same
            node->setName(name);
            node->setParent(parentName);
            node->setSVNPath(relPath);
            node->setIsLeaf(isLeaf);
            nodes.append(*node.getLink());
        }

        if (!nodes.empty())
            buildNodeJsonString4(nodesJSONString, parentName, parentName, navID, "", nodes);
    }

    if (nodesJSONString.length() < 1)
        throw MakeStringException(-1, "No node data for %s", navPane);

    addRootNodeToJSONString(nodesJSONString, navID, navPane);

    DBGLOG("JSON sent: <%s>", nodesJSONString.str());
    response->setContent(nodesJSONString.str());
    response->setContentType("application/x-json; charset=utf-8");
    response->setStatus("200 OK");
    response->send();

    return 0;
}
