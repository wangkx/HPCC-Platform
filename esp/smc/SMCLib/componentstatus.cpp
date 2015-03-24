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

#include "jiface.hpp"
#include "componentstatus.hpp"
#include "jlib.hpp"
#include "esp.hpp"
#include "ws_machine_esp.ipp"
#include "ws_machine.hpp"

class CComponentStatusUtils : public CInterface, implements IComponentStatusUtils
{
    StringArray componentTypes, componentStatusTypes;
    unsigned numOfComponentTypes, numOfComponentStatusTypes;

    void readTypeStringsFromCFGTree(const char* xpath, IPropertyTree* cfg, StringArray& typeStrings)
    {
        Owned<IPropertyTreeIterator> itr = cfg->getElements(xpath);
        ForEach(*itr)
        {
            IPropertyTree& branch = itr->query();
            const char* type = branch.queryProp("@type");
            if (type && *type)
                typeStrings.append(type);
        }
    }

    StringBuffer& getTypeStringByID(int id, int idLimit, const char* type, StringArray& typeStrings, StringBuffer& out)
    {
        if (id < idLimit)
            out.set(typeStrings.item(id));
        else //in case of undefined ID
            out.setf("%s %d", type, id);
        return out;
    };
public:
    IMPLEMENT_IINTERFACE
    CComponentStatusUtils()
    {
        numOfComponentTypes = 0;
        numOfComponentStatusTypes = 0;
    };
    virtual ~CComponentStatusUtils() { };

    virtual StringBuffer& getComponentTypeByID(unsigned id, StringBuffer& out)
    {
        return getTypeStringByID(id, numOfComponentTypes, "Component", componentTypes, out);
    }
    virtual StringBuffer& getComponentStatusTypeByID(unsigned id, StringBuffer& out)
    {
        return getTypeStringByID(id, numOfComponentStatusTypes, "Status", componentStatusTypes, out);
    }
    virtual void setComponentTypes(IPropertyTree* cfg)
    {
        for (unsigned i = 0; i < ComponentTypeIDSize; i++)
            componentTypes.append(componentType[i]);

        //Read more components from config if any
        readTypeStringsFromCFGTree("ComponentStatus/ComponentTypes/ComponentType", cfg, componentTypes);
        numOfComponentTypes = componentTypes.length();
    };
    virtual void setComponentStatusTypes(IPropertyTree* cfg)
    {
        //Read component status from config if any.
        readTypeStringsFromCFGTree("ComponentStatus/ComponentStatusTypes/ComponentStatusType", cfg, componentStatusTypes);

        if (!componentStatusTypes.length())
        {//If status type is not defined in config, use default status types.
            for (unsigned ii = 0; ii < ComponentStatusTypeIDSize; ii++)
                componentStatusTypes.append(componentStatusType[ii]);
        }
        numOfComponentStatusTypes = componentStatusTypes.length();
    };
};

class CESPComponentStatusInfo : public CInterface, implements IESPComponentStatusInfo
{
    StringAttr reporter;
    StringAttr timeCached;
    IArrayOf<IEspComponentStatus> statusList;
    Owned<IComponentStatusUtils> componentStatusUtils;

    bool fromReporter; //this CESPComponentStatusInfo object is created by ws_machine.UpdateComponentStatus
    int componentStatusID; //the worst component status in the system
    int componentTypeID; //the worst component status in the system
    Owned<IEspStatusReport> componentStatusReport; //the worst component status in the system

    void updateTimeStamp()
    {
        char timeStr[32];
        time_t tNow;
        time(&tNow);

#ifdef _WIN32
        struct tm *ltNow;
        ltNow = localtime(&tNow);
        strftime(timeStr, 32, "%Y-%m-%d %H:%M:%S", ltNow);
#else
        struct tm ltNow;
        localtime_r(&tNow, &ltNow);
        strftime(timeStr, 32, "%Y-%m-%d %H:%M:%S", &ltNow);
#endif
        timeCached.set(timeStr);
    }
    void addStatusReport(const char* reporterIn, const char* timeCachedIn, IConstComponentStatus& csIn, IEspComponentStatus& csOut)
    {
        int componentType = csIn.getComponentTypeID();
        IArrayOf<IConstStatusReport>& statusReports = csOut.getStatusReports();
        IArrayOf<IConstStatusReport>& reportsIn = csIn.getStatusReports();
        ForEachItemIn(i, reportsIn)
        {
            IConstStatusReport& report = reportsIn.item(i);

            int statusID = report.getStatusTypeID();
            Owned<IEspStatusReport> statusReport = createStatusReport();
            statusReport->setStatusTypeID(statusID);

            const char* details = report.getStatusDetails();
            if (details && *details)
                statusReport->setStatusDetails(details);

            const char* url = report.getURL();
            if (url && *url)
                statusReport->setURL(url);

            statusReport->setReporter(reporterIn);
            statusReport->setTimeCached(timeCachedIn);
            if (!fromReporter)
            {//We need to add more info for a user-friendly output
                StringBuffer statusStr;
                statusReport->setStatusType(componentStatusUtils->getComponentStatusTypeByID(statusID, statusStr).str());

                if (statusID > csOut.getComponentStatusID()) //worst case
                {
                    csOut.setComponentStatusID(statusID);
                    csOut.setComponentStatus(statusStr.str());
                    csOut.setReporter(reporterIn);
                }
                if (statusID > componentStatusID) //worst case
                {
                    componentTypeID = componentType;
                    componentStatusID = statusID;
                    componentStatusReport.setown(statusReport.getLink());
                    reporter.set(reporterIn);
                }
            }
            statusReports.append(*statusReport.getClear());
        }
    }
    void addComponentStatus(const char* reporterIn, const char* timeCachedIn, IConstComponentStatus& st)
    {
        Owned<IEspComponentStatus> cs = createComponentStatus();
        cs->setComponentStatusID(-1);

        int componentType = st.getComponentTypeID();
        cs->setComponentTypeID(componentType);

        StringBuffer componentTypeStr;
        cs->setComponentType(componentStatusUtils->getComponentTypeByID(componentType, componentTypeStr).str());

        IArrayOf<IConstStatusReport> statusReports;
        cs->setStatusReports(statusReports);

        addStatusReport(reporterIn, timeCachedIn, st, *cs);
        statusList.append(*cs.getClear());
    }
public:
    IMPLEMENT_IINTERFACE;

