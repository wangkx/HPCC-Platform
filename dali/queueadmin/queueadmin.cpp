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

#include "platform.h"
//#include "portlist.h"
#include "jlib.hpp"
#include "jmisc.hpp"
#include "jfile.hpp"
#include "jptree.hpp"
#include "jarray.hpp"
#include "jencrypt.hpp"
#include "jexcept.hpp"

#include "environment.hpp"
#include "dautils.hpp"
#include "dasds.hpp"
#include "dalienv.hpp"
#include "daclient.hpp"
#include "workunit.hpp"
#include "wujobq.hpp"

#define SDS_LOCK_TIMEOUT  60000
#define SSTterm  4
static const char *StatusServerTypeNames[] = { "", "HThorServer", "RoxieServer", "ThorMaster" };

const char *getStatusServerTypeName(ClusterType type)
{
    return (type < SSTterm) ? StatusServerTypeNames[type] : NULL;
}

void usage(const char *exe)
{
  printf("Usage:\n");
  printf("  %s <daliserver-ip> { <option> }\n", exe);
  printf("\n");
  printf("  dali=<daliserver-ip>         \n");
  printf("  cluster=<target cluster name>\n");
}

void printVersion()
{
    printf("queueadmin Version: 0.1\n");
}

bool build_globals(int argc, const char *argv[], IProperties * globals)
{
    int i;

    for(i = 0; i < argc; i++)
    {
        if(argv[i] != NULL && argv[i][0] == '@' && argv[i][1] != '\0')
        {
            globals->loadFile(argv[i]+1);
        }
    }

    for (i = 1; i < argc; i++)
    {
        if (strchr(argv[i],'='))
        {
            globals->loadProp(argv[i]);
        }
    }

    return true;
}

class CTargetCluster : public CInterface
{
public:
    ClusterType clusterType;
    StringAttr clusterName;
    StringAttr queueName;
    SCMStringBuffer statusServerName;
    SCMStringBuffer clusterQueueName;
    SCMStringBuffer agentQueueName;
    SCMStringBuffer serverQueueName;

    CTargetCluster(){};
    virtual ~CTargetCluster(){};
};

void addQueueItem(const char* wuid, const char* queueName, const char* clusterInstance, IConstWorkUnit* cw, IPropertyTree* queueInfo)
{
    Owned<IPTree> branch = createPTree();
    branch->setProp("@wuid", wuid);
    const char* owner = cw->queryUser();
    const char* jobName = cw->queryJobName();
    const char *state = cw->queryStateDesc();
    if (owner && *owner)
        branch->setProp("owner", owner);
    if (jobName && *jobName)
        branch->setProp("jobName", jobName);
    if (state && *state)
        branch->setProp("state", state);
    if (queueName && *queueName)
        branch->setProp("queue", queueName);
    if (clusterInstance && *clusterInstance)
        branch->setProp("clusterInstance", clusterInstance);

    queueInfo->addPropTree("item", branch.getClear());
}

void readRunningWUsOnStatusServer(IPropertyTree* serverStatusRoot, CTargetCluster* targetCluster, const char* serverName, bool isECLAgent, IPropertyTree* queueInfo)
{
    const char* clusterName = targetCluster->clusterName.str();
    ClusterType clusterType = targetCluster->clusterType;
    VStringBuffer path("Server[@name=\"%s\"]", serverName);
    Owned<IPropertyTreeIterator> itrStatusServer(serverStatusRoot->getElements(path.str()));
    ForEach(*itrStatusServer)
    {
        IPropertyTree& serverStatusNode = itrStatusServer->query();

        StringBuffer serverInstance;
        if ((clusterType == ThorLCRCluster) || (clusterType == RoxieCluster))
            serverStatusNode.getProp("@cluster", serverInstance);
        else
            serverInstance.appendf("%s on %s", serverName, serverStatusNode.queryProp("@node"));

        Owned<IPropertyTreeIterator> wuids(serverStatusNode.getElements("WorkUnit"));
        ForEach(*wuids)
        {
            const char* wuid=wuids->query().queryProp(NULL);
            if (!wuid || !*wuid)
                continue;

            Owned<IWorkUnitFactory> factory = getWorkUnitFactory();
            Owned<IConstWorkUnit> cw = factory->openWorkUnit(wuid);
            if (!cw)
                continue;

            const char* targetClusterName = cw->queryClusterName();
            if (!strieq(clusterName, targetClusterName))
                continue;

            DBGLOG("Running WU %s", wuid);

            StringBuffer queueName;
            if (!isECLAgent)
            {
                const char *cluster = serverStatusNode.queryProp("Cluster");
                if (cluster)
                    getClusterThorQueueName(queueName, cluster);
                else
                    queueName.append(targetCluster->queueName.get());
            }

            addQueueItem(wuid, queueName.str(), serverInstance.str(), cw, queueInfo);
        }
    }
}

