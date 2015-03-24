/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2015 HPCC Systems.

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

#ifndef _COMPONENTSTATUS_HPP__
#define _COMPONENTSTATUS_HPP__

#include "jiface.hpp"

class IEspComponentStatus;
class IEspStatusReport;
class IConstComponentStatus;
class IEspComponentStatus;
class IPropertyTreeIterator;
class StringBuffer;
class IPropertyTree;
template<class IEspComponentStatus> class IArrayOf;


#ifdef WIN32
    #ifdef SMCLIB_EXPORTS
        #define COMPONENTSTATUS_API __declspec(dllexport)
    #else
        #define COMPONENTSTATUS_API __declspec(dllimport)
    #endif
#else
    #define COMPONENTSTATUS_API
#endif

enum ComponentTypeID
{
    CTThorCluster = 0,
    CTHThorCluster = 1,
    CTRoxieCluster = 2,
    CTDaliServer = 3,
    CTDFUServer = 4,
    CTEclServer = 5,
    CTEclCCServer = 6,
    CTEclAgent = 7,
    CTEclSechduler = 8,
    CTSashaServer = 9,
    CTESPServer = 10,
    ComponentTypeIDSize = 11
};

static const char *componentType[] = { "ThorCluster", "HThorCluster", "RoxieCluster", "DaliServer",
    "DFUServer", "EclServer", "EclCCServer", "EclAgent", "EclSechduler", "SashaServer", "ESPServer" };

enum ComponentStatusTypeID
{
    CSTNormal = 0,
    CSTWarning = 1,
    CSTError = 2,
    ComponentStatusTypeIDSize = 3
};

static const char *componentStatusType[] = { "Normal", "Warning", "Error"};

class IComponentStatusUtils : public IInterface
{
public:
    virtual StringBuffer& getComponentTypeByID(unsigned ID, StringBuffer& out) = 0;
    virtual StringBuffer& getComponentStatusTypeByID(unsigned ID, StringBuffer& out) = 0;
    virtual void setComponentTypes(IPropertyTree* cfg) = 0;
    virtual void setComponentStatusTypes(IPropertyTree* cfg) = 0;
};

class IESPComponentStatusInfo : public IInterface
{
public:
    virtual const char* getReporter() = 0;
    virtual const char* getTimeStamp() = 0;
    virtual int getComponentStatusID() = 0;
    virtual const int getComponentTypeID() = 0;
    virtual IEspStatusReport* getStatusReport() = 0;
    virtual IArrayOf<IEspComponentStatus>& getComponentStatus() = 0;
    virtual void mergeComponentStatusInfo(IESPComponentStatusInfo& statusInfo) = 0;
    virtual void updateComponentStatus(IArrayOf<IConstComponentStatus>& StatusList) = 0;
};

class IComponentStatusFactory : public IInterface
{
public:
    virtual IESPComponentStatusInfo* getComponentStatus() = 0;
    virtual void updateComponentStatus(const char* reporter, IArrayOf<IConstComponentStatus>& StatusList) = 0;
};

extern "C" COMPONENTSTATUS_API IComponentStatusFactory* getComponentStatusFactory();
extern "C" COMPONENTSTATUS_API IComponentStatusUtils* getComponentStatusUtils();

#endif  //_COMPONENTSTATUS_HPP__
