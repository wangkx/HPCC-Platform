#pragma warning (disable : 4355)
#pragma warning (disable : 4786)

#include <iostream>
#include <string.h>

#include <jexcept.hpp>
#include <jlog.hpp>
#include <jfile.hpp>
#include <jmutex.hpp>
#include <jthread.hpp>
#include <jregexp.hpp>
#include <jmisc.hpp>

#include <list>

#include "MySVN.hpp"

//svncpp
#include "svncpp/context.hpp"
#include "svncpp/client.hpp"
#include "svncpp/path.hpp"
#include "svncpp/entry.hpp"
#include "svncpp/log_entry.hpp"
#include "svncpp/revision.hpp"
#include "svncpp/status.hpp"
#include "svncpp/targets.hpp"
#include "svncpp/info.hpp"
#include "svncpp/dirent.hpp"

#include "SVNContextListener.hpp"

// md5
#include "md5wrapper.h"
//
//namespace eclrepository
//{


#define COMPONENT_TYPE  "ComponentType"  //this maps to 'flags', it is COMPONENT_TYPE in mysql
//#define RESULT_TYPE     "ResultType"
//#define LOCKED_BY_PROP  "LockedBy"
#define NUM_SVN_RETRIES  (10)

MySVN::MySVN(const char* svn_config, const char* user, const char* password, const char* commentStr) : m_svn_client(NULL)
{
//    m_svn_context     = new svn::Context("./svn_config");
    m_svn_config.clear().append(svn_config);
    m_svn_context     = new svn::Context(m_svn_config.str());
    m_contextListener = new SVNContextListener( m_svn_context );
    m_svn_context->setListener( m_contextListener);

    m_svn_context->setLogin( user, password );
    if (commentStr && *commentStr)
        m_commentStr = commentStr;
    else
        m_commentStr = "default log message" ;

    m_svn_context->setLogin( user, password );
    m_svn_context->setLogMessage( m_commentStr.str() );

    m_svn_client = new svn::Client( m_svn_context );
}

MySVN::~MySVN()
{
    if( m_svn_client )
        delete m_svn_client;
}

