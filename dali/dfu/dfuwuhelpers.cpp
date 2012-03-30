/*##############################################################################

    Copyright (C) 2011 HPCC Systems.

    All rights reserved. This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
############################################################################## */

#ifndef dfuwu_decl
#define dfuwu_decl __declspec(dllexport)
#endif

#include "jlib.hpp"
#include "environment.hpp"
#include "dalienv.hpp"
#include "dadfs.hpp"
#include "dautils.hpp"
#include "dfuwu.hpp"
#include "dfuwuhelpers.hpp"

StringBuffer& constructFileMask(const char* filename, StringBuffer& filemask)
{
    filemask.clear().append(filename).toLowerCase().append("._$P$_of_$N$");
    return filemask;
}

void getClusterFromLFN(const char* lfn, StringBuffer& cluster, const char* username, const char* passwd)
{
    Owned<IUserDescriptor> udesc;
    if(username != NULL && *username != '\0')
    {
        udesc.setown(createUserDescriptor());
        udesc->set(username, passwd);
    }

    Owned<IDistributedFile> df = queryDistributedFileDirectory().lookup(lfn, udesc) ;
    if(!df)
        throw MakeStringException(-1,"Could not find logical file");
    df->getClusterName(0,cluster);    
}

bool ParseLogicalPath(IPropertyTree* directories, const char * pLogicalPath, const char* cluster, StringBuffer &folder, StringBuffer &title, StringBuffer &defaultFolder, StringBuffer &defaultReplicateFolder)
{
    if(!pLogicalPath || !*pLogicalPath)
        return false;

    folder.clear();
    title.clear();

    defaultFolder.clear();
    defaultReplicateFolder.clear();
    DFD_OS os = DFD_OSdefault;

    if(cluster != NULL && *cluster != '\0')
    {
        Owned<IGroup> group = queryNamedGroupStore().lookup(cluster);
        if (group) {
            switch (queryOS(group->queryNode(0).endpoint())) {
            case MachineOsW2K:
                os = DFD_OSwindows; break;
            case MachineOsSolaris:
            case MachineOsLinux:
                os = DFD_OSunix; break;
            }
            if (directories) {
                getConfigurationDirectory(directories, "data", "thor", cluster, defaultFolder);
                getConfigurationDirectory(directories, "mirror", "thor", cluster, defaultReplicateFolder);
            }
        }
        else 
        {
            // Error here?
        }
    }

    makePhysicalPartName(pLogicalPath,0,0,folder,false,os,defaultFolder.str());
    
    const char *n = pLogicalPath;
    const char* p;
    do {
        p = strstr(n,"::");
        if(p)
            n = p+2;
    } while(p);
    title.append(n);
    return true;
}

DFUclusterPartDiskMapping readClusterMappingSettings(const char *cluster, StringBuffer &dir, int& offset)
{
    DFUclusterPartDiskMapping mapping = DFUcpdm_c_only;
    Owned<IEnvironmentFactory> envFactory = getEnvironmentFactory();
    envFactory->validateCache();

    StringBuffer dirxpath;
    dirxpath.appendf("Software/RoxieCluster[@name=\"%s\"]",cluster);
    Owned<IConstEnvironment> constEnv = envFactory->openEnvironmentByFile();
    Owned<IPropertyTree> pEnvRoot = &constEnv->getPTree();
    Owned<IPropertyTreeIterator> processes = pEnvRoot->getElements(dirxpath);
    if (processes->first())
    {
        IPropertyTree &processe = processes->query();
        const char *slaveConfig = processe.queryProp("@slaveConfig");
        if (slaveConfig && *slaveConfig)
        {
            if (!stricmp(slaveConfig, "simple"))
            {
                mapping = DFUcpdm_c_only;
            }
            else if (!stricmp(slaveConfig, "overloaded"))
            {
                mapping = DFUcpdm_c_then_d;
            }
            else if (!stricmp(slaveConfig, "full_redundancy"))
            {
                ;
            }
            else //circular redundancy
            {
                mapping = DFUcpdm_c_replicated_by_d;
                offset = processe.getPropInt("@cyclicOffset");
            }
            dir = processe.queryProp("@slaveDataDir");
        }
        else
        {
            DBGLOG("Failed to get RoxieCluster settings");
            throw MakeStringException(-1, "Failed to get RoxieCluster settings. The workunit will not be created.");
        }
    }
    else
    {
        DBGLOG("Failed to get RoxieCluster settings");
        throw MakeStringException(-1, "Failed to get RoxieCluster settings. The workunit will not be created.");
    }

    return mapping;
}

