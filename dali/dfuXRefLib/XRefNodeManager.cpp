/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2012 HPCC Systems®.

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

// XRefNodeManager.cpp: implementation of the CXRefNodeManager class.
//
//////////////////////////////////////////////////////////////////////

#include "jiface.hpp"
#include "jstring.hpp"
#include "jptree.hpp"
#include "jmisc.hpp"

#include "mpcomm.hpp"
#include "platform.h"
#include "jlib.hpp"
#include "mpbase.hpp"
#include "daclient.hpp"
#include "dadiags.hpp"
#include "danqs.hpp"
#include "dadfs.hpp"
#include "dasds.hpp"
#include "dautils.hpp"
#include "daft.hpp"
#include "rmtfile.hpp"

#include "XRefNodeManager.hpp"



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IXRefNodeManager * CreateXRefNodeFactory()
{
    return new CXRefNodeManager();
}

EnumMapping DFUXRefSortFields[] =
{
        //TODO: add more from DFUXRefSortField
   { DFUXRefSFpartmask, "Partmask" },
   { DFUXRefSFsize, "Size" },
   { DFUXRefSFsizeto, "Size" },
   { DFUXRefSFmodified, "Modified" },
   { DFUXRefSFmodifiedto, "Modified" },
   { DFUXRefSFparts, "Numparts" },
   { DFUXRefSFpartsto, "Numparts" },
   { DFUXRefSFterm, NULL }
};

IXRefNode * CXRefNodeManager::getXRefNode(const char* NodeName)
{
    //DBGLOG("Node Name %s:",NodeName);

    StringBuffer xpath;
    xpath.appendf("Cluster[@name=\"%s\"]", NodeName);
    //if not exists, add DFU/XREF/ClusterName branch to SDS
    //not linked
    Owned<IRemoteConnection> conn = querySDS().connect("/DFU/XREF",myProcessSession(),RTM_CREATE_QUERY|RTM_NONE ,INFINITE);
    IPropertyTree* cluster_ptree = conn->queryRoot()->queryPropTree(xpath.str());
    conn->commit();
    if (cluster_ptree==0)
    {
        DBGLOG("Cluster[@name=%s] can't be found under /DFU/XREF", NodeName);
        return 0 ;
    }
    return new CXRefNode(NodeName,conn);

}

IXRefNode * CXRefNodeManager::CreateXRefNode(const char* NodeName)
{
    Owned<IRemoteConnection> conn = querySDS().connect("/DFU/XREF",myProcessSession(),RTM_CREATE_QUERY|RTM_LOCK_WRITE ,INFINITE);
    IPropertyTree* xref_ptree = conn->queryRoot();
    IPropertyTree* cluster_ptree = xref_ptree->addPropTree("Cluster", createPTree());
    cluster_ptree->setProp("@name",NodeName);
    conn->commit();
    conn->changeMode(RTM_NONE);
    return new CXRefNode(NodeName,conn);
}



CXRefNode::CXRefNode()
{
    m_bChanged = false;
}

CXRefNode::~CXRefNode()
{
}

CXRefNode::CXRefNode(const char* NodeName, IRemoteConnection *_conn)
{
    //DBGLOG("CXRefNode::CXRefNode(const char* NodeName)");

    m_bChanged = false;
    m_conn.set(_conn);

    StringBuffer xpath;
    xpath.appendf("Cluster[@name=\"%s\"]", NodeName);

    IPropertyTree* cluster_ptree = m_conn->queryRoot()->queryPropTree(xpath.str());
    m_XRefTree.set(cluster_ptree);
    m_XRefTree->getProp("@name",m_origName);
    rootDir.set(m_XRefTree->queryProp("@rootdir"));
    //DBGLOG("returning from CXRefNode::CXRefNode(const char* NodeName)");

}

CXRefNode::CXRefNode(IPropertyTree* pTreeRoot)
{
    m_bChanged = false;
    try
    {
        m_XRefTree.set(pTreeRoot);
        rootDir.set(m_XRefTree->queryProp("@rootdir"));
        pTreeRoot->getProp("@name",m_origName);
        //load up our tree with the data.....if there is data
        MemoryBuffer buff;
        pTreeRoot->getPropBin("data",buff);
        if (buff.length())
        {
            m_dataStr.append(buff.length(),buff.toByteArray());
        }
        //lets check to ensure we have the correct children inplace(Orphan,lost,found)
    }
    catch(...)
    {
        ERRLOG("Error in creation of XRefNode...");
    }
}

bool CXRefNode::useSasha()
{
    if (!m_conn)
        return false;
    return m_conn->queryRoot()->getPropBool("@useSasha");
}