svn::Client* MySVN::getUserSvnContext( const char* user, const char* password )
{
    //StringBuffer userDir("./svn_config_");
    StringBuffer userDir;
    userDir.appendf("%s_%s", m_svn_config.str(), user );

    //warning... potential leaks
    svn::Context* my_svn_context           = new svn::Context( userDir.str() );    
    SVNContextListener* my_contextListener = new SVNContextListener( my_svn_context );

    my_svn_context->setListener( my_contextListener);
    //my_svn_context->setLogin( user, password );

    StringBuffer userLogMsg( m_commentStr );

    my_svn_context->setLogMessage( userLogMsg.str() );

    svn::Client* my_svn_client = new svn::Client( my_svn_context );

    return my_svn_client;
}
  
 
//
// SVN routines...
//
svn_revnum_t MySVN::svnCheckout( const char* srcURL, const char* destDir)
{
    if( m_svn_client == NULL )
        throw "SVN Client is null...";

    try {
        svn::Path path( destDir ); 
        svn_revnum_t revNumber = m_svn_client->checkout (
            srcURL,   
            path,
            svn::Revision::HEAD,
            true,
            true);   
        return revNumber;
    }
    catch(svn::ClientException& ce)
    {
        DBGLOG(0,"ClientException at: %s:%d, msg=<%s>", __FILE__,__LINE__,ce.message() );
        throw MakeStringException(0, ce.message() );
    }    
}
//
//
//
svn_revnum_t MySVN::svnCommit( const char* path,
        const char* commitMessage, 
        const char* copyFromPath,       //=null
        const char* username,           //=null
        const char* password)           //=null
{

//DBGLOG("svnCommit() path=<%s>,\nmessage=<%s>,\ncopyFromPath=<%s>,\nusername=<%s>,\npw=<%s>",
//path, commitMessage, copyFromPath, username, password);

    if( m_svn_client == NULL )
        throw MakeStringException(0,"SVN Client is null...");   

    if( commitMessage==NULL || strlen(commitMessage)==0  )
        throw MakeStringException(0,"Commit Message not set");
    
    if( path==NULL || strlen(path)==0  )
        throw MakeStringException(0,"Path not set");

    //check commit msg for non printables...put this somewhere else??
    char buffer[256];
    memset( buffer,0,256);
    strncpy( buffer, commitMessage, 255);

    int end = strlen(buffer);
    unsigned char * foo = (unsigned char*) buffer;

    for( int i(0); i < end; i++, foo++)
    {
        if( (*foo > 0) && (*foo < 0x20) )  //replace non printable ascii chars with a space (' ')
	    if( (*foo != 0x0a) && (*foo != 0x0d) )
        {   
            *foo = 0x20;
        }
    }

    if( copyFromPath && strlen(copyFromPath) )
    {
        Owned<IFile> dest = createIFile( path );
        Owned<IFile> src  = createIFile( copyFromPath );
        src->copyTo( dest );
    }

    StringBuffer CommitMessage( buffer );
    int tries = 0;
    svn_revnum_t revNumber(0);
    while( tries < NUM_SVN_RETRIES )
    {
		  svn::Client* user_svn_client = NULL;

        try 
        {        
            svn::Path    svnPath( path ); 
            svn::Targets targets; 
            svn::Revision rev;
            targets.push_back( svnPath );
                        
            user_svn_client = getUserSvnContext( username, password );

            revNumber = user_svn_client->commit (
                targets,
                CommitMessage.str(),
                true //recurse
                ); 
              
            delete user_svn_client;
				user_svn_client = NULL;
          
            if( revNumber <=0 )
            {
                DBGLOG(0,"commit rev number : %d", revNumber );
                throw MakeStringException(0, "Commit of <%s> Failed, invalid revison number returned.", path );
            }

            /***
            //change author from 'esp' to current user...
            if( username && strlen(username) )
            {   
                if( strstr(path,"file:///") )    
                {
                    DBGLOG("svnCommit(): setting svn:author...");
                    svnSetProperty(  path, "svn:author",  username, revNumber );           
                }
            }
            ***/
            break;
        }
        catch(svn::ClientException& ce)
        {
			   if (user_svn_client)
					delete user_svn_client;

            DBGLOG(0,"ClientException at: %s:%d, msg=<%s>", __FILE__,__LINE__,ce.message() );
            tries++;
            if( tries >= NUM_SVN_RETRIES ) {
                throw MakeStringException(0, ce.message() );
            }
            sleep(5);
        } 
    }
    return revNumber;
}
//
//
//
void MySVN::svnStatus( const char* dir )
{
    if( m_svn_client == NULL )
        throw "SVN Client is null...";

    try {
        // svn status...
        svn::StatusEntries entries;
        entries = m_svn_client->status ( dir, true); //TODO, add path arg
        svn::StatusEntries::const_iterator it;
        for (it=entries.begin (); it!=entries.end (); it++)
        {
            svn::Status status (*it);
            if( status.entry().isValid () )
            {
                std::cout << status.textStatus() << "\t\t" <<  status.path() << std::endl;
            }
            else
            {
                std::cout << "\t\t(not in repository)" <<  status.path() << std::endl;
            }
        }
    }
    catch(svn::ClientException& ce)
    {
        DBGLOG(0,"ClientException at: %s:%d, msg=<%s>", __FILE__,__LINE__,ce.message() );
        throw MakeStringException(0, ce.message() );
    } 
}
//
//
//
void MySVN::svnCleanup( const char* dir )
{
    if( m_svn_client == NULL )
        throw "SVN Client is null...";

    try {
        svn::Path path( dir); 
        // svn cleanup
        m_svn_client->cleanup( path );                
    }
    catch(svn::ClientException& ce)
    {
        DBGLOG(0,"ClientException at: %s:%d, msg=<%s>", __FILE__,__LINE__,ce.message() );
        throw MakeStringException(0, ce.message() );
    } 
}
//
//
//
svn_revnum_t MySVN::svnUpdate( const char* dir, const svn_revnum_t toRev )
{
    if( m_svn_client == NULL )
        throw "SVN Client is null...";

    try {
        svn::Path path( dir ); 
        ///svn::Revision rev;
        ///if( toRev > 0 )
        ///    rev.set( toRev );
        ///else
        ///    rev = svn::Revision::HEAD;

        // svn update
        svn_revnum_t revNumber;
        if( toRev <= 0 )
        {
            revNumber = m_svn_client->update(path, svn::Revision::HEAD,
                true, //recurse
                true);//ignore_externals
        }
        else
        {
            svn::Revision rev(toRev);
            revNumber = m_svn_client->update(path, rev, true, true);
        }
        return revNumber;               
    }
    catch(svn::ClientException& ce)
    {        
        DBGLOG(0,"ClientException at: %s:%d, msg=<%s>", __FILE__,__LINE__,ce.message() );
        throw MakeStringException(0, ce.message() );
    } 
}
//
//
//
void MySVN::svnAdd(const char* filename)
{
    if( m_svn_client == NULL )
        throw "SVN Client is null...";

    int tries = 0;
    while( tries < NUM_SVN_RETRIES )
    {
        try {
            //check if file exists...?
            svn::Path path( filename ); 
            // svn add 
            m_svn_client->add(
                path,
                true,  //recurse
                true   //force  'svn add --force'
                );   
            break;           
        }
        catch(svn::ClientException& ce)
        {
            DBGLOG(0,"ClientException at: %s:%d, msg=<%s>", __FILE__,__LINE__,ce.message() );            
            tries++;
            if( tries >= NUM_SVN_RETRIES ) {
                throw MakeStringException(0, ce.message() );
            }
            sleep(3);
        } 
    }
}
//
//
//
void MySVN::svnInfo( svn::InfoVector& infoVector,
                 const char* dir,         
                 const svn_revnum_t revnum, //=0
                 bool recurse               //= false
                 )
{
    if( m_svn_client == NULL )
        throw "SVN Client is null...";

    try {
        svn::Path path( dir ); 
        ///svn::Revision rev;
        ///if( revnum > 0 )
        ///    rev.set( revnum );
        ///else
        ///    rev = svn::Revision::HEAD;

        if( revnum <= 0 )
            infoVector = m_svn_client->info(path, recurse, svn::Revision::HEAD);
        else
        {
            svn::Revision rev(revnum);
            infoVector = m_svn_client->info(path, recurse, rev);
        }
    }
    catch(svn::ClientException& ce)
    {
        DBGLOG(0,"ClientException at: %s:%d, msg=<%s>", __FILE__,__LINE__,ce.message() );
        throw MakeStringException(0, ce.message() );
    } 
}
//
//
//
void
MySVN::svnList(svn::DirEntries &dirEntriesVector, 
                 const char* dir, 
                 bool recurse // =false
                 )
{
    if( m_svn_client == NULL )
        throw "SVN Client is null...";

    if( dir==NULL || strlen(dir)==0  )
        throw MakeStringException(0,"dir not set");

    try {
        svn_opt_revision_t revision;
        revision.kind = svn_opt_revision_head;

        dirEntriesVector = m_svn_client->list(
            dir,
            &revision,
            recurse
            );  
    }
    catch(svn::ClientException& ce)
    {
        DBGLOG(0,"ClientException at: %s:%d, msg=<%s>", __FILE__,__LINE__,ce.message() );
        //throw MakeStringException(0, ce.message() );
    } 
}
//
//
//
void MySVN::svnLog( const char* path, StringBuffer& comment, int version)
{
    if( m_svn_client == NULL )
        throw "SVN Client is null...";

    if( path==NULL || strlen(path)==0  )
        throw MakeStringException(0,"Path not set");

    const svn::LogEntries *logEntries_p = NULL;
    try {
        if( version <= 0 )
            logEntries_p = m_svn_client->log(path, svn::Revision::BASE, svn::Revision::HEAD, true, false);   
        else
        {
            svn::Revision start(version), end(version);
            logEntries_p = m_svn_client->log(path, start, end, true, false);
        }
    }
    catch(svn::ClientException& ce)
    {
        DBGLOG(0,"ClientException at: %s:%d, msg=<%s>", __FILE__,__LINE__,ce.message() );
        throw MakeStringException(0, ce.message() );
    } 
    if( logEntries_p->size() )
    {
        const svn::LogEntry &loge = (*logEntries_p)[0];    
        comment   = loge.message.c_str();
    }
    delete logEntries_p;
}
//
//
//
void MySVN::testSvnLog( const char* path, int version, StringArray& logArray)
{
    if( m_svn_client == NULL )
        throw "SVN Client is null...";

    if( path==NULL || strlen(path)==0  )
        throw MakeStringException(0,"Path not set");

    const svn::LogEntries *logEntries_p = NULL;
    try {
        svn::Revision end(1);
        if( version <= 0 )
        {
            logEntries_p = m_svn_client->log(path, end, svn::Revision::HEAD, true, false);
        }
        else
        {   svn::Revision start(version);
            logEntries_p = m_svn_client->log(path, end, start, true, false);
        }
    }
    catch(svn::ClientException& ce)
    {
        DBGLOG(0,"ClientException at: %s:%d, msg=<%s>", __FILE__,__LINE__,ce.message() );        
        throw MakeStringException(0, ce.message() );
    } 
    for( unsigned i(0); i < logEntries_p->size(); i++)
    {
        const svn::LogEntry &loge = (*logEntries_p)[i];  

        DBGLOG("testSvnLog(): comment=<%s>",  loge.message.c_str() );

        std::list<svn::LogChangePathEntry>::const_iterator it;

        for ( it=loge.changedPaths.begin(); 
              it != loge.changedPaths.end(); 
              it++ )
        {
            DBGLOG("testSvnLog(): path =<%s>",  it->path.c_str() ); 
        }
    }
    delete logEntries_p;
}

