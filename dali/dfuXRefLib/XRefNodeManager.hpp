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

// XRefNodeManager.h: interface for the CXRefNodeManager class.
//
//////////////////////////////////////////////////////////////////////
#ifndef XREFNODEMANAGER_HPP
#define XREFNODEMANAGER_HPP

#include "jstring.hpp"
#include "dasds.hpp"


#ifdef DFUXREFLIB_EXPORTS
    #define DFUXREFNODEMANAGERLIB_API DECL_EXPORT
#else
    #define DFUXREFNODEMANAGERLIB_API DECL_IMPORT
#endif


#include "XRefFilesNode.hpp"

enum DFUXRefSortField
{
    DFUXRefSFpartmask = 1,
    DFUXRefSFsize = 2,
    DFUXRefSFsizeto = 3,
    DFUXRefSFmodified = 4,
    DFUXRefSFmodifiedto = 5,
    DFUXRefSFparts = 6,
    DFUXRefSFpartsto = 7,
    /*DFUXRefSFtotalparts = 6,
    DFUXRefSFpartsfound = 7,
    DFUXRefSFpartslost = 8,
    DFUXRefSFprimarylost = 9,
    DFUXRefSFreplicatedlost = 10,
    DFUXRefSFfiles = 3,
    DFUXRefSFdirectory = 11,
    DFUXRefSFtotalsize = 12,
    DFUXRefSFmaxsize = 13,
    DFUXRefSFminsize = 14,
    DFUXRefSFmaxnode = 15,
    DFUXRefSFminnode = 16,
    DFUXRefSFskewplus = 17,
    DFUXRefSFskewminus = 18,
    DFUXRefSFmessage = 19,
    DFUXRefSFstatus = 20,
    DFUXRefSFfile = 21,*/
    DFUXRefSFterm = 0,
    DFUXRefSFreverse = 256,
    DFUXRefSFnocase = 512,
    DFUXRefSFnumeric = 1024,
    DFUXRefSFwild = 2048
};

typedef IIteratorOf<IPropertyTree> IDFUXRefItemIterator;

class CDFUXRefItemIterator: public CInterfaceOf<IDFUXRefItemIterator>
{
    IArrayOf<IPropertyTree> attrs;
    unsigned index;
public:
    CDFUXRefItemIterator(IArrayOf<IPropertyTree> &trees)
    {
        ForEachItemIn(t, trees)
            attrs.append(*LINK(&trees.item(t)));
        index = 0;
    }

    virtual ~CDFUXRefItemIterator()
    {
        attrs.kill();
    }

    bool  first()
    {
        index = 0;
        return (attrs.ordinality()!=0);
    }

    bool  next()
    {
        index++;
        return (index<attrs.ordinality());
    }

    bool  isValid()
    {
        return (index<attrs.ordinality());
    }

    IPropertyTree &  query()
    {
        return attrs.item(index);
    }
};

interface IXRefProgressCallback: extends IInterface
{
    virtual void progress(const char *text)=0;
    virtual void error(const char *text)=0;
};



interface IConstXRefNode : extends IInterface
{
    virtual StringBuffer & getName(StringBuffer & str) = 0;
    virtual StringBuffer & getLastModified(StringBuffer & str)   = 0;
    virtual StringBuffer& getXRefData(StringBuffer & buff)   = 0;
    virtual StringBuffer& getStatus(StringBuffer & buff) = 0;
    virtual IXRefFilesNode* getLostFiles() = 0;
    virtual IXRefFilesNode* getFoundFiles() = 0;
    virtual IXRefFilesNode* getOrphanFiles() = 0;
    virtual IDFUXRefItemIterator* getDFUXRefItemsSorted(const char* type,
        DFUXRefSortField *sortOrder, // list of fields to sort by (terminated by DFUXRefSFterm)
        DFUXRefSortField *filters,   // NULL or list of fields to filter on (terminated by DFUXRefSFterm)
        const void *filterBuf,  // (appended) string values for filters
        const unsigned pageStartFrom,
        const unsigned pageSize,
        unsigned *total,
        __int64 *cacheHint) = 0;
    virtual StringBuffer& getCluster(StringBuffer& Cluster) = 0;
    virtual const char *queryRootDir() const = 0;
    virtual bool useSasha() = 0;
};