IPropertyTree& CXRefNode::getDataTree()
{
    if(m_XRefDataTree.get() == 0)
        m_XRefDataTree.setown(createPTreeFromXMLString(m_dataStr.str()));

    return *m_XRefDataTree.get();

}


    //IConstXRefNode
StringBuffer & CXRefNode::getName(StringBuffer & str)
{
    if(m_XRefTree.get())
        m_XRefTree->getProp("@name",str);
    return str;
}

StringBuffer& CXRefNode::getStatus(StringBuffer & str)
{
    if(m_XRefTree.get())
        m_XRefTree->getProp("@status",str);
    return str;
}


StringBuffer & CXRefNode::getLastModified(StringBuffer & str)
{
    if(m_XRefTree.get())
        m_XRefTree->getProp("@modified",str);
    return str;
}


StringBuffer& CXRefNode::getXRefData(StringBuffer & str)
{
    return toXML(&getDataTree(),str);
}


IXRefFilesNode* CXRefNode::getLostFiles()
{
    if(!m_lost.get())
    {
        IPropertyTree* lostBranch = m_XRefTree->queryPropTree("Lost");
        if(lostBranch == 0)
        {
            lostBranch = m_XRefTree->addPropTree("Lost",createPTree());
            commit();
        }
        StringBuffer tmpbuf;
        m_lost.setown(new CXRefFilesNode(*lostBranch,getName(tmpbuf).str(),rootDir));
    }
    return m_lost.getLink();
}

IXRefFilesNode* CXRefNode::getFoundFiles()
{
    if(!m_found.get())
    {
        IPropertyTree* foundBranch = m_XRefTree->queryPropTree("Found");
        if(foundBranch == 0)
        {
            foundBranch = m_XRefTree->addPropTree("Found",createPTree());
            commit();
        }
        StringBuffer tmpbuf;
        m_found.setown(new CXRefFilesNode(*foundBranch,getName(tmpbuf).str(),rootDir));
    }
    return m_found.getLink();
}

IXRefFilesNode* CXRefNode::getOrphanFiles()
{
    if(!m_orphans.get())
    {
        IPropertyTree* orphanBranch = m_XRefTree->queryPropTree("Orphans");
        if(orphanBranch == 0)
        {
            orphanBranch = m_XRefTree->addPropTree("Orphans",createPTree());
            commit();
        }
        StringBuffer tmpbuf;
        m_orphans.setown(new CXRefOrphanFilesNode(*orphanBranch,getName(tmpbuf).str(),rootDir));
    }
    return m_orphans.getLink();
}

StringBuffer &CXRefNode::serializeMessages(StringBuffer &buf)
{
    if(!m_messages.get())
    {
        IPropertyTree* messagesBranch = m_XRefTree->queryPropTree("Messages");
        if(messagesBranch == 0)
        {
            messagesBranch = m_XRefTree->addPropTree("Messages",createPTree());
            commit();
        }
        StringBuffer tmpbuf;
        m_messages.set(messagesBranch);
    }
    buf.clear();
    MemoryBuffer data;
    m_messages->getPropBin("data",data);
    if (data.length())
    {
        buf.append(data.length(),data.toByteArray());
    }
    return buf;
}

void CXRefNode::deserializeMessages(IPropertyTree& inTree)
{
    if(!m_messages.get())
    {
        IPropertyTree* messagesBranch = m_XRefTree->queryPropTree("Messages");
        if(messagesBranch == 0)
        {
            messagesBranch = m_XRefTree->addPropTree("Messages",createPTree());
            commit();
        }
        StringBuffer tmpbuf;
        m_messages.set(messagesBranch);
    }
    StringBuffer datastr;
    toXML(&inTree,datastr);
    m_messages->setPropBin("data",datastr.length(),(void*)datastr.str());
}

StringBuffer &CXRefNode::serializeDirectories(StringBuffer &buf)
{
    if(!m_directories.get())
    {
        IPropertyTree* directoriesBranch = m_XRefTree->queryPropTree("Directories");
        if(directoriesBranch == 0)
        {
            directoriesBranch = m_XRefTree->addPropTree("Directories",createPTree());
            commit();
        }
        StringBuffer tmpbuf;
        m_directories.set(directoriesBranch);
    }
    buf.clear();
    MemoryBuffer data;
    m_directories->getPropBin("data",data);
    if (data.length())
    {
        buf.append(data.length(),data.toByteArray());
    }
    return buf;
}