extern dfuwu_decl void CreateSprayFileWorkunit(DFUspraytype sprayType, WsFSSprayInput& input, StringBuffer& wuid)
{
    const char* destCluster = input.destGroup.str();
    if(destCluster == NULL || *destCluster == '\0')
        throw MakeStringException(-1, "Destination cluster/group not specified.");

    char* destname = (char*) input.destLogicalName.str();
    if(!destname || !*destname)
        throw MakeStringException(-1, "Destination file not specified.");

    CDfsLogicalFileName lfn;
    if (!lfn.setValidate(destname))
        throw MakeStringException(-1, "Invalid destination filename");
    destname = (char*) lfn.get();

    if(input.sourceXML.length() == 0)
    {
        if(input.sourceIP.length() < 1)
            throw MakeStringException(-1, "Source network IP not specified.");
        if(input.sourcePath.length() < 1)
            throw MakeStringException(-1, "Source file not specified.");
    }

    if((sprayType == DFUsprayfixed) && (input.sourceRecordSize == 0) && !input.options.noSplit)             // -ve record sizes for blocked
        throw MakeStringException(-1, "Invalid record size"); 

    StringBuffer gName, ipAddr;
    const char *pTr = strchr(destCluster, ' ');
    if (pTr)
    {
        gName.append(pTr - destCluster, destCluster);
        ipAddr.append(pTr+1);
    }
    else
        gName.append(destCluster);

    StringBuffer destFolder, destTitle, defaultFolder, defaultReplicateFolder;
    if (ipAddr.length() > 0)
        ParseLogicalPath(NULL, destname, ipAddr.str(), destFolder, destTitle, defaultFolder, defaultReplicateFolder);
    else
        ParseLogicalPath(NULL, destname, destCluster, destFolder, destTitle, defaultFolder, defaultReplicateFolder);

    Owned<IDFUWorkUnitFactory> factory = getDFUWorkUnitFactory();
    Owned<IDFUWorkUnit> wu = factory->createWorkUnit();

    wu->setClusterName(gName.str());
    wu->setJobName(destTitle.str());
    wu->setQueue(input.queueLabel.str());
    wu->setUser(input.userName.str());
    wu->setPassword(input.password.str());
    wu->setCommand(DFUcmd_import);

    IDFUfileSpec *source = wu->queryUpdateSource();
    if(input.sourceXML.length() == 0)
    {
        RemoteMultiFilename rmfn;
        SocketEndpoint ep(input.sourceIP.str());
        rmfn.setEp(ep);
        StringBuffer fnamebuf(input.sourcePath.str());
        fnamebuf.trim();
        rmfn.append(fnamebuf.str());    // handles comma separated files
        source->setMultiFilename(rmfn);
    }
    else
    {
        input.sourceXML.append('\0');
        source->setFromXML((const char*)input.sourceXML.toByteArray());
    }


    IDFUfileSpec *destination = wu->queryUpdateDestination();
    IDFUoptions *options = wu->queryUpdateOptions();
    if(sprayType == DFUsprayfixed)
    {
        if(input.sourceRecordSize > 0)
            source->setRecordSize(input.sourceRecordSize);
        else if (input.sourceRecordSize == RECFMVB_RECSIZE_ESCAPE) 
        {
            source->setFormat(DFUff_recfmvb);
            destination->setFormat(DFUff_variable);
        }
        else if (input.sourceRecordSize == RECFMV_RECSIZE_ESCAPE) 
        {
            source->setFormat(DFUff_recfmv);
            destination->setFormat(DFUff_variable);
        }
        else if (input.sourceRecordSize == PREFIX_VARIABLE_RECSIZE_ESCAPE) 
        {
            source->setFormat(DFUff_variable);
            destination->setFormat(DFUff_variable);
        }
        else if (input.sourceRecordSize == PREFIX_VARIABLE_BIGENDIAN_RECSIZE_ESCAPE) 
        {
            source->setFormat(DFUff_variablebigendian);
            destination->setFormat(DFUff_variable);
        }
    }
    else
    {
        source->setMaxRecordSize(input.sourceRecordSize);
        source->setFormat((DFUfileformat)input.sourceFormat);

        // if rowTag specified, it means it's xml format, otherwise it's csv
        if(input.sourceRowTag.length() > 0)
        {
            source->setRowTag(input.sourceRowTag.str());
            options->setKeepHeader(true);
        }
        else
        {
            const char* cs = input.sourceCsvSeparate.str();
            if(cs == NULL || *cs == '\0')
                cs = "\\,";
            const char* ct = input.sourceCsvTerminate.str();
            if(ct == NULL || *ct == '\0')
                ct = "\\n,\\r\\n";
            const char* cq = input.sourceCsvQuote.str();
            if(cq== NULL)
                cq = "'";
            source->setCsvOptions(cs, ct, cq);
        }
    }

    destination->setLogicalName(destname);
    destination->setDirectory(destFolder.str());

    StringBuffer fileMask;
    constructFileMask(destTitle.str(), fileMask);
    destination->setFileMask(fileMask.str());
    destination->setGroupName(gName.str());

    ClusterPartDiskMapSpec mspec;
    destination->getClusterPartDiskMapSpec(gName.str(), mspec);
    mspec.setDefaultBaseDir(defaultFolder.str());
    mspec.setDefaultReplicateDir(defaultReplicateFolder.str());
    destination->setClusterPartDiskMapSpec(gName.str(), mspec);

    if(input.compress || (input.encryptKey.length() > 0))
        destination->setCompressed(true);

    if (input.replicateOffset != 1)
        destination->setReplicateOffset(input.replicateOffset);

    if((sprayType == DFUsprayfixed) && input.wrap)
        destination->setWrap(true);
    
    options->setReplicate(input.options.replicate);
    options->setOverwrite(input.options.overwrite);             // needed if target already exists
    if ((input.encryptKey.length() > 0)||(input.decryptKey.length() > 0))
        options->setEncDec(input.encryptKey.str(),input.decryptKey.str());
    if(input.options.prefix.length() > 0)
        options->setLengthPrefix(input.options.prefix.str());
    if(input.options.noSplit)
        options->setNoSplit(true);
    if(input.options.noRecover)
        options->setNoRecover(true);
    if(input.options.maxConnections > 0)
        options->setmaxConnections(input.options.maxConnections);
    if(input.options.throttle > 0)
        options->setThrottle(input.options.throttle);
    if(input.options.transferBufferSize > 0)
        options->setTransferBufferSize(input.options.transferBufferSize);
    if (input.options.pull)
        options->setPull(true);
    if (input.options.push)
        options->setPush(true);

    wuid.append(wu->queryId());
    submitDFUWorkUnit(wu.getClear());

    return;
}