const svn::LogEntries * //delete me
MySVN::svnLog( const char* path )
{
    if( m_svn_client == NULL )
        throw "SVN Client is null...";

    if( path==NULL || strlen(path)==0  )
        throw MakeStringException(0,"Path not set");

    const svn::LogEntries *logEntries_p = NULL;
    try {
        logEntries_p = m_svn_client->log(
            path,
            svn::Revision::START,          
            svn::Revision::HEAD,
            true, false
            );       
    }
    catch(svn::ClientException& ce)
    {
        DBGLOG(0,"ClientException at: %s:%d, msg=<%s>", __FILE__,__LINE__,ce.message() );
        throw MakeStringException(0, ce.message() );
    } 
    return logEntries_p;
}
//
//
//
void
MySVN::svnLock( const char* path, const char* user)              
{
     DBGLOG(0,"svnLock: path=<%s>,user==<%s>", path,user );

    if( m_svn_client == NULL )
        throw MakeStringException(0,"SVN Client is null...");

    if( path==NULL || strlen(path)==0 )
        throw MakeStringException(0,"path is null...");

    if( user==NULL || strlen(user)==0 )
        throw MakeStringException(0,"user is null...");

	 svn::Client* user_svn_client = NULL;

    try {

        /*** dad 10/30/08
        StringBuffer lockedBy;
        svnGetProperty(path, LOCKED_BY_PROP, lockedBy);
         
        if( lockedBy.length() && (strcmp(lockedBy.str(), user)!= 0) ) 
        {
            throw MakeStringException(0,"%s already locked by <%s>",path,lockedBy.str() );
        }
        else if( lockedBy.length() && (strcmp(lockedBy.str(), user) == 0) ) 
        {
            //already locked by this user, nothing to do            
        }
        else 
        ***/
        {
            /***  dad 10/29/08
            svn::InfoVector infoVector;
            StringBuffer lockedBy;
            svnInfo( infoVector, path );
            if( infoVector.size() )
            {
                svn::Info &info = infoVector[0];
                lockedBy = info.lockedBy();
            }  
            else {
                throw MakeStringException(0,"svnInfo(%s) failed",path);
            } 

            if( strlen(lockedBy.str()) && strcmp(lockedBy.str(),user)!=0 )
            {
                throw MakeStringException(0,"%s is locked by: %s",path,lockedBy.str());
            }***/

            //not locked, lock it...

            svn::Client* user_svn_client = getUserSvnContext( user );

            //m_svn_client->lock(
            user_svn_client->lock(
                path,
                false,
                "svnLock()"
                );  
            delete user_svn_client;

            //svnSetProperty( path, LOCKED_BY_PROP,  user);
        }
    }
    catch(svn::ClientException& ce)
    {
		  if (user_svn_client)
			  delete user_svn_client;

        DBGLOG(0,"ClientException at: %s:%d, msg=<%s>", __FILE__,__LINE__,ce.message() );
        throw MakeStringException(0, ce.message() );
    } 
}
//
//
//
void
MySVN::svnUnlock( const char* path, const char* user)
{
    if( m_svn_client == NULL )
        throw MakeStringException(0,"SVN Client is null...");

    if( path==NULL || strlen(path)==0 )
        throw MakeStringException(0,"path is null...");

    if( user==NULL || strlen(user)==0 )
        throw MakeStringException(0,"user is null...");

	 svn::Client* user_svn_client = NULL;

    try {
        /***
        StringBuffer lockedBy;
        svnGetProperty(path, LOCKED_BY_PROP, lockedBy);
        
        if( lockedBy.length() && (strcmp(lockedBy.str(), user)!= 0) ) {
            throw MakeStringException(0,"%s locked by <%s>",path,lockedBy.str() );
        }   
        ***/

        /***
        svn::InfoVector infoVector;
        svnInfo( infoVector, path );
        if( infoVector.size() )
        {
            svn::Info &info = infoVector[0];
            lockedBy = info.lockedBy();
        }  
        else {
            throw MakeStringException(0,"svnInfo(%s) failed",path);
        }

        if( strlen(lockedBy.str()) && strcmp(lockedBy.str(),user)!=0 )
        {
            throw MakeStringException(0,"%s is locked by: %s",path,lockedBy.str());
        } ***/


        svn::Client* user_svn_client = getUserSvnContext( user );

        user_svn_client->unlock( path, false);   
        delete user_svn_client;
        
        //svnUnsetProperty(path, LOCKED_BY_PROP);
    }
    catch(svn::ClientException& ce)
    {
		  if (user_svn_client)
			  delete user_svn_client;

        DBGLOG(0,"ClientException at: %s:%d, msg=<%s>", __FILE__,__LINE__,ce.message() );        
        throw MakeStringException(0, ce.message() );
    } 
}
//
//
//
void
MySVN::svnMkdir( const char* path, const char* user )              
{    
    if( m_svn_client == NULL )
        throw MakeStringException(0,"SVN Client is null...");

    if( path==NULL || strlen(path)==0 )
        throw MakeStringException(0,"path is null...");

    /* Owned<IFile> ifile = createIFile( path );
    if( !ifile->isDirectory() )
    {
    */
        try {
            svn::Path svnPath( path );
            m_svn_client->mkdir(svnPath);             
        }
        catch(svn::ClientException& ce)
        {
            DBGLOG(0,"ClientException at: %s:%d, msg=<%s>", __FILE__,__LINE__,ce.message() );
            //throw MakeStringException(0, ce.message() );
        } 
    /* 
    }
    else
    {
        throw MakeStringException(0, "svnMkdir(): invalid path <%s>", path);
    }*/
}
//
//
//
void
MySVN::svnMerge( 
        const char* path1, 
        const char* path2, 
        const char* target,
        const svn_revnum_t revision1,
        const svn_revnum_t revision2)
        //const svn::Revision & revision1,  //=svn::Revision::HEAD
        //const svn::Revision & revision2)  //=svn::Revision::HEAD
{
    throw MakeStringException(0, "Not implemented" );
}
//
//
//
void
MySVN::svnRemove( const char* path )              
{
    if( m_svn_client == NULL )
        throw MakeStringException(0,"SVN Client is null...");

    if( path==NULL || strlen(path)==0 )
        throw MakeStringException(0,"path is null...");

    try {        
        svn::Path svnPath( path ); 
        m_svn_client->remove (
            svnPath,
            true );  
    }
    catch(svn::ClientException& ce)
    {
        DBGLOG(0,"ClientException at: %s:%d, msg=<%s>", __FILE__,__LINE__,ce.message() );
        throw MakeStringException(0, ce.message() );
    } 
}