void CXRefNode::deserializeDirectories(IPropertyTree& inTree)
{
    if(!m_directories.get())
    {
        IPropertyTree* directoriesBranch = m_XRefTree->queryPropTree("Directories");
        if(directoriesBranch == 0)
        {
            directoriesBranch = m_XRefTree->addPropTree("Directories",createPTree());
            commit();
        }
        StringBuffer tmpbuf;
        m_directories.set(directoriesBranch);
    }
    StringBuffer datastr;
    toXML(&inTree,datastr);
    m_directories->setPropBin("data",datastr.length(),(void*)datastr.str());


}

static bool deleteEmptyDir(IFile *dir)
{
    // this is a bit odd - basically we already know no files but there may be empty sub-dirs
    Owned<IDirectoryIterator> iter = dir->directoryFiles(NULL,false,true);
    bool candelete = true;
    ForEach(*iter)
    {
        if (iter->isDir())
        {
            Owned<IFile> file = &iter->get();
            try
            {
                if (!deleteEmptyDir(file))
                    candelete = false;
            }
            catch (IException *e)
            {
                EXCLOG(e,"deleteEmptyDir");
                candelete = false;
            }
        }
        else
            candelete = false;
    }
    if (!candelete)
        return false;
    static CriticalSection sect;
    CriticalBlock block(sect);      // don't want to actually remove in parallel
    dir->remove();
    return !dir->exists();
}

static bool recursiveCheckEmptyScope(IPropertyTree &ct)
{
    Owned<IPropertyTreeIterator> iter = ct.getElements("*");
    ForEach(*iter) {
        IPropertyTree &item = iter->query();
        const char *n = item.queryName();
        if (!n||(strcmp(n,queryDfsXmlBranchName(DXB_Scope))!=0))
            return false;
        if (!recursiveCheckEmptyScope(item))
            return false;
    }
    return true;
}



static void emptyScopes()
{
    PROGLOG("Removing empty scopes");
    Owned<IDFScopeIterator> iter = queryDistributedFileDirectory().getScopeIterator(UNKNOWN_USER,NULL,true,true);//MORE:Pass IUserDescriptor
    CDfsLogicalFileName dlfn;
    StringBuffer s;
    StringArray toremove;
    ForEach(*iter) {
        CDfsLogicalFileName dlfn;
        StringBuffer scope;
        scope.append(iter->query());
        dlfn.set(scope.str(),"x");
        dlfn.makeScopeQuery(s.clear(),true);
        Owned<IRemoteConnection> conn = querySDS().connect(s.str(),myProcessSession(),RTM_LOCK_READ, INFINITE);
        if (!conn)  
            DBGLOG("Could not connect to '%s' using %s",iter->query(),s.str());
        else {
            if (recursiveCheckEmptyScope(*conn->queryRoot())) {
                PROGLOG("Empty scope %s",iter->query());
                toremove.append(iter->query());
            }
        }
    }
    iter.clear();
    ForEachItemIn(i,toremove) {
        PROGLOG("Removed scope %s",toremove.item(i));
        queryDistributedFileDirectory().removeEmptyScope(toremove.item(i));
    }
}



bool CXRefNode::removeEmptyDirectories(StringBuffer &errstr)
{
    StringBuffer dataStr;
    serializeDirectories(dataStr);
    if (dataStr.length()==0)
        return true;
    Owned<IPropertyTree> t = createPTreeFromXMLString(dataStr.str());
    Owned<IPropertyTreeIterator> iter = t->getElements("Directory");
    const char *clustername = t->queryProp("Cluster");
    if (!clustername||!*clustername)
        return false;
    Owned<IGroup> group = queryNamedGroupStore().lookup(clustername);
    if (!group) {
        ERRLOG("%s cluster not found",clustername);
        errstr.appendf("ERROR: %s cluster not found",clustername);
        return false;
    }
    StringArray dellist;
    PointerArray todelete;
    ForEach(*iter) {
        IPropertyTree &dir = iter->query();
        if (dir.getPropInt64("Num")==0) {
            const char *dirname = dir.queryProp("Name");
            if (dirname&&*dirname) {
                dellist.append(dirname);
                todelete.append(&dir);
            }
        }
    }
    dellist.sortAsciiReverse(false);
    ForEachItemIn(di,dellist) {
        const char *dirname = dellist.item(di);
        class casyncfor: public CAsyncFor
        {
            IGroup *grp;
            StringAttr name;

        public:
            casyncfor(IGroup *_grp,const char *_name)
                : name(_name)
            {
                grp = _grp;
            }
            void Do(unsigned i)
            {
                RemoteFilename rfn;
                rfn.setPath(grp->queryNode(i).endpoint(),name);
                StringBuffer eps;
                try
                {
                    Owned<IFile> dir = createIFile(rfn);
                    if (deleteEmptyDir(dir))
                        PROGLOG("Removed '%s'",dir->queryFilename());
                    else
                        WARNLOG("Could not remove '%s'",dir->queryFilename());
                }
                catch (IException *e)
                {
                    EXCLOG(e,"Could not remove directory");
                    e->Release();
                }
            }
        } afor(group,dirname);
        afor.For(group->ordinality(),10,false,true);
    }
    iter.clear();
    ForEachItemInRev(i,todelete) 
        t->removeTree((IPropertyTree *)todelete.item(i)); // probably should check succeeded above but next run will correct
    toXML(t,dataStr.clear());
    m_directories->setPropBin("data",dataStr.length(),(void*)dataStr.str());
    emptyScopes();
    return true;
}




    //IXRefNode