    CESPComponentStatusInfo(const char* _reporter)
    {
        componentStatusUtils.setown(getComponentStatusUtils());
        componentStatusID = -1;
        fromReporter = _reporter? true : false;
        if (_reporter && *_reporter)
            reporter.set(_reporter);
    };

    virtual const char* getReporter() { return reporter.get(); };
    virtual const char* getTimeStamp() { return timeCached.get(); };
    virtual int getComponentStatusID() { return componentStatusID; };
    virtual const int getComponentTypeID() { return componentTypeID; };
    virtual IEspStatusReport* getStatusReport() { return componentStatusReport; };
    virtual IArrayOf<IEspComponentStatus>& getComponentStatus() { return statusList; };

    virtual void mergeComponentStatusInfo(IESPComponentStatusInfo& statusInfo)
    {
        const char* reporterIn = statusInfo.getReporter();
        const char* timeCachedIn = statusInfo.getTimeStamp();
        IArrayOf<IEspComponentStatus>& statusListIn = statusInfo.getComponentStatus();
        ForEachItemIn(i, statusListIn)
        {
            bool newCompoment = true;
            IEspComponentStatus& statusIn = statusListIn.item(i);
            ForEachItemIn(ii, statusList)
            {
                IEspComponentStatus& statusOut = statusList.item(ii);
                if (statusIn.getComponentTypeID() == statusOut.getComponentTypeID())
                {
                    addStatusReport(reporterIn, timeCachedIn, statusIn, statusOut);
                    newCompoment =  false;
                    break;
                }
            }
            if (newCompoment)
                addComponentStatus(reporterIn, timeCachedIn, statusIn);
        }
    }
    virtual void updateComponentStatus(IArrayOf<IConstComponentStatus>& statusListIn)
    {
        statusList.kill();
        updateTimeStamp();
        ForEachItemIn(s, statusListIn)
            addComponentStatus(reporter, timeCached, statusListIn.item(s));
    }
};

static CriticalSection componentStatusSect;

class CComponentStatusFactory : public CInterface, implements IComponentStatusFactory
{
    IArrayOf<IESPComponentStatusInfo> cache; //multiple caches from different reporter

    void updateCache(const char* reporter, Owned<IESPComponentStatusInfo> status)
    {
        ForEachItemIn(i, cache)
        {
            IESPComponentStatusInfo& item = cache.item(i);
            if (!strieq(reporter, item.getReporter()))
                continue;
            cache.remove(i);
            break;
        }
        cache.append(*status.getClear());
    }
public:
    IMPLEMENT_IINTERFACE;

    CComponentStatusFactory() { };

    virtual ~CComponentStatusFactory()
    {
        cache.kill();
    };

    virtual IESPComponentStatusInfo* getComponentStatus()
    {
        CriticalBlock block(componentStatusSect);

        Owned<IESPComponentStatusInfo> status = new CESPComponentStatusInfo(NULL);
        ForEachItemIn(i, cache)
        {
            IESPComponentStatusInfo& item = cache.item(i);
            status->mergeComponentStatusInfo(item);
        }
        return status.getClear();
    }

    virtual void updateComponentStatus(const char* reporter, IArrayOf<IConstComponentStatus>& statusList)
    {
        CriticalBlock block(componentStatusSect);

        Owned<IESPComponentStatusInfo> status = new CESPComponentStatusInfo(reporter);
        status->updateComponentStatus(statusList);
        updateCache(reporter, status.getClear());
    }
};

static CComponentStatusFactory *csFactory = NULL;

static CriticalSection getComponentStatusSect;

extern COMPONENTSTATUS_API IComponentStatusFactory* getComponentStatusFactory()
{
    CriticalBlock block(getComponentStatusSect);

    if (!csFactory)
        csFactory = new CComponentStatusFactory();

    return LINK(csFactory);
}

static CComponentStatusUtils *csUtils = NULL;

static CriticalSection getComponentStatusUtilsSect;

extern COMPONENTSTATUS_API IComponentStatusUtils* getComponentStatusUtils()
{
    CriticalBlock block(getComponentStatusUtilsSect);

    if (!csUtils)
        csUtils = new CComponentStatusUtils();

    return LINK(csUtils);
}