//
//
//
void
MySVN::svnMove( const char* oldPath,  const char* newPath, const char* user )              
{
    if( m_svn_client == NULL )
        throw MakeStringException(0,"SVN Client is null...");

    if( oldPath==NULL || strlen(oldPath)==0 )
        throw MakeStringException(0,"oldPath is null...");

    if( newPath==NULL || strlen(newPath)==0 )
        throw MakeStringException(0,"newPath is null...");

    svn::Client* user_svn_client=NULL;

    try {
        svn::Path old( oldPath ); 
        svn::Path neW( newPath );
            
        if( user && strlen(user) )
            user_svn_client = getUserSvnContext( user );

        user_svn_client->move (
            old,
            svn::Revision::HEAD,
            neW,
            false);  
       
        if( user_svn_client )
            delete user_svn_client;

    }
    catch(svn::ClientException& ce)
    {
        if( user_svn_client )
            delete user_svn_client;

        DBGLOG(0,"ClientException at: %s:%d, msg=<%s>", __FILE__,__LINE__,ce.message() );
        throw MakeStringException(0, ce.message() );
    } 
}
//
//
//
svn_revnum_t 
MySVN::svnSwitch(const char* src,  
                 const char* toUrl,
                 const svn_revnum_t toRev
                 )
{
    if( m_svn_client == NULL )
        throw "SVN Client is null...";

    try {
        svn::Path path( src );
        
        svn_revnum_t switchedRev;
        if( toRev <= 0 )
            switchedRev =  m_svn_client->doSwitch (path, toUrl, svn::Revision::HEAD, false);
        else
        {
            svn::Revision rev( toRev );
            switchedRev =  m_svn_client->doSwitch (path, toUrl, rev, false);
        }
        return switchedRev;       
    }
    catch(svn::ClientException& ce)
    {
        DBGLOG(0,"ClientException at: %s:%d, msg=<%s>", __FILE__,__LINE__,ce.message() );
        throw MakeStringException(0, ce.message() );
    }
}
//
//
//
void 
MySVN::svnCopy(const char* src,  
                 const char* dest,
                 const svn_revnum_t toRev) //=svn::Revision::HEAD
{
    if( m_svn_client == NULL )
        throw "SVN Client is null...";
    try
    {
        if( toRev <= 0 )
            m_svn_client->copy (src, svn::Revision::HEAD, dest);
        else
        {
            svn::Revision rev( toRev );
            m_svn_client->copy (src, rev, dest);
        }
    }
    catch(svn::ClientException& ce)
    {
        DBGLOG(0,"ClientException at: %s:%d, msg=<%s>", __FILE__,__LINE__,ce.message() );
        throw MakeStringException(0, ce.message() );
    }    
}
//
//
//
void 
MySVN::svnListProperties(svn::PathPropertiesMapList &propList, const char* path)
{
    if( m_svn_client == NULL )
        throw MakeStringException(0,"SVN Client is null...");

    if( path==NULL || strlen(path)==0 )
        throw MakeStringException(0,"path is null...");

    try
    {
        svn::Path svnPath( path );
        propList = m_svn_client->proplist (path, svn::Revision::HEAD, false);
    }
    catch(svn::ClientException& ce)
    {
        DBGLOG(0,"ClientException at: %s:%d, msg=<%s>", __FILE__,__LINE__,ce.message() );
        throw MakeStringException(0, ce.message() );
    }
}
//
//
//
void
MySVN::svnSetProperty( const char* path,const char* name, const char* value)
{
    if( m_svn_client == NULL )
        throw MakeStringException(0,"SVN Client is null...");

    if( path==NULL || strlen(path)==0 )
        throw MakeStringException(0,"path is null...");
    
    if( name==NULL || strlen(name)==0 )
        throw MakeStringException(0,"name is null...");
    
    if( value==NULL || strlen(value)==0 )
        throw MakeStringException(0,"value is null...");

    svn::Path svnPath( path );
    svnSetProperty( svnPath, name, value );
}
//
//
//
void
MySVN::svnSetProperty(
        const svn::Path &path,
        const char* name,  
        const char* value)
{
    if( m_svn_client == NULL )
        throw MakeStringException(0,"SVN Client is null...");

    if( name==NULL || strlen(name)==0 )
        throw MakeStringException(0,"name is null...");

    if( value==NULL || strlen(value)==0 )
        throw MakeStringException(0,"value is null...");

    try {
        m_svn_client->propset (name, value, path, svn::Revision::HEAD, false, true);
    }
    catch(svn::ClientException& ce)
    {
        DBGLOG(0,"ClientException at: %s:%d, msg=<%s>", __FILE__,__LINE__,ce.message() );
        throw MakeStringException(0, ce.message() );
    }
}
//
//
//
void
MySVN::svnResetProperty(//This function does not work!!
        const char* path,
        const char* name,
        const char* value)
{
    if( m_svn_client == NULL )
        throw MakeStringException(0,"SVN Client is null...");

    if( path==NULL || strlen(path)==0 )
        throw MakeStringException(0,"path is null...");
    
    if( name==NULL || strlen(name)==0 )
        throw MakeStringException(0,"name is null...");

    if( value==NULL || strlen(value)==0 )
        throw MakeStringException(0,"value is null...");

    svn::Path svnPath( path );
    svnResetProperty( svnPath, name, value );
}
//
//
//
void
MySVN::svnResetProperty(  //This function does not work!!
        const svn::Path &path,
        const char* name,
        const char* value)
{
    if( m_svn_client == NULL )
        throw MakeStringException(0,"SVN Client is null...");

    if( name==NULL || strlen(name)==0 )
        throw MakeStringException(0,"name is null...");

    if( value==NULL || strlen(value)==0 )
        throw MakeStringException(0,"value is null...");

    try
    {
        m_svn_client->propdel (name, path, svn::Revision::HEAD, false);
        m_svn_client->propset (name, value, path, svn::Revision::HEAD, false, true);
    }
    catch(svn::ClientException& ce)
    {
        DBGLOG(0,"ClientException at: %s:%d, msg=<%s>", __FILE__,__LINE__,ce.message() );
        throw MakeStringException(0, ce.message() );
    }
}
//
//
//
void
MySVN::svnUnsetProperty(
        const char* path,
        const char* name)
{
    if( m_svn_client == NULL )
        throw MakeStringException(0,"SVN Client is null...");

    if( path==NULL || strlen(path)==0 )
        throw MakeStringException(0,"path is null...");

    if( name==NULL || strlen(name)==0 )
        throw MakeStringException(0,"name is null...");

    svn::Path svnPath( path );
    svnUnsetProperty( svnPath, name);
}
//
//
//
void
MySVN::svnUnsetProperty(
        const svn::Path &path,
        const char* name)
{
    if( m_svn_client == NULL )
        throw MakeStringException(0,"SVN Client is null...");

    if( name==NULL || strlen(name)==0 )
        throw MakeStringException(0,"name is null...");

    try
    {
        m_svn_client->propdel (name, path, svn::Revision::HEAD, false);
    }
    catch(svn::ClientException& ce)
    {
        DBGLOG(0,"ClientException at: %s:%d, msg=<%s>", __FILE__,__LINE__,ce.message() );
        throw MakeStringException(0, ce.message() );
    }
}

//
//
//
void
MySVN::svnSetRevProperty( const char* path,const char* name, const char* value,
        svn_revnum_t revision)
{
    if( m_svn_client == NULL )
        throw MakeStringException(0,"SVN Client is null...");

    if( path==NULL || strlen(path)==0 )
        throw MakeStringException(0,"path is null...");

    if( name==NULL || strlen(name)==0 )
        throw MakeStringException(0,"name is null...");

    if( value==NULL || strlen(value)==0 )
        throw MakeStringException(0,"value is null...");

    svn::Path svnPath( path );
    svnSetRevProperty( svnPath, name, value, revision );
}
//
//
//
void
MySVN::svnSetRevProperty(
        const svn::Path &path,
        const char* name,
        const char* value,
        svn_revnum_t revision)
{
    if( m_svn_client == NULL )
        throw MakeStringException(0,"SVN Client is null...");

    if( name==NULL || strlen(name)==0 )
        throw MakeStringException(0,"name is null...");

    if( value==NULL || strlen(value)==0 )
        throw MakeStringException(0,"value is null...");

    try
    {
        if( strstr( name, "svn:") )
        {
#ifndef WIN32
            long revision = rev.revnum();
            StringBuffer propSetCommand("svn propset ");
            propSetCommand.append( name ).append(" --revprop -r");
            propSetCommand.appendlong( revision );
            propSetCommand.append( " " ).append( value );
            propSetCommand.append( " " ).append( path.c_str() );

            //bool 
            DWORD runCode;
            //invoke_program( propSetCommand.str(), runCode, true);
            invoke_program( propSetCommand.str(), runCode, false);

            int status;
            pid_t pid=1;
            while( pid > 0 )
            {
                pid = waitpid(-1, &status, WNOHANG);
            } 
#else
            if ( revision <= 0 )
                m_svn_client->revpropset (name, value, path, svn::Revision::HEAD);
            else
            {
                svn::Revision rev( revision );
                m_svn_client->revpropset (name, value, path, rev);
            }
#endif     
        }
        else
        {
            if ( revision <= 0 )
                m_svn_client->revpropset (name, value, path, svn::Revision::HEAD);
            else
            {
                svn::Revision rev( revision );
                m_svn_client->revpropset (name, value, path, rev);
            }
        }
    }
    catch(svn::ClientException& ce)
    {
        DBGLOG(0,"ClientException at: %s:%d, msg=<%s>", __FILE__,__LINE__,ce.message() );
        throw MakeStringException(0, ce.message() );
    } 
}
//
//
//
void 
MySVN::svnUnsetRevProperty(
        const char* path,
        const char* name,          
        svn_revnum_t revision //= 0
        )
{
    if( m_svn_client == NULL )
        throw MakeStringException(0,"SVN Client is null...");

    if( name==NULL || strlen(name)==0 )
        throw MakeStringException(0,"name is null...");
    
    try
    {
        if( strstr( name, "svn:") )
        {
#ifndef WIN32

            long revision = rev.revnum();
            StringBuffer propSetCommand("svn propdel ");
            propSetCommand.append( name ).append(" --revprop -r");
            propSetCommand.appendlong( revision );
            propSetCommand.append( " " ).append( path );

            //bool 
            DWORD runCode;
            invoke_program( propSetCommand.str(), runCode, false);

            int status;
            pid_t pid=1;
            while( pid > 0 )
            {
                pid = waitpid(-1, &status, WNOHANG);
            } 
#else            
            if ( revision <= 0 )
                m_svn_client->revpropdel(name, path, svn::Revision::HEAD);
            else
            {
                svn::Revision rev( revision );
                m_svn_client->revpropdel(name, path, rev);
            }
#endif     
        }
        else
        {
            if ( revision <= 0 )
                m_svn_client->revpropdel(name, path, svn::Revision::HEAD);
            else
            {
                svn::Revision rev( revision );
                m_svn_client->revpropdel(name, path, rev);
            }
        }
    }
    catch(svn::ClientException& ce)
    {
        DBGLOG(0,"ClientException at: %s:%d, msg=<%s>", __FILE__,__LINE__,ce.message() );
        throw MakeStringException(0, ce.message() );
    } 
}
//
//
//
void 
MySVN::svnGetProperty(
        const char* path,
        const char* name,  
        StringBuffer& value,
        svn_revnum_t revision ) //=0
{
    if( m_svn_client == NULL )
        throw MakeStringException(0,"SVN Client is null...");

    if( path==NULL || strlen(path)==0 )
        throw MakeStringException(0,"path is null...");

    if( name==NULL || strlen(name)==0 )
        throw MakeStringException(0,"name is null...");
    try {        
        svn::Path svnPath( path ); 
        svn_revnum_t revNumber = revision;
        if( revNumber <= 0 )
        {
            //get current version...
            svn::InfoVector infoVector;
            svnInfo( infoVector, path );
            if( infoVector.size() <= 0 )
                throw MakeStringException(0,"svnInfo(%s) failed",path);

            svn::Info &svninfo = infoVector[0];
            revNumber = svninfo.lastChangedRev();
        }            

        svn::Revision rev(revNumber);
        if( strstr( name, "svn:") )
        {
            std::pair<svn_revnum_t,std::string> revProp;
            {
                //MTimeSection timing(NULL, "revpropget()");
                revProp = m_svn_client->revpropget ( name, svnPath, rev );
            }
            value = revProp.second.c_str();
        }
        else
        {
            svn::PathPropertiesMapList pmaplist = m_svn_client->propget (name,svnPath,rev );
            if( pmaplist.size() )
            {
                svn::PathPropertiesMapEntry pme = pmaplist[0];
                svn::PropertiesMap  &pmap = pme.second;
                std::string name_string( name );
                std::string value_string( pmap[name_string] );
                value = value_string.c_str();
            }
        }
    }
    catch(svn::ClientException& ce)
    {
        DBGLOG(0,"ClientException at: %s:%d, msg=<%s>", __FILE__,__LINE__,ce.message() );
        //throw MakeStringException(0, ce.message() );
    } 

    
}   
//
//
//