extern dfuwu_decl void CreateCopyFileWorkunit(WsFSCopyInput& input, StringBuffer& wuid)
{
    const char* srcname = input.sourceLogicalName.str();
    const char* dstname = input.destLogicalName.str();
    if(!srcname || !*srcname)
        throw MakeStringException(-1, "Source logical file not specified.");
    if(!dstname || !*dstname)
        throw MakeStringException(-1, "Destination logical file not specified.");

    CDfsLogicalFileName lfn;
    if (!input.isRoxie)
    {
        if (!lfn.setValidate(dstname))
            throw MakeStringException(-1, "invalid destination filename");
        dstname = lfn.get();
    }

    StringBuffer destCluster = input.destGroup;
    if(destCluster.length() < 0)
    {
        getClusterFromLFN(srcname, destCluster, input.userName.str(), input.password.str());
    }

    StringBuffer destFolder, destTitle, defaultFolder, defaultReplicateFolder;
    ParseLogicalPath(input.directories.get(), dstname, destCluster.str(), destFolder, destTitle, defaultFolder, defaultReplicateFolder);

    StringBuffer fileMask; 
    constructFileMask(destTitle.str(), fileMask);

    Owned<IDFUWorkUnitFactory> factory = getDFUWorkUnitFactory();
    Owned<IDFUWorkUnit> wu = factory->createWorkUnit();
    wu->setJobName(dstname);
    wu->setQueue(input.queueLabel.str());

    wu->setUser(input.userName.str());
    wu->setPassword(input.password.str());
    if(destCluster.length() > 0)
        wu->setClusterName(destCluster.str());

    if (input.superCopy) 
    {
        StringBuffer u(input.userName);
        StringBuffer p(input.password);
        Owned<INode> foreigndali;
        if (input.sourceDali.length() > 0)
        {
            SocketEndpoint ep(input.sourceDali.str());
            foreigndali.setown(createINode(ep));
            const char* srcu = input.sourceUserName.str();
            if(srcu && *srcu)
            {
                u.clear().append(srcu);
                p.clear().append(input.sourcePassword.str());
            }
        }
        Owned<IUserDescriptor> udesc=createUserDescriptor();
        udesc->set(u.str(),p.str());
        if (!queryDistributedFileDirectory().isSuperFile(srcname,foreigndali,udesc))
            input.superCopy = false;
    }

    if (input.superCopy)
        wu->setCommand(DFUcmd_supercopy);
    else
        wu->setCommand(DFUcmd_copy);

    IDFUfileSpec *source = wu->queryUpdateSource();
    IDFUfileSpec *destination = wu->queryUpdateDestination();
    IDFUoptions *options = wu->queryUpdateOptions();

    source->setLogicalName(srcname);
    if(input.sourceDali.length() > 0)
    {
        SocketEndpoint ep(input.sourceDali.str());
        source->setForeignDali(ep);

        if(input.sourceUserName.length() > 0)
            source->setForeignUser(input.sourceUserName.str(), input.sourcePassword.str());
    }

    destination->setLogicalName(dstname);
    destination->setFileMask(fileMask.str());
    options->setOverwrite(input.options.overwrite);

    const char * encryptkey = input.encryptKey.str();
    const char * decryptkey = input.decryptKey.str();
    if ((encryptkey&&*encryptkey)||(decryptkey&&*decryptkey))
        options->setEncDec(encryptkey,decryptkey);

    if (input.isRoxie)
    {
        int offset;
        StringBuffer slaveDataDir;
        DFUclusterPartDiskMapping mapping = readClusterMappingSettings(destCluster, slaveDataDir, offset);
        destination->setClusterPartDiskMapping(mapping, slaveDataDir, destCluster.str(), true);  // repeat last part

        destination->setRoxiePrefix(destCluster);       // added to start of each file name
        destination->setWrap(true);                     // roxie always wraps
        if(input.compress)
            destination->setCompressed(true);

        options->setReplicate(mapping==DFUcpdm_c_replicated_by_d);
        if (input.superCopy)
            options->setSuppressNonKeyRepeats(true);  // only repeat last part when src kind = key
    }
    else
    {
        destination->setGroupName(destCluster.str());
        destination->setDirectory(destFolder.str());
        destination->setWrap(input.wrap);
        if (input.sourceDiffKeyName.length() > 0)
            source->setDiffKey(input.sourceDiffKeyName.str());
        if (input.destDiffKeyName.length() > 0)
            destination->setDiffKey(input.destDiffKeyName.str());

        ClusterPartDiskMapSpec mspec;
        destination->getClusterPartDiskMapSpec(destCluster.str(), mspec);
        mspec.setDefaultBaseDir(defaultFolder.str());
        mspec.setDefaultReplicateDir(defaultReplicateFolder.str());
        destination->setClusterPartDiskMapSpec(destCluster.str(), mspec);

        if(input.compress||(encryptkey&&*encryptkey))
            destination->setCompressed(true);

        options->setReplicate(input.options.replicate);
        if(input.options.noRecover)
            options->setNoRecover(true);
        if(input.options.noSplit)
            options->setNoSplit(true);
        else
            options->setNoSplit(false);
        if(input.options.maxConnections > 0)
            options->setmaxConnections(input.options.maxConnections);
        if(input.options.throttle > 0)
            options->setThrottle(input.options.throttle);
        if(input.options.transferBufferSize > 0)
            options->setTransferBufferSize(input.options.transferBufferSize);
        if (input.options.pull)
            options->setPull(true);
        if (input.options.push)
            options->setPush(true);
        if (input.options.ifnewer)
            options->setIfNewer(true);
    }

    wuid.append(wu->queryId());
    submitDFUWorkUnit(wu.getClear());

    return;
}