/*##############################################################################
    HPCC SYSTEMS software Copyright (C) 2014 HPCC Systems.
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

#ifndef _DPAGENTBASE_HPP__
#define _DPAGENTBASE_HPP__

#pragma warning (disable : 4786)

#include "jiface.hpp"
#include "jstring.hpp"

#ifdef WIN32
    #ifdef DPAGENTLIB_EXPORTS
        #define DPAGENTLIB_API __declspec(dllexport)
    #else
        #define DPAGENTLIB_API __declspec(dllimport)
    #endif
#else
    #define DPAGENTLIB_API
#endif

#define LIBPDMSERVICELIB "PDMServiceLib"

interface IDPAgent : extends IInterface
{
    virtual void init(int _traceLevel) = 0;
    virtual bool deploy(const char* package, const char* options, StringBuffer& results, bool reDeploy) = 0;
    virtual bool undeploy(StringBuffer& results) = 0;
    virtual bool start(const char* component, StringBuffer& results, bool restart) = 0;
    virtual bool stop(const char* component, StringBuffer& results) = 0;
};

typedef IDPAgent* (*newPDAgentLib_t_)();

#endif  //_DPAGENTBASE_HPP__