void writeFile( StringBuffer& filename, const char* text )
{
    recursiveCreateDirectoryForFile( filename.str() );

    Owned<IFile> ifile = createIFile( filename.str() );

	OwnedIFileIO ifileio = ifile->open(IFOcreaterw);
    if (!ifileio)
    {
		throw MakeStringException(0, "Failed to open: %s", ifile->queryFilename()  );              
    }
    else 
    {
        size32_t nBytes = ifileio->write( 0, strlen(text), text );

        if( nBytes != strlen(text) )
        {   
		    throw MakeStringException(0, "Wrote <%d> bytes out of <%d>.", 
                nBytes,  strlen(text) );
        }
    }
}
//
//
//
int 
MySVN::svnSave( 
           const char* user,
           const char* path, 
           const char* url,
           const char* filename, 
           const char* comment,
           const char* text, 
           const char* componentType, //flags
           const char* resultType)  //not needed
{
    svn_revnum_t rev = 0;

    if( m_svn_client == NULL )
        throw MakeStringException(0,"SVN Client is null...");

    if( user == NULL || strlen(user) == 0 )
        throw MakeStringException(0,"User Name not set");

    if( path== NULL || strlen(path ) == 0 )
        throw MakeStringException(0,"Path not set");

    if( filename == NULL || strlen(filename) == 0 )
    {
        //check directory
        Owned<IFile> ifile = createIFile( path );

        svnAdd(path);

        rev = svnCommit( path, comment, NULL,  user);

        return rev; 
    }

    //save File
    StringBuffer fileN( path );
    fileN.append("/").append(filename);

    writeFile( fileN, text);  

    //add, commit
    DBGLOG(0,"svnAdd <%s>", fileN.str());
    svnAdd(fileN.str());
    rev = svnCommit( fileN.str(), comment, NULL, user);
         
    DBGLOG(0,"svnCommit <%s> returned revison <%d>", fileN.str(), rev);

    // set COMPONENT_TYPE, or 'flags' property...
    ///if( rev > 0 ) 
    ///{
	///	  DBGLOG(0,"svnSetProperty: name=<%s>,value=<%s>,path=<%s>", COMPONENT_TYPE, componentType, fileN.str());
   ///     svnSetProperty(  fileN.str(), COMPONENT_TYPE,  componentType, rev);
    ///}
    return rev;
}
//
//
//
void readFile( StringBuffer& filename, StringBuffer& text )
{
    Owned<IFile> ifile = createIFile( filename.str() );
    text.loadFile( ifile );
}
void readFile( const char* filename, StringBuffer& text )
{
    Owned<IFile> ifile = createIFile( filename );
    text.loadFile( ifile );
}

