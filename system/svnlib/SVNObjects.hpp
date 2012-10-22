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

#ifndef __SVNObjects__
#define __SVNObjects__

//#include "UserProfile.hpp"
#include <iostream> 
#include <jstring.hpp>
#include <jtime.hpp>

//#include "MySVN.hpp"

#include "svncpp/client.hpp"

namespace eclrepository
{
 

   

class Module;
class Attribute; 

class SVNConfig
{
public:
    
private:
    StringBuffer m_svn_root_url; 
    StringBuffer m_wc_root_dir;  //working copy root    
    StringBuffer m_environment;
    StringBuffer m_branch;
};

class SVNBase
{
public:
    SVNBase() {}
    virtual ~SVNBase() {}


    void setConfig( const SVNConfig& config)
    { 
        m_svn_config = config;
    }

    void setName( const char* name ){m_name = name;}
    const char* Name() {return m_name.str();}

protected:
    StringBuffer m_name;
    long         m_checkSum;

    static SVNConfig m_svn_config;
private:
};

//
//
//
class Module : public SVNBase
{
public:
    Module() {}
    virtual ~Module() {}

private:

    std::list<SVNBase> m_Modules;
    std::list<SVNBase> m_Attributes;
};

//
// Attribute
//
class Attribute : public SVNBase
{
public:
    Attribute() {}
    virtual ~Attribute() {}

private:
    StringBuffer m_moduleName;
    StringBuffer m_comment;
    StringBuffer m_date; 
    StringBuffer m_lockedBy;
    StringBuffer m_text;
    StringBuffer m_componentType;
    StringBuffer m_resultType;

    int          m_revision;
    int          m_type;  //ecl, xsdl, xslt
    int          m_state; //??
};

//
// Info
//

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
};

typedef std::vector<SvnInfo> SvnInfoVector;

};
#endif