void CXRefNode::setName(const char* str)
{
    m_XRefTree->setProp("@name",str);
    if (m_origName.length() == 0)
        m_origName.append(str);
}

void CXRefNode::setStatus(const char* str)
{
    m_XRefTree->setProp("@status",str);
}


StringBuffer& CXRefNode::getCluster(StringBuffer& Cluster)
{
    Cluster.append(m_ClusterName.str());
    return Cluster;
}

void CXRefNode::setCluster(const char* str)
{
    m_ClusterName.clear();
    m_ClusterName.append(str);
}

void CXRefNode::setLastModified(IJlibDateTime& dt )
{
    SCMStringBuffer datestr,timestr;
    dt.getDateString(datestr);
    dt.getTimeString(timestr);
    StringBuffer tmpstr(datestr.str());
    tmpstr.append(" ");
    tmpstr.append(timestr.str());
    m_XRefTree->setProp("@modified",tmpstr.str());
}

void CXRefNode::BuildXRefData(IPropertyTree & pTree,const char* Cluster)
{

//  DBGLOG("CXRefNode::BuildXRefData");
    if(m_XRefTree.get() == 0)
        throw MakeStringException(-1, "No XRef Dali Tree available");

    Owned<IXRefFilesNode> lost = getLostFiles();
    Owned<IXRefFilesNode> found = getFoundFiles() ;
    Owned<IXRefFilesNode> orphan = getOrphanFiles();

    IPropertyTree* pSubTree = pTree.queryPropTree("Orphans");
    pSubTree->setProp("Cluster",Cluster);
    orphan->Deserialize(*pSubTree);

    pSubTree = pTree.queryPropTree("Lost");
    pSubTree->setProp("Cluster",Cluster);
    lost->Deserialize(*pSubTree);

    pSubTree = pTree.queryPropTree("Found");
    pSubTree->setProp("Cluster",Cluster);
    found->Deserialize(*pSubTree);

    pSubTree = pTree.queryPropTree("Messages");
    pSubTree->setProp("Cluster",Cluster);
    deserializeMessages(*pSubTree);

    pSubTree = pTree.queryPropTree("Directories");
    pSubTree->setProp("Cluster",Cluster);
    deserializeDirectories(*pSubTree);

    Owned<IJlibDateTime> dt =  createDateTimeNow();
    setLastModified(*dt);
    setStatus("Generated");
    commit();

}


bool CXRefNode::IsChanged()
{
    if ((m_orphans.get() && m_orphans->IsChanged() == true) ||
        (m_lost.get() && m_lost->IsChanged() == true) ||
        (m_found.get() && m_found->IsChanged() == true) ||
         m_bChanged == true )
         return true;
    return false;
}

void CXRefNode::SetChanged(bool bChanged)
{
    m_bChanged = bChanged;
}


void CXRefNode::commit()
{
    
    CriticalSection(commitMutex);
    if(m_conn == 0)
        return;
    Owned<IXRefFilesNode> lost = getLostFiles();
    Owned<IXRefFilesNode> found = getFoundFiles() ;
    Owned<IXRefFilesNode> orphan = getOrphanFiles();
    lost->Commit();
    found->Commit();
    orphan->Commit();
    m_conn->commit();

}

void CXRefNode::progress(const char *text)
{
    DBGLOG("PROGRESS: %s\n",text);
    setStatus(text);
    commit();
}
void CXRefNode::error(const char *text)
{
    DBGLOG("ERROR: %s\n",text);
    setStatus(text);
    commit();
}