void 
MySVN::svnReadFile( const char* filepath, const char* url, bool includeText, int version, 
                   eclrepository::SvnInfo &info)
{
    if( m_svn_client == NULL )
        throw MakeStringException(0,"SVN Client is null...");

    if( filepath==NULL || strlen(filepath)==0 )
        throw MakeStringException(0,"filepath is null...");

    //if( url==NULL || strlen(url)==0 )
        //throw MakeStringException(0,"url is null...");
    

    /**
    StringBuffer filename( pathTail(filepath) );

    const char* dot = strstr( filename.str(), ".");
    if(dot)
        filename.setLength( filename.length() - strlen(dot) );
    **/

    try {        

        DBGLOG("svnReadFile(): filepath=<%s>, url=<%s>", filepath, url);
        //get checkin comment (svn::log property)
        StringBuffer logMessage;
        if( url && strlen(url) ) 
        {
            svnGetProperty(url, "svn:log", logMessage, version );  
            info.logMessage = logMessage;
        }
   
        if( includeText )
        {                 
            ////readFile( filepath, info.text );
            svn::Path path( filepath );

            std::string text1;
            if( version <= 0 )
                text1 = m_svn_client->cat( path, svn::Revision::HEAD );
            else
            {
                svn::Revision rev( version );
                text1 = m_svn_client->cat( path, rev );
            }
            info.text = text1.c_str();

            //md5 checksum
            md5wrapper md5;
            std::string hash1 = md5.getHashFromString( info.text.str() );
            info.checksum = hash1.c_str();
            info.checksum.setLength( 16 );
        }
        else
        {  
            //md5 checksum
            md5wrapper md5;
            std::string hash1 = md5.getHashFromFile( filepath );  
            info.checksum = hash1.c_str();
            info.checksum.setLength( 16 );   
        }
        //ComponentType, ResultType, LockedBy    
        {
            //MTimeSection timing(NULL, "GetProperties()");
            svnGetProperty( filepath, COMPONENT_TYPE, info.componentType );
            //svnGetProperty( filepath, RESULT_TYPE,    info.resultType );
        }

        //use Info() 
        svn::InfoVector infoVector;        
        {
            //MTimeSection timing(NULL, "svnInfo()");
            svnInfo( infoVector, filepath, version );

        }
        if( infoVector.size() )
        {
            svn::Info &svninfo = infoVector[0];
            
            if( svninfo.locked() )
            {                
                info.lockedBy = svninfo.lockedBy(); 
                info.locked = true;
                DBGLOG("<%s> locked by <%s>", filepath, svninfo.lockedBy() );
            }   
            else 
            {
                info.locked = false;
                DBGLOG("<%s> not locked", filepath );
            }
            //info.lastChangedRevision  = svninfo.revision();  //this is the repository revision
            info.lastChangedRevision  = svninfo.lastChangedRev();

            info.timestamp  = svninfo.lastChangedDate();
            info.author     = svninfo.lastChangedAuthor();
            info.kind       = svninfo.kind();
            //info.fileName   = filename;

            //not set
            info.filesize = 0;            
            info.createdRevision  = 0;
            info.baseRevision = 0;

            //info.path       = svninfo.path();
        }   
        //Sandbox attribute?
        if( url && strstr(url, "/sandboxes/") )
        {
            info.sandboxed = true;
        }
        else { 
            info.sandboxed = false; 
        }

        //??? needed ???
        if( filepath && strstr(filepath, "/sandboxes/") )
        {
            info.sandboxed = true;
        }
    }
    catch(svn::ClientException& ce)
    {
        DBGLOG(0,"ClientException at: %s:%d, msg=<%s>", __FILE__,__LINE__,ce.message() );        
        //throw MakeStringException(0, ce.message() );
    } 
}
//
//
//
bool
MySVN::svnReadFile( const char* dir, const char* url, const char* filename, bool includeText, int version, 
                   eclrepository::SvnInfo *info)
{
	 bool bRet = false;
    if( m_svn_client == NULL )
        throw MakeStringException(0,"SVN Client is null...");

    if( dir==NULL || strlen(dir)==0 )
        throw MakeStringException(0,"path is null...");

    if( filename==NULL || strlen(filename)==0 )
        throw MakeStringException(0,"filename is null...");

    if( url==NULL || strlen(url)==0 )
        throw MakeStringException(0,"url is null...");
    
    try {        

        svn::DirEntries dirEList;

        svnList(dirEList, dir);

        //eclrepository::SvnInfo *info = new eclrepository::SvnInfo;
        svn::DirEntries::const_iterator it;

		  StringBuffer file;
        for (it=dirEList.begin (); it!=dirEList.end (); it++)
        {
            const svn::DirEntry &dirEntry = (*it);
				file = dirEntry.name();
				if (stricmp(filename, file.str()))
					continue;

            info->baseRevision = 0;
            info->lastChangedRevision = 0;
            info->createdRevision = 0;
            info->kind = 0;
            info->type = 0;
            info->state = 0;
            info->sandboxed = 0;
            info->locked = 0;
            info->hasProperties = 0;
            info->timestamp = 0;

            info->lockedBy.clear();
            info->logMessage.clear();
            info->componentType.clear();
            info->resultType.clear();
            info->text.clear();
            info->checksum.clear();
            info->changedTime.clear();

            info->fileName = dirEntry.name();
            info->kind = dirEntry.kind();
            info->author = dirEntry.lastAuthor();
            info->timestamp = (svn_filesize_t) dirEntry.time();
            info->filesize = (int) dirEntry.size();

            StringBuffer filename, fileUrl;
            filename=dir;
            filename.append("/").append(dirEntry.name());

            fileUrl = url;
            fileUrl.append("/").append(dirEntry.name());

            svnReadFile( filename.str(), fileUrl.str(), includeText,version,*info);

				bRet = true;
				break;
        }
    }
    catch(svn::ClientException& ce)
    {
        DBGLOG(0,"ClientException at: %s:%d, msg=<%s>", __FILE__,__LINE__,ce.message() );
    } 

	 return bRet;
}

//
//
//
void 
MySVN::svnReadFiles( const char* dir, const char* url, bool includeText, int version, 
                   eclrepository::SvnInfoVector &infoVector)
{
    if( m_svn_client == NULL )
        throw MakeStringException(0,"SVN Client is null...");

    if( dir==NULL || strlen(dir)==0 )
        throw MakeStringException(0,"filename is null...");

    if( url==NULL || strlen(url)==0 )
        throw MakeStringException(0,"url is null...");
    
    try {        

        svn::DirEntries dirEList;

        svnList(dirEList, dir);

        eclrepository::SvnInfo *info = new eclrepository::SvnInfo;
        svn::DirEntries::const_iterator it;


        for (it=dirEList.begin (); it!=dirEList.end (); it++)
        {
            const svn::DirEntry &dirEntry = (*it);

            info->baseRevision = 0;
            info->lastChangedRevision = 0;
            info->createdRevision = 0;
            info->kind = 0;
            info->type = 0;
            info->state = 0;
            info->sandboxed = 0;
            info->locked = 0;
            info->hasProperties = 0;
            info->timestamp = 0;

            info->lockedBy.clear();
            info->logMessage.clear();
            info->componentType.clear();
            info->resultType.clear();
            info->text.clear();
            info->checksum.clear();
            info->changedTime.clear();

            info->fileName = dirEntry.name();
            info->kind = dirEntry.kind();
            info->author = dirEntry.lastAuthor();
            info->timestamp = dirEntry.time();
            info->filesize = (int) dirEntry.size();

            StringBuffer filename, fileUrl;
            filename=dir;
            filename.append("/").append(dirEntry.name());

            fileUrl = url;
            fileUrl.append("/").append(dirEntry.name());

            svnReadFile( filename.str(), fileUrl.str(), includeText,version,*info);

            infoVector.push_back( *info );
        }
        delete info;
        /*
- <ECLAttribute>
  <ModuleName>doxie</ModuleName> 
  <Name>bankruptcy_service2</Name> 
  <Version>7</Version> 
  <LatestVersion>7</LatestVersion> 
  <Flags>0</Flags> 
  <Access>255</Access> 
  <IsLocked>0</IsLocked> 
  <IsCheckedOut>0</IsCheckedOut> 
  <IsSandbox>0</IsSandbox> 
  <IsOrphaned>0</IsOrphaned> 
  <ResultType>0</ResultType> 
  <ModifiedBy>Vladimir Myullyari</ModifiedBy> 
  <ModifiedDate>2007-01-10T21:32:50Z</ModifiedDate> 
  <Description>Bankruptcy for FCRA CRS (#22197)</Description> 
  <Checksum>c0b8ff61223fcd06</Checksum> 
  </ECLAttribute>
  */
    }
    catch(svn::ClientException& ce)
    {
        DBGLOG(0,"ClientException at: %s:%d, msg=<%s>", __FILE__,__LINE__,ce.message() );
        //throw MakeStringException(0, ce.message() );
    } 
}

//
// 
//
void getTokens( const char* str, const char* delims, StringArray &tokensArray)
{
    if( !str )    return;
    if( !delims ) return;

    const char* strPtr = str;
    const char* delim = delims;
    StringBuffer token;
    for(  ; (*strPtr)!= NULL; strPtr++)
    {
        for( delim = delims; (*delim) != NULL; delim++)
        {
            if( *strPtr == *delim ) //str[n] is a delim?
            {
                if( token.length() )  //terminate token
                {
                    tokensArray.append( token.str() );
                    token.clear();    //reset
                }                    
                break;
            }
            //not a delim?
            if( (*(delim+1)) == NULL )
                token.append( *strPtr );
        }
    }                 
}
//
//
//
static bool searchFile(IFile &iFile, const char* pattern)
{
    StringBuffer file;
    file.loadFile( &iFile );
    bool match = false;

    if( file.length() )
    {
        StringBuffer branchURL;
        StringBuffer filepath;

        if( pattern )
        {
            if( strstr(file.str(), pattern) )
            {
                match = true;
            }
        }
    }
    return match;
}