interface IXRefNode :  extends IConstXRefNode
{
    virtual void setName(const char* str) = 0;
    virtual void BuildXRefData(IPropertyTree & pTree,const char* Cluster) = 0;
    virtual void setStatus(const char* str) = 0;
    virtual void commit() = 0;
    virtual void setCluster(const char* str) = 0;
    virtual StringBuffer &serializeMessages(StringBuffer &buf) = 0;
    virtual void deserializeMessages(IPropertyTree& inTree) = 0;
    virtual StringBuffer &serializeDirectories(StringBuffer &buf) = 0;
    virtual void deserializeDirectories(IPropertyTree& inTree) = 0;
    virtual bool removeEmptyDirectories(StringBuffer &errstr) = 0;
};

interface IXRefNodeManager :  extends IInterface
{
    virtual IXRefNode * getXRefNode(const char* NodeName) = 0;
    virtual IXRefNode * CreateXRefNode(const char* NodeName) = 0;
};

extern DFUXREFNODEMANAGERLIB_API IXRefNodeManager * CreateXRefNodeFactory();

class CXRefNode : implements IXRefNode ,implements IXRefProgressCallback, public CInterface
{
private:
    Owned<IRemoteConnection> m_conn;
    StringBuffer m_origName;
    Owned<IPropertyTree> m_XRefTree;
    Owned<IPropertyTree> m_XRefDataTree;
    Owned<IXRefFilesNode> m_orphans;
    Owned<IXRefFilesNode> m_lost;
    Owned<IXRefFilesNode> m_found;
    Owned<IPropertyTree> m_messages;
    Owned<IPropertyTree> m_directories;
    bool m_bChanged;
    StringBuffer m_dataStr;
    StringBuffer m_ClusterName;
    Mutex commitMutex;
    StringAttr rootDir;

private:
    IPropertyTree& getDataTree();

public:
    IMPLEMENT_IINTERFACE;
    CXRefNode(); 
    CXRefNode(IPropertyTree* pTreeRoot); 
    CXRefNode(const char* clusterName,IRemoteConnection *conn); 
    virtual ~CXRefNode();
    bool IsChanged();
    void SetChanged(bool bChanged);
    void setLastModified(IJlibDateTime& DateTime);

    //IXRefProgressCallback
    void progress(const char *text);
    void error(const char *text);

    //IConstXRefNode
    virtual StringBuffer & getName(StringBuffer & str) override;
    virtual StringBuffer & getLastModified(StringBuffer & str) override;
    virtual bool useSasha() override;
    virtual const char *queryRootDir() const override { return rootDir; }
    
    virtual StringBuffer& getXRefData(StringBuffer & buff) override;
    virtual StringBuffer& getStatus(StringBuffer & buff) override;
    virtual IXRefFilesNode* getLostFiles() override;
    virtual IXRefFilesNode* getFoundFiles() override;
    virtual IXRefFilesNode* getOrphanFiles() override;
    virtual IDFUXRefItemIterator* getDFUXRefItemsSorted(const char* type,
        DFUXRefSortField* sortOrder, // list of fields to sort by (terminated by DFUXRefSFterm)
        DFUXRefSortField* filters,   // NULL or list of fields to filter on (terminated by DFUXRefSFterm)
        const void* filterBuf,  // (appended) string values for filters
        const unsigned pageStartFrom,
        const unsigned pageSize,
        unsigned* total,
        __int64* cacheHint);
    //IXRefNode
    virtual StringBuffer &serializeMessages(StringBuffer &buf) override;
    virtual void deserializeMessages(IPropertyTree& inTree) override;
    virtual StringBuffer &serializeDirectories(StringBuffer &buf) override;
    virtual void deserializeDirectories(IPropertyTree& inTree) override;
    virtual bool removeEmptyDirectories(StringBuffer &errstr) override;
    virtual void setName(const char* str) override;
    
    void setXRefData(StringBuffer & buff);
    void setXRefData(IPropertyTree & pTree);

    void BuildXRefData(IPropertyTree & pTree,const char* Cluster);

    void setStatus(const char* str);
    void commit();

    virtual StringBuffer& getCluster(StringBuffer& Cluster);
    virtual void setCluster(const char* str);


};

class CXRefNodeManager : implements IXRefNodeManager, public CInterface
{
public:
    IMPLEMENT_IINTERFACE;
    CXRefNodeManager(){};
    virtual ~CXRefNodeManager(){};
    IXRefNode * getXRefNode(const char* NodeName);
    IXRefNode * CreateXRefNode(const char* NodeName);
};



#endif // 