bool readJobQueue(IJQSnapshot* jobQueueSnapshot, const char* queueName, IPropertyTree* queueInfo)
{
    if (!queueName || !*queueName)
        return false;

    if (!jobQueueSnapshot)
    {
        WARNLOG("readJobQueue: jobQueueSnapshot not found.");
        return false;
    }

    Owned<IJobQueueConst> jobQueue = jobQueueSnapshot->getJobQueue(queueName);
    if (!jobQueue)
    {
        WARNLOG("readJobQueue: failed to get info for job queue %s", queueName);
        return false;
    }

    CJobQueueContents queuedJobs;
    StringBuffer state, stateDetails;
    jobQueue->copyItemsAndState(queuedJobs, state, stateDetails);

    Owned<IJobQueueIterator> iter = queuedJobs.getIterator();
    ForEach(*iter)
    {
        const char* wuid = iter->query().queryWUID();
        if (!wuid || !*wuid)
            continue;

        Owned<IWorkUnitFactory> factory = getWorkUnitFactory();
        Owned<IConstWorkUnit> cw = factory->openWorkUnit(wuid);
        if (!cw)
            continue;

        DBGLOG("queued WU %s at %s", wuid, queueName);
        addQueueItem(wuid, queueName, NULL, cw, queueInfo);
    }
    return true;
}


void readTargetClusterInfo(IConstWUClusterInfo* clusterSettings, CTargetCluster* targetCluster)
{
    if (!clusterSettings)
        throw MakeStringException(-1,"Failed to get cluster settings.");

    SCMStringBuffer clusterName;
    clusterSettings->getName(clusterName);
    targetCluster->clusterName.set(clusterName.str());
    targetCluster->clusterType = clusterSettings->getPlatform();
    const char* statusServerName = getStatusServerTypeName(targetCluster->clusterType);
    if (!statusServerName || !*statusServerName)
        throw MakeStringException(-1, "Cluster type not found for %s.", clusterName.str());

    targetCluster->statusServerName.set(statusServerName);
    clusterSettings->getServerQueue(targetCluster->serverQueueName);
    clusterSettings->getAgentQueue(targetCluster->agentQueueName);

    if (targetCluster->clusterType == ThorLCRCluster)
    {
        clusterSettings->getThorQueue(targetCluster->clusterQueueName);
        targetCluster->queueName.set(targetCluster->clusterQueueName.str());
    }
    else
        targetCluster->queueName.set(targetCluster->agentQueueName.str());
}

void readQueueInfo(const char* cluster, IPropertyTree* queueInfo)
{
    Owned<CTargetCluster> targetCluster = new CTargetCluster();
    readTargetClusterInfo(getTargetClusterInfo(cluster), targetCluster);

    {
        Owned<IRemoteConnection> connStatusServers = querySDS().connect("/Status/Servers",myProcessSession(),RTM_LOCK_READ,SDS_LOCK_TIMEOUT);
        if (!connStatusServers)
            throw MakeStringException(-1, "Failed to get status server information.");

        //Read running WUs
        readRunningWUsOnStatusServer(connStatusServers->queryRoot(), targetCluster, targetCluster->statusServerName.str(), false, queueInfo);
        if (targetCluster->clusterType == HThorCluster)
            readRunningWUsOnStatusServer(connStatusServers->queryRoot(), targetCluster, "ECLagent", true, queueInfo);
    }

    //Read queued WUs
    Owned<IJQSnapshot> jobQueueSnapshot = createJQSnapshot();
    readJobQueue(jobQueueSnapshot, targetCluster->queueName.str(), queueInfo);
    readJobQueue(jobQueueSnapshot, targetCluster->serverQueueName.str(), queueInfo);
}

void initDali(const char *servers)
{
    if (servers!=NULL && *servers!=0 && !daliClientActive())
    {
        DBGLOG("Initializing DALI client [servers = %s]", servers);

        // Create server group
        Owned<IGroup> serverGroup = createIGroup(servers, DALI_SERVER_PORT);

        if (!serverGroup)
            throw MakeStringException(0, "Could not instantiate dali IGroup");

        // Initialize client process
        if (!initClientProcess(serverGroup, DCR_EspServer))
            throw MakeStringException(0, "Could not initialize dali client");
        setPasswordsFromSDS();
    }
}

int main(int argc, const char* argv[])
{
    int ret = 0;
    InitModuleObjects();

    if ((argc >= 2) && ((stricmp(argv[1], "/version") == 0) || (stricmp(argv[1], "-v") == 0)
        || (stricmp(argv[1], "--version") == 0)))
    {
        printVersion();
        return 0;
    }

    Owned<IFile> inifile = createIFile("queueadmin.ini");
    if(argc < 2 && !(inifile->exists() && inifile->size() > 0))
    {
        usage("queueadmin");
        return 0;
    }

    if ((argc >= 2) && ((argv[1][0]=='/' || argv[1][0]=='-') && (argv[1][1]=='?' || argv[1][1]=='h')))
    {
        usage("queueadmin");
        return 0;
    }
    Owned<IProperties> globals = createProperties("queueadmin.ini", true);
    if(!build_globals(argc, argv, globals))
    {
        fprintf(stderr, "ERROR: Invalid command syntax.\n");
        releaseAtoms();
        return -1;
    }

    initDali(globals->queryProp("dali"));

    StringBuffer result;
    Owned<IPropertyTree> resultTree = createPTree("Result");
    readQueueInfo(globals->queryProp("cluster"), resultTree);
    toXML(resultTree, result);
    fprintf(stdout, "%s\n", result.str());

    closedownClientProcess();   // dali client closedown
    releaseAtoms();
    fflush(stdout);
    fflush(stderr);
    return ret;
}