void findInFiles(
    const char* workDir,
    const char* branchURL,
    StringArray& typeList,
    const char* username,
    const char* pattern,
    const char* regexp,
    bool        includeText,
    bool        includeMatchedLines,
    eclrepository::SvnInfoVector& infoVector)
{
	Owned<IDirectoryIterator> di = createDirectoryIterator( workDir ); 

    DBGLOG(0,"findAttributesWith() Directory: <%s>", workDir );

    StringArray  tokensArray;
    
    char* delims = ".*-()|";

    if( regexp )
        getTokens( regexp, delims, tokensArray);

    if( typeList.length() == 0 )
        typeList.append(ATTR_EXTENSION);

    ForEach (*di)
	{
		StringBuffer fileName;
        di->getName(fileName);

        //skip .svn dir
        if( strcmp( fileName.str(), ".svn") ==0 )
            continue;

        if( !di->isDir() )
        {
            IFile &iFile =  di->get();
		    
            //StringBuffer filename;
            //di->getName(filename);  

            //check types...
            bool found = false;
            const char* type = NULL;
            for(unsigned i=0; i < typeList.length(); i++)
            {
                const char* filename = iFile.queryFilename();
                type = typeList.item(i);
                int filenameLen = strlen(filename);
                int typeLen = strlen(type);
                if( strcmp(filename+(filenameLen-typeLen), type)==0)
                {
                    found = true;
                    break;
                }
            }
            if( !found ) continue;

            bool match=false;                    

            if( pattern )
            {
                match = searchFile( iFile, pattern );
               
                if( match )
                {
                    eclrepository::SvnInfo info;

                    info.path = workDir;
                    info.fileName = fileName;
                    info.type = 1; //fix me
                    info.text.clear();
                    if( includeText )
                    {
                        info.text.loadFile( &iFile );
                    }
                    if( includeMatchedLines )
                    {
                        //fix me...
                        info.text.loadFile( &iFile );
                    }
                    infoVector.push_back(info);
/**
TODO:  Set:

  <Version>1</Version> 
  <LatestVersion>1</LatestVersion> 
  <Flags>0</Flags> 

  <IsLocked>0</IsLocked> 
  <IsCheckedOut>0</IsCheckedOut> 
  <IsSandbox>0</IsSandbox> 
  <IsOrphaned>0</IsOrphaned> 
  <ResultType>0</ResultType> 
  <ModifiedBy>Wendy Ma</ModifiedBy> 
  <ModifiedDate>2006-01-26T19:18:42Z</ModifiedDate> 
  <Description>init</Description> 
  <Checksum>42a51a01fa43d95a</Checksum> 

  **/
                }
            }
            else if( regexp )
            {
                //TODO, handle '&' in regexp
                if( strstr(regexp, ".*--((") )
                {
                    StringBuffer searchToken;
                    for( unsigned i(0); !match && i < tokensArray.ordinality(); i++)
                    {
                        //DBGLOG("token is: %s", tokensArray.item(i));

                        searchToken.clear().append("--").append( tokensArray.item(i) ).append("--");

                        //if( strstr(file.str(), searchToken.str() ) )
                        //    match = true;
                        match = searchFile(iFile, searchToken.str() );
                    }
                }
                else 
                {
                    StringBuffer file;
                    file.loadFile( &iFile );
                    RegExpr re( regexp );
                    if( re.find( file.str() ) )
                    {
                        match = true;
                    }
                }

                if( match )
                {
                    eclrepository::SvnInfo info;

                    info.path = workDir;
                    info.fileName = fileName;
                    info.type = 1; //fix me
                    info.text.clear();
                    if( includeText )
                    {
                        info.text.loadFile( &iFile );
                    }
                    if( includeMatchedLines )
                    {
                        //fix me...
                        info.text.loadFile( &iFile );
                    }
                    infoVector.push_back(info);
                }                        
            }   
        }
    }
}
#define LINETERM '\n'

int readLine(OwnedIFileIO& ifo, StringBuffer& buf, int& filePos)
{
	char onechar[1];
	int len = 0;
	char linebuf[1024];
	int bufIndex= 0;
	while((len = ifo->read(filePos, 1, onechar )) > 0)
	{
		if(bufIndex == 1024)
		{
			buf.append(1024, linebuf);
			bufIndex = 0;
		}
	    filePos++;
        if(onechar[0] == LINETERM)
            break;

		linebuf[bufIndex++] = onechar[0];
	}    
	if(bufIndex > 0)
		buf.append(bufIndex, linebuf);

	return buf.length();
}
//
//
//
void MySVN::findInFilesUsingEgrep(
    const char* workDir,
    const char* branchURL,
    StringArray& typeList,
    const char* username,
    const char* pattern,
    const char* regexp,
    bool        includeText,
    bool        includeMatchedLines,
    eclrepository::SvnInfoVector& infoVector)

{
    //start in workdir and look for pattern or regexp, return results...

    StringBuffer searchPat;
    if( pattern )
    searchPat.append( pattern );
    else 
    searchPat.append( regexp );

    //StringBuffer command("egrep.exe -r --regexp=\"");
    StringBuffer command("egrep -r --regexp=\"");

    command.append( searchPat ).append("\" ").append( workDir )
        .append( " > egrep.out");       

    DBGLOG(0,"Executing command: <%s>", command.str() );

    int failed =system ( command.str() );
    if( failed )
    {   
      StringBuffer msg;
      msg.append( "findInFiles(): system command:<").append( command.str() ).append("> failed.");
      DBGLOG(0, msg.str() );
      throw MakeStringException(0, msg.str() );
    }

    //read egrep.out, 
    
    Owned<IFile> ifile = createIFile( "./egrep.out" );
    if( ifile->isFile() )    
    {
        OwnedIFileIO fio = ifile->open(IFOread);
        StringBuffer inputLine;
        int filePos = 0;
        while( readLine( fio, inputLine.clear(), filePos ) )
        {
            //check for .svn dir

            if( strstr( inputLine.str(), ".svn") )
                continue;

            //DBGLOG(0, "findInFilesUsingGrep: read line <%s>", inputLine.str() );

            //format is: (path)/filename: pattern_found
            // get filename...
            int at = String(inputLine).lastIndexOf( ':' );

            if( at > 0 )
            {
                inputLine.setLength( at );

                StringBuffer fileName( inputLine );
                
                DBGLOG(0, "findInFilesUsingGrep: inputLine=<%s>", inputLine.str() );

                getFileNameOnly( fileName, false/*no ext*/);  //erases path, returns filename + .ext

                DBGLOG(0, "findInFilesUsingGrep: found file <%s>", fileName.str() );

                if( fileName.length() )
                {
                        eclrepository::SvnInfo info;

                        StringBuffer filePath, fileUrl;

                        //moduleName
                        StringBuffer moduleName( inputLine );
                        moduleName.setLength( String(inputLine).lastIndexOf( '/' )  );

                        DBGLOG(0, "findInFilesUsingGrep: moduleName: <%s>", moduleName.str() );

                        //moduleName.trimLeft( );
                        String foo(moduleName);
                        moduleName =  /*String->toCharArray()*/
                            ( foo.substring(  String(moduleName).lastIndexOf( '/') +1)->toCharArray() );

                        //module search?  module name not needed....
                        if( strstr( workDir, moduleName.str() ) )
                        {
                            moduleName.clear();
                        }
                        else {
                            moduleName.append("/");
                        }

                        filePath=workDir;
                        filePath.append("/").append( moduleName ).append( fileName );

                        fileUrl = branchURL;
                        fileUrl.append("/").append( moduleName ).append( fileName );

                        if( moduleName.length() )
                            moduleName.setLength( moduleName.length()-1);  //strip ending '/'

                        svnReadFile( filePath.str(), fileUrl.str(), includeText, 0/*version*/, info);                        

                        info.fileName = fileName;
                        info.path     = workDir;

                        //now fill in the rest of svnInfo...
                        info.moduleName = moduleName;
                        info.url = fileUrl.str();

/*
class SvnInfo
{
public:
	StringBuffer fileName;
    StringBuffer moduleName;
    StringBuffer path;
    StringBuffer url;
    StringBuffer changedTime;
    int baseRevision;
    int lastChangedRevision;
    int createdRevision;
    int kind;//set me
    int type;
    int filesize;
    bool hasProperties;
    apr_time_t timestamp;
    StringBuffer author;
    bool locked;
    StringBuffer lockedBy;
    StringBuffer logMessage;

    StringBuffer checksum;

    StringBuffer componentType;
    StringBuffer resultType;

    StringBuffer text;

    int     state;
    bool    sandboxed;
};**/
                        //info.kind = dirEntry.kind();
                        //info.author = dirEntry.lastAuthor();
                        //info.timestamp = dirEntry.time();
                        //info.filesize = dirEntry.size();

                        infoVector.push_back( info );

                        /***
                        info.path = workDir;
                        info.fileName = fileName;
                        info.type = 1; //fix me
                        info.text.clear();
                        if( includeText )
                        {
                            info.text.loadFile( ifile.get() );
                        }
                        if( includeMatchedLines )
                        {
                            //fix me...
                            info.text.loadFile( ifile.get() );
                        }
                        infoVector.push_back(info);
                        ***/
    /**
 tODO: 

  need:: 

- <ECLAttribute>
  <ModuleName>Gong</ModuleName> 
  <Name>GONG_BDID_Search</Name> 
  <Version>2</Version> 
  <LatestVersion>2</LatestVersion> 
  <Flags>0</Flags> 
  <Access>255</Access> 
  <IsLocked>0</IsLocked> 
  <IsCheckedOut>0</IsCheckedOut> 
  <IsSandbox>0</IsSandbox> 
  <IsOrphaned>0</IsOrphaned> 
  <ResultType>0</ResultType> 
  <ModifiedBy>Tammy Gibson</ModifiedBy> 
  <ModifiedDate>2006-11-02T17:44:00Z</ModifiedDate> 
  <Description>2005-05-06 10:34:04, Copied with AMT from Attribute Modified by Colin Maroney1</Description> 
  <Checksum>e3c28901512deaab</Checksum> 
  </ECLAttribute>


      **/                    
                }
            }
        }        
    }
}

