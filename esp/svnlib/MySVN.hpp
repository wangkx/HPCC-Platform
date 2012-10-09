#ifndef __MySVN__
#define __MySVN__

//#include "UserProfile.hpp"
#include <iostream> 
#include <jstring.hpp>
#include <jtime.hpp>

#include "svn_types.h"

#include "svncpp/client.hpp"
#include "svncpp/info.hpp"
#include "svncpp/dirent.hpp"

#include "SVNObjects.hpp"

#define DEFAULT_COMMENT "ECL:"

#define ATTR_EXTENSION ".eclattr"

namespace svn
{
    class Context;
    class Client;

};
    class SVNContextListener;
//namespace eclrepository
//{

class MySVN 
{
public:                 
    
    MySVN(const char* svn_config, const char* user, const char* password, const char* commentStr);

    virtual ~MySVN();  

    /**
    void setContext(svn::Context& context) 
    {
        m_svn_context = &context;
        m_svn_client = new svn::Client( m_svn_context );
    }**/

    svn::Client* getUserSvnContext( const char* user, const char* password="password" );

    svn_revnum_t svnCheckout( const char* srcURL, const char* destDir);

    svn_revnum_t svnCommit( 
        const char* path, 
        const char* commitMessage,
        const char* copyFromPath=NULL, 
        const char* username=NULL,
        const char* password=NULL);

    void svnStatus( const char* dir );

    svn_revnum_t  svnUpdate( const char* path, const svn_revnum_t toRev=0 );

    svn_revnum_t  svnSwitch(const char* src,  
                   const char* toUrl,
                   const svn_revnum_t toRev = 0);

    void svnCleanup(const char* dir );

    void svnLock(   const char* path, const char* user);
    void svnUnlock( const char* path, const char* user);

    void svnAdd(const char* = NULL);
    void svnInfo( svn::InfoVector& infoVector, const char* dir, const svn_revnum_t revnum=0, bool recurse = false);

    void svnMkdir(const char* path, const char* user);
    void svnList(svn::DirEntries &dirEntriesVector, const char* dir, bool recurse = false);
    void svnRemove( const char* path );
    void svnMove( const char* oldPath,  const char* newPath, const char* user )  ;

    void svnMerge( 
        const char* path1, 
        const char* path2, 
        const char* target,
        const svn_revnum_t revision1=0,
        const svn_revnum_t revision2=0);
        //const svn::Revision & revision1 = svn::Revision::HEAD,
        //const svn::Revision & revision2 = svn::Revision::HEAD);

    void svnLog( const char* path, StringBuffer& comment, int version);
    void testSvnLog( const char* path, int version, StringArray& logArray);

    const svn::LogEntries* //delete me
         svnLog( const char* path );

    void svnCopy(const char* src,  
                 const char* dest,const svn_revnum_t toRev=0 );
                 //svn::Revision revision = svn::Revision::HEAD );

    void svnListProperties(svn::PathPropertiesMapList &propList, const char* path);

    void svnSetProperty(
        const char* path,
        const char* name,
        const char* value);

    void svnSetProperty(
        const svn::Path &path,
        const char* name,  
        const char* value);

    void svnResetProperty(
        const char* path,
        const char* name,
        const char* value);

    void svnResetProperty(
        const svn::Path &path,
        const char* name,
        const char* value);

    void svnUnsetProperty(
        const char* path,
        const char* name);

    void svnUnsetProperty(
        const svn::Path &path,
        const char* name);

    void svnGetProperty(
        const char* path,
        const char* name,
        StringBuffer& value,
        svn_revnum_t revision = 0);

    void svnSetRevProperty(
        const char* path,
        const char* name,
        const char* value,
        svn_revnum_t revision = 0);

    void svnSetRevProperty(
        const svn::Path &path,
        const char* name,
        const char* value,
        svn_revnum_t revision = 0);

    void svnUnsetRevProperty(
        const char* path,
        const char* name,
        svn_revnum_t revision = 0);

    void svnSetLogin(const char* user, const char* password)
    {
        m_svn_context->setLogin( user, password );
    }
    void svnSetLogMessage(const char* msg)
    {
        m_svn_context->setLogMessage( msg );
    }

    void svnReadFile( const char* filename, const char* url, bool includeText, int version, eclrepository::SvnInfo &info);

	 bool svnReadFile( const char* dir, const char* url, const char* filename, bool includeText, int version, 
                   eclrepository::SvnInfo *info);

    void svnReadFiles( const char* filename, const char* url, bool includeText, int version, eclrepository::SvnInfoVector &infoVector);
    int  svnSave( 
           const char* user,
           const char* path, 
           const char* url, 
           const char* filename, 
           const char* comment,
           const char* text, 
           const char* componentType, 
           const char* resultType);

    void findInFilesUsingEgrep(    
        const char* workDir,
        const char* branchURL,
        StringArray& typeList,
        const char* username,
        const char* pattern,
        const char* regexp,
        bool        includeText,
        bool        includeMatchedLines,
        eclrepository::SvnInfoVector& infoVector);

    void FindInFiles(                                 
        const char* username,
        const char* workDir,
        const char* branchURL,
        StringArray& typeList,        
        const char* pattern,
        const char* regexp,
        bool        includeText,
        bool        includeMatchedLines,
        eclrepository::SvnInfoVector& infoVector) ;
    //bool onFindInFiles(IEspContext &context, IEspFindInFilesRequest &req, IEspFindInFilesResponse &resp);
protected:
       
    //disable copy constructor
    MySVN( MySVN& obj);

    //disable asignment operator
    MySVN& operator=( const MySVN& lhs );
    
    StringBuffer        m_svn_config;
    svn::Context*       m_svn_context;
    svn::Client*        m_svn_client;
    SVNContextListener* m_contextListener;
    StringBuffer	    m_commentStr;
};

//};
#endif
