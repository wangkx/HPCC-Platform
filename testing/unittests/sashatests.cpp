/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2018 HPCC Systems®.

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

/*
 * Sasha Quick Regression Suite: Tests Sasha functionality on a programmatic way.
 *
 */

#ifdef _USE_CPPUNIT

#include "ws_workunits.hpp"
#include "mpbase.hpp"
#include "mpcomm.hpp"
#include "sacmd.hpp"

#include "unittests.hpp"

//#define COMPAT

#define MY_DEBUG

// ======================================================================= Support Functions / Classes

IClientWsWorkunits * createWorkunitsClient()
{
    Owned<IClientWsWorkunits> wuclient = createWsWorkunitsClient();
    wuclient->addServiceUrl("http://localhost:8010/WsWorkUnits");
    return LINK(wuclient.get());
}

// ================================================================================== UNIT TESTS

static void setupABC(const IContextLogger &logctx, unsigned &testNum, StringAttr &testString)
{
#ifdef MY_DEBUG
    //logctx.CTXLOG("Enter setupABC."); //Not sure why logctx.CTXLOG does not work
    printf("Enter setupABC.\n");
#endif

    testNum = 2;
    testString.set("abc");

    // Make sure it got created
    ASSERT(testString.length() == 3 && "Can't set testString");

#ifdef MY_DEBUG
    printf("Leave setupABC: testNum=%d; testString=%s.\n", testNum, testString.get());
#endif
}

class CSashaClient
{
    Owned<INode> sashaserver;
/*
    void getSashaNode(SocketEndpoint &ep)
    {
        Owned<IEnvironmentFactory> factory = getEnvironmentFactory();
        Owned<IConstEnvironment> env = factory->openEnvironment();
        if (env)
        {
            Owned<IPropertyTree> root = &env->getPTree();
            IPropertyTree *pt = root->queryPropTree("Software/SashaServerProcess[1]/Instance[1]");
            if (pt)
                ep.set(pt->queryProp("@netAddress"), pt->getPropInt("@port",DEFAULT_SASHA_PORT));
        }
    }*/

    IEspECLWorkunit *createArchivedWUEntry(StringArray& wuDataArray)
    {
        Owned<IEspECLWorkunit> info= createECLWorkunit();
        info->setWuid(wuDataArray.item(0));

        const char* owner = wuDataArray.item(1);
        const char* jobName = wuDataArray.item(2);
        const char* cluster = wuDataArray.item(3);
        const char* state = wuDataArray.item(4);
        if (!isEmptyString(owner))
            info->setOwner(owner);
        if (!isEmptyString(jobName))
            info->setJobname(jobName);
        if (!isEmptyString(cluster))
            info->setCluster(cluster);
        if (!isEmptyString(state))
            info->setState(state);
        return info.getClear();
    }
public:

    CSashaClient()
    {
        SocketEndpoint ep;
        ep.set(".", 8877);
        sashaserver.setown(createINode(ep));
    }
    void getWUs(unsigned type, unsigned from, unsigned numWUs, const char *wuid, const char *cluster,
        const char *owner, const char *job, const char *state, const char *startDate, const char *endDate,
        IArrayOf<IEspECLWorkunit> &wus)
    {
#ifdef MY_DEBUG
        printf("Enter getWUs.\n");
#endif
        Owned<ISashaCommand> cmd = createSashaCommand();
        cmd->setAction(SCA_LIST);
        cmd->setOutputFormat("owner,jobname,cluster,state");
        if (type == 0)
        {
            cmd->setOnline(false);
            cmd->setArchived(true);
        }
        else if (type == 1)
        {
            cmd->setOnline(true);
            cmd->setArchived(false);
        }
        else
        {
            cmd->setOnline(true);
            cmd->setArchived(true);
        }
        cmd->setStart(from);
        cmd->setLimit(numWUs);
        if (!isEmptyString(wuid))
            cmd->addId(wuid);
        if (!isEmptyString(cluster))
            cmd->setCluster(cluster);
        if (!isEmptyString(owner))
            cmd->setOwner(owner);
        if (!isEmptyString(job))
            cmd->setJobName(job);
        if (!isEmptyString(state))
            cmd->setState(state);
        if (!isEmptyString(startDate))
            cmd->setAfter(startDate);
        if (!isEmptyString(endDate))
            cmd->setBefore(endDate);

        if (!cmd->send(sashaserver, 1*60*1000))
        {
            StringBuffer msg("Cannot connect to archive server at ");
            sashaserver->endpoint().getUrlStr(msg);

#ifdef MY_DEBUG
            printf("%s\n", msg.str());
#endif
            throwUnexpected();
       }

       unsigned numberOfWUsReturned = cmd->numIds();
       for (unsigned i=0; i<numberOfWUsReturned; i++)
       {
           const char *csline = cmd->queryId(i);
           if (!csline || !*csline)
               continue;

           StringArray wuDataArray;
           wuDataArray.appendList(csline, ",");

           const char* wuid = wuDataArray.item(0);
           if (isEmptyString(wuid))
           {
               WARNLOG("Empty WUID in SCA_LIST response");
#ifdef MY_DEBUG
               printf("Empty WUID in SCA_LIST response");
#endif
               throwUnexpected();
           }
           Owned<IEspECLWorkunit> info = createArchivedWUEntry(wuDataArray);
           wus.append(*info.getClear());
       }

#ifdef MY_DEBUG
       printf("Leave getWUs.\n");
#endif
    }
};

class CESPClient
{
    Owned<IClientWsWorkunits> wuclient;

public:

