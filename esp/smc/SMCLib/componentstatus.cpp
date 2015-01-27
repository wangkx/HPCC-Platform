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

#include "jlib.hpp"
#include "componentstatus.hpp"

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
    int systemStatusID; //the worst component status in the system
    IArrayOf<IEspComponentStatus> status;
    Owned<IComponentStatusUtils> componentStatusUtils;

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
    void copyAndSaveComponentStatus(IConstComponentStatus& st)
    {
        Owned<IEspComponentStatus> cs = createComponentStatus();
        cs->setComponentTypeID(st.getComponentTypeID());
        cs->setComponentType(st.getComponentType());
        cs->setStatusTypeID(st.getStatusTypeID());
        cs->setStatusType(st.getStatusType());
        cs->setStatusDetails(st.getStatusDetails());
        status.append(*cs.getClear());
    }
    void addNewComponentStatus(const char* reporter, const char* timeCached, IEspComponentStatus& st)
    {
        int statusID = st.getStatusTypeID();
        if (statusID > systemStatusID) //worst case
            systemStatusID = statusID;

        StringBuffer componentTypeStr, statusStr;
        int componentTypeID = st.getComponentTypeID();
        componentStatusUtils->getComponentTypeByID(componentTypeID, componentTypeStr);
        componentStatusUtils->getComponentStatusTypeByID(statusID, statusStr);
        VStringBuffer details("%s %s: %s -- %s", timeCached, reporter, statusStr.str(), st.getStatusDetails());

        Owned<IEspComponentStatus> cs = createComponentStatus();
        cs->setComponentTypeID(componentTypeID);
        cs->setComponentType(componentTypeStr.str());
        cs->setStatusTypeID(statusID);
        cs->setStatusType(statusStr.str());
        cs->setStatusDetails(details.str());
        status.append(*cs.getClear());
    }
    void updateStatus(const char* reporter, const char* timeCached, IEspComponentStatus& st, IEspComponentStatus& stMerged)
    {
        int statusID = st.getStatusTypeID();
        if (statusID > systemStatusID) //worst case
            systemStatusID = statusID;

        StringBuffer statusStr;
        componentStatusUtils->getComponentStatusTypeByID(statusID, statusStr);
        if (statusID > stMerged.getStatusTypeID())
        {
            stMerged.setStatusTypeID(statusID);
            stMerged.setStatusType(statusStr.str());
        }

        VStringBuffer details("%s; %s %s: %s -- %s", stMerged.getStatusDetails(), timeCached, reporter, statusStr.str(), st.getStatusDetails());
        stMerged.setStatusDetails(details.str());
    }
public:
    IMPLEMENT_IINTERFACE;

    CESPComponentStatusInfo(const char* _reporter) : reporter(_reporter)
    {
        systemStatusID = -1;
        componentStatusUtils.setown(getComponentStatusUtils());
    };

    virtual const char* getReporter() { return reporter.get(); };
    virtual const char* getTimeStamp() { return timeCached.get(); };
    virtual int getSystemStatusID() { return systemStatusID; };
    virtual IArrayOf<IEspComponentStatus>& getComponentStatus() { return status; };
    virtual void mergeComponentStatus(const char* reporter, const char* timeCached, IArrayOf<IEspComponentStatus>& statusIn)
    {
        ForEachItemIn(i, statusIn)
        {
            bool newCompoment = true;
            IEspComponentStatus& item = statusIn.item(i);
            ForEachItemIn(ii, status)
            {
                IEspComponentStatus& item1 = status.item(ii);
                if (item.getComponentTypeID() == item1.getComponentTypeID())
                {
                    updateStatus(reporter, timeCached, item, item1);
                    newCompoment =  false;
                    break;
                }
            }
            if (newCompoment)
                addNewComponentStatus(reporter, timeCached, item);
        }
    }
    virtual IArrayOf<IEspComponentStatus>& updateComponentStatus(IArrayOf<IConstComponentStatus>& statusList)
    {//Called from another process, such as a SOAP call
        status.kill();
        updateTimeStamp();
        ForEachItemIn(s, statusList)
            copyAndSaveComponentStatus(statusList.item(s));
        return status;
    }
    virtual IArrayOf<IEspComponentStatus>& updateComponentStatus(IArrayOf<IEspComponentStatus>& statusList)
    {//Called from another thread
        status.kill();
        updateTimeStamp();
        ForEachItemIn(s, statusList)
            status.append(*LINK(&statusList.item(s)));
        return status;
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

        Owned<IESPComponentStatusInfo> status = new CESPComponentStatusInfo("");
        ForEachItemIn(i, cache)
        {
            IESPComponentStatusInfo& item = cache.item(i);
            status->mergeComponentStatus(item.getReporter(), item.getTimeStamp(), item.getComponentStatus());
        }
        return status.getClear();
    }

    virtual void updateComponentStatus(const char* reporter, IArrayOf<IConstComponentStatus>& statusList)
    {//Called from another process, such as a SOAP call
        CriticalBlock block(componentStatusSect);

        Owned<IESPComponentStatusInfo> status = new CESPComponentStatusInfo(reporter);
        status->updateComponentStatus(statusList);
        updateCache(reporter, status.getClear());
    }

    virtual void updateComponentStatus(const char* reporter, IArrayOf<IEspComponentStatus>& statusList)
    {//Called from another thread
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