IDFUXRefItemIterator *CXRefNode::getDFUXRefItemsSorted(const char *type,
    DFUXRefSortField *sortOrder, // list of fields to sort by (terminated by DFUXRefSFterm)
    DFUXRefSortField *filters,   // NULL or list of fields to filter on (terminated by DFUXRefSFterm)
    const void *filterBuf,  // (appended) string values for filters
    const unsigned pageStartFrom,
    const unsigned pageSize,
    unsigned *total,
    __int64 *cacheHint)
{
    class CDFUXRefElementsPager : public CSimpleInterface, implements IElementsPager
    {
        IXRefNode* xrefNode;
        StringAttr xrefBranch, sortOrder, filter;
        IPropertyTree* xrefTree;
    public:
        IMPLEMENT_IINTERFACE_USING(CSimpleInterface);

        CDFUXRefElementsPager(IXRefNode* _xrefNode, IPropertyTree* _xrefTree, const char *_xrefBranch, const char *_filter, const char *_sortOrder)
            : xrefNode(_xrefNode), xrefTree(_xrefTree), xrefBranch(_xrefBranch), filter(_filter), sortOrder(_sortOrder)
        {
        }
        virtual IRemoteConnection* getElements(IArrayOf<IPropertyTree> &elements)
        {
            StringArray unknownAttributes;
            //Not sure what the commit() does. Copied from
            IPropertyTree* foundBranch = xrefTree->queryPropTree(xrefBranch.get());
            if (!foundBranch)
            {
                foundBranch = xrefTree->addPropTree(xrefBranch.get(), createPTree());
                xrefNode->commit();
            }

            StringBuffer xml;
            MemoryBuffer data;
            foundBranch->getPropBin("data", data);
            xml.append(data.length(), data.toByteArray());
            //DBGLOG("******(%s)", xml.str());

            StringBuffer xpath;
            if (filter.isEmpty())
                xpath.set("*");
            else
                xpath.set("File").append(filter.get());
            Owned<IPropertyTree> dataTree = createPTreeFromXMLString(xml);
            Owned<IPropertyTreeIterator> iter = dataTree->getElements(xpath.str());
            if (!iter)
                return NULL;

            sortElements(iter, sortOrder.get(), NULL, NULL, unknownAttributes, elements);
            return NULL;
        }
        virtual bool allMatchingElementsReceived() { return true; } //For now, always returns all of matched items.
    };

    StringBuffer so, filterXPath;
    if (filters)
    {
        const char *fv = (const char *)filterBuf;
        for (unsigned i=0; filters[i] != DFUXRefSFterm; i++)
        {
            int fmt = filters[i];
            int subfmt = (fmt&0xff);
            if ((subfmt==DFUXRefSFsize) || (subfmt==DFUXRefSFparts))
                filterXPath.append('[').append(getEnumText(subfmt, DFUXRefSortFields)).append(">=").append(fv).append("]");
            else if ((subfmt==DFUXRefSFsizeto) || (subfmt==DFUXRefSFpartsto))
                filterXPath.append('[').append(getEnumText(subfmt, DFUXRefSortFields)).append("<=").append(fv).append("]");
            else if (subfmt==DFUXRefSFmodified)
                filterXPath.append('[').append(getEnumText(subfmt, DFUXRefSortFields)).append(">='").append(fv).append("']");
            else if (subfmt==DFUXRefSFmodifiedto)
                filterXPath.append('[').append(getEnumText(subfmt, DFUXRefSortFields)).append("<='").append(fv).append("']");
            else
            {
                filterXPath.append('[').append(getEnumText(subfmt, DFUXRefSortFields)).append('=');
                if (fmt&DFUXRefSFnocase)
                    filterXPath.append('?');
                if (fmt&DFUXRefSFnumeric)
                    filterXPath.append('#');
                if (fmt&DFUXRefSFwild)
                    filterXPath.append('~');
                filterXPath.append('"').append(fv).append("\"]");
            }
            fv = fv + strlen(fv)+1;
        }
    }
    if (sortOrder)
    {
        for (unsigned i=0; sortOrder[i] != DFUXRefSFterm; i++)
        {
            if (so.length())
                so.append(',');
            int fmt = sortOrder[i];
            if (fmt&DFUXRefSFreverse)
                so.append('-');
            if (fmt&DFUXRefSFnocase)
                so.append('?');
            if (fmt&DFUXRefSFnumeric)
                so.append('#');
            so.append(getEnumText(fmt&0xff, DFUXRefSortFields));
        }
    }
    IArrayOf<IPropertyTree> results;
    Owned<IElementsPager> elementsPager = new CDFUXRefElementsPager(this, m_XRefTree, type, filterXPath.str(), so.length()?so.str():NULL);
    Owned<IRemoteConnection> conn=getElementsPaged(elementsPager, pageStartFrom, pageSize, NULL, "", cacheHint, results, total, NULL, false);
    return new CDFUXRefItemIterator(results);
}