void MySVN::FindInFiles(                                 
    const char* username,
    const char* workDir,
    const char* branchURL,
    StringArray& typeList,
    const char* pattern,
    const char* regexp,
    bool        includeText,
    bool        includeMatchedLines,
    eclrepository::SvnInfoVector& infoVector)   
                   //std::vector<ECLAttribute*>& results)
{
    //const UserProfile& userProfile = getUserProfile(  user );
    //StringBuffer workingDir        = userProfile.BranchDirectory;
    //workingDir.append("/").append( moduleName );

    Owned<IFile> ifile = createIFile( workDir );
    if( ifile->isFile() )    
    {
        //findInFile();
    }
    else if( ifile->isDirectory() )
    {
        //findInFiles(workDir,branchURL,typeList,username,pattern,regexp,includeText,includeMatchedLines,infoVector);
        findInFilesUsingEgrep(workDir,branchURL,typeList,username,pattern,regexp,includeText,includeMatchedLines,infoVector);
    }
    else {
        //throw up
    }
}





//  OLD FindInFiles logic...
#ifdef NOT  


        else if( di->isDir() )
        {
            //StringBuffer path( userProfile.BranchDirectory );
            StringBuffer path( workdir );
            path.append("/").append( moduleName );
	        
            Owned<IDirectoryIterator> attributeIterator = createDirectoryIterator( path.str() );

            StringBuffer attrName;

            ForEach( *attributeIterator )
            {
		        StringBuffer filename;
                attributeIterator->getName(filename);            

                if( attributeIterator->isDir() )
                    continue; //skip .svn
               
                //DBGLOG(0,"      Attribute =<%s>", filename.str() );

                //check attrname if set...
                /***
                if( attrname && strlen(attrname) )
                {
                    StringBuffer attributeName( attrname );
                    if( !strstr( attrname, ATTR_EXTENSION) )
                        attributeName.append(ATTR_EXTENSION);

                    if ( strcmp(attributeName.str(), filename.str()) != 0)
                        continue;//attrname name does not match...
                }
                ***/
                IFile &iFile =  attributeIterator->get();

                //check types...
                bool found = false;
                const char* type = NULL;
                for(int i=0; i < typeList.length(); i++)
                {
                    const char* filename = iFile.queryFilename();
                    type = typeList.item(i);
                    int filenameLen = strlen(filename);
                    int typeLen = strlen(type);
                    if( strcmp(filename+(filenameLen-typeLen), type)==0)
                    {
                        found = true;
                        break;
                    }
                }
                if( !found ) continue;

                bool match=false;                    
                StringBuffer branchURL;
                StringBuffer filepath; 
                if( pattern )
                {
                    match = searchFile( iFile, pattern );
                   
                    if( match )
                    {
                        //found pattern... include            
                        //branchURL = userProfile.BranchURL;
                        branchURL = repos_path;
                        branchURL.append("/").append( moduleName );
                        branchURL.append("/").append( filename );
    
                        filepath = workdir;
                        filepath.append("/").append( moduleName );
                        filepath.append("/").append( filename );

                        ECLAttribute * eclatt   = new ECLAttribute;
                        eclatt->ModuleName      = moduleName;
                        eclatt->Filename        = filename;                                                                                                     
                        eclatt->Type            = type;
                    
                        //FIX readAttribute( filepath.str(), branchURL.str(), *eclatt, includeText);                            

                        attributeMap[std::string( filename )] = eclatt;
                        //results.push_back( eclatt );
                    }
                }
/***


                if( file.length() )
                {

    
                    bool match = false;
                    if( pattern )
                    {
                        if( strstr(file.str(), pattern) )
                        {
                            match = true;
                        }

                    }
***/
                else if( regexp )
                {
                    //TODO, handle '&' in regexp
                    if( strstr(regexp, ".*--((") )
                    {
                        StringBuffer searchToken;
                        for( int i(0); !match && i < tokensArray.ordinality(); i++)
                        {
                            //DBGLOG("token is: %s", tokensArray.item(i));

                            searchToken.clear().append("--").append( tokensArray.item(i) ).append("--");

                            //if( strstr(file.str(), searchToken.str() ) )
                            //    match = true;
                            match = searchFile(iFile, searchToken.str() );
                        }
                    }
                    else 
                    {
                        StringBuffer file;
                        file.loadFile( &iFile );
                        RegExpr re( regexp );
                        if( re.find( file.str() ) )
                        {
                            match = true;
                        }
                    }
                    if( match )
                    {
                        branchURL = repos_path;
                        branchURL.append("/").append( moduleName );
                        branchURL.append("/").append( filename );
    
                        filepath = workdir;
                        filepath.append("/").append( moduleName );
                        filepath.append("/").append( filename );

                        ECLAttribute * eclatt   = new ECLAttribute;
                        eclatt->ModuleName      = moduleName;
                        eclatt->Filename        = filename;                                                                                                     
                        eclatt->Type            = type;
                    
                        //FIX readAttribute( filepath.str(), branchURL.str(), *eclatt, includeText);          

                        attributeMap[std::string( filename )] = eclatt;
                        //results.push_back( eclatt );
                    }                        
                }               
            }
        }
    }

    }
}
#endif
//};
 