    CESPClient()
    {
        wuclient.setown(createWorkunitsClient());
    }
    void createAEclWU(StringBuffer &wuid)
    {
#ifdef MY_DEBUG
        printf("Enter createAEclWU.\n");
#endif
        Owned<IClientWUCreateRequest> creq = wuclient->createWUCreateRequest();
        Owned<IClientWUCreateResponse> cresp = wuclient->WUCreate(creq);
        const IMultiException* excep = &cresp->getExceptions();
        if(excep != NULL && excep->ordinality() > 0)
        {
            StringBuffer msg;
            excep->errorMessage(msg);
            printf("%s\n", msg.str());
            throwUnexpected();
        }

        IConstECLWorkunit* wu = &cresp->getWorkunit();
        if(!wu)
        {
            printf("can't create workunit\n");
            throwUnexpected();
        }
        wuid.set(wu->getWuid());
#ifdef MY_DEBUG
        printf("%s created\n", wuid.str());
#endif
    }
    void deleteAEclWU(const char *wuid)
    {
        Owned<IClientWUDeleteRequest> creq = wuclient->createWUDeleteRequest();
        Owned<IClientWUDeleteResponse> cresp = wuclient->WUDelete(creq);
        const IMultiException* excep = &cresp->getExceptions();
        if(excep != NULL && excep->ordinality() > 0)
        {
            StringBuffer msg;
            excep->errorMessage(msg);
            printf("%s\n", msg.str());
            throwUnexpected();
        }
    }
    void deleteEclWUs(StringArray &wuids)
    {
#ifdef MY_DEBUG
        printf("Enter deleteEclWUs.\n");
#endif
        Owned<IClientWUDeleteRequest> creq = wuclient->createWUDeleteRequest();
        creq->setWuids(wuids);
        creq->setBlockTillFinishTimer(-1);
        Owned<IClientWUDeleteResponse> cresp = wuclient->WUDelete(creq);
        const IMultiException* excep = &cresp->getExceptions();
        if(excep != NULL && excep->ordinality() > 0)
        {
            StringBuffer msg;
            excep->errorMessage(msg);
            printf("%s\n", msg.str());
            throwUnexpected();
        }
#ifdef MY_DEBUG
        printf("deleteEclWUs done\n");
#endif
    }
    void updateEclWUs(StringArray &wuids)
    {
#ifdef MY_DEBUG
        printf("Enter updateEclWUs.\n");
#endif
        Owned<IClientWUActionRequest> creq = wuclient->createWUActionRequest();
        creq->setWuids(wuids);
        creq->setWUActionType("SetToFailed");
        creq->setBlockTillFinishTimer(-1);
        Owned<IClientWUActionResponse> cresp = wuclient->WUAction(creq);
        const IMultiException* excep = &cresp->getExceptions();
        if(excep != NULL && excep->ordinality() > 0)
        {
            StringBuffer msg;
            excep->errorMessage(msg);
            printf("%s\n", msg.str());
            throwUnexpected();
        }
#ifdef MY_DEBUG
        printf("updateEclWUs done\n");
#endif
    }
};

class CSashaWUiterateTests : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(CSashaWUiterateTests);
        CPPUNIT_TEST(testInit);
        CPPUNIT_TEST(testWUiterate);
    CPPUNIT_TEST_SUITE_END();

    CSashaClient sashaClient;
    CESPClient espClient;
    StringArray newWUIDs;

    const IContextLogger &logctx;
    unsigned testNum = 1;
    StringAttr testString;

    const char *getTestString() { return testString.get(); };
public:
    CSashaWUiterateTests() : logctx(queryDummyContextLogger())
    {
    }
    ~CSashaWUiterateTests()
    {
        ;//daliClientEnd();
        releaseAtoms();
        stopMPServer();
    }
    void testInit()
    {
        ;//daliClientInit();
        InitModuleObjects();
        startMPServer(0);
    }

    void testWUiterate()
    {
#ifdef MY_DEBUG
        //logctx.CTXLOG("Enter testWUiterate.\n"); //Not sure why logctx.CTXLOG does not work
        printf("\nEnter testWUiterate.\n");
#endif

        setupABC(logctx, testNum, testString);

#ifdef MY_DEBUG
        printf("Start testing\n");
#endif
        StringBuffer wuid1;
        espClient.createAEclWU(wuid1);
        if (!wuid1.isEmpty())
            newWUIDs.append(wuid1.str());

        IArrayOf<IEspECLWorkunit> wus;
        sashaClient.getWUs(1, 0, 200, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, wus);

        if (newWUIDs.length() != 0)
        {
#ifdef MY_DEBUG
            ForEachItemIn(i, wus)
            {
                IEspECLWorkunit &item = wus.item(i);
                const char *wuid = item.getWuid();
                ForEachItemIn(ii, newWUIDs)
                {
                    if (strieq(wuid, newWUIDs.item(ii)))
                        printf("Found %s\n", wuid);
                }
            }
#endif
            espClient.updateEclWUs(newWUIDs);
            espClient.deleteEclWUs(newWUIDs);
        }

        const char *str = getTestString();
        ASSERT(nullptr != str && "The testString should not be empty.");
        ASSERT(0 != streq(str, "abc") && "The testString should be 'abc'.");

        ASSERT(0 == testNum && "testNum should be 0.");

#ifdef MY_DEBUG
        printf("End testing\n");
        printf("Leave testWUiterate.\n");
#endif
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( CSashaWUiterateTests );
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION( CSashaWUiterateTests, "CSashaWUiterateTests" );


#endif // _USE_CPPUNIT
