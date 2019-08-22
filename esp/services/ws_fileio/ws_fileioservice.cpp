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

#pragma warning (disable : 4786)
#pragma warning (disable : 4129)

#include "jiface.hpp"
#include "environment.hpp"
#include "ws_fileioservice.hpp"
#ifdef _WIN32
#include "windows.h"
#endif
#include "exception_util.hpp"

#define FILE_IO_URL     "FileIOAccess"

void CWsFileIOEx::init(IPropertyTree *cfg, const char *process, const char *service)
{
}

bool CWsFileIOEx::CheckServerAccess(const char* targetDZNameOrAddress, const char* relPath, StringBuffer& netAddr, StringBuffer& absPath)
{
    if (!targetDZNameOrAddress || (targetDZNameOrAddress[0] == 0) || !relPath || (relPath[0] == 0))
        return false;

    netAddr.clear();
    Owned<IEnvironmentFactory> factory = getEnvironmentFactory(true);
    Owned<IConstEnvironment> env = factory->openEnvironment();
    Owned<IConstDropZoneInfo> dropZoneInfo = env->getDropZone(targetDZNameOrAddress);
    if (!dropZoneInfo || !dropZoneInfo->isECLWatchVisible())
    {
        if (stricmp(targetDZNameOrAddress, "localhost")==0)
            targetDZNameOrAddress = ".";

        Owned<IConstDropZoneInfoIterator> dropZoneItr = env->getDropZoneIteratorByAddress(targetDZNameOrAddress);
        ForEach(*dropZoneItr)
        {
            IConstDropZoneInfo & dz = dropZoneItr->query();
            if (dz.isECLWatchVisible())
            {
                dropZoneInfo.set(&dropZoneItr->query());
                netAddr.set(targetDZNameOrAddress);
                break;
            }
        }
    }

    if (dropZoneInfo)
    {
        SCMStringBuffer directory, computerName, computerAddress;
        if (netAddr.isEmpty())
        {
            dropZoneInfo->getComputerName(computerName); //legacy structure
            if(computerName.length() != 0)
            {
                Owned<IConstMachineInfo> machine = env->getMachine(computerName.str());
                if (machine)
                {
                    machine->getNetAddress(computerAddress);
                    if (computerAddress.length() != 0)
                    {
                        netAddr.set(computerAddress.str());
                    }
                }
            }
            else
            {
                Owned<IConstDropZoneServerInfoIterator> serverIter = dropZoneInfo->getServers();
                ForEach(*serverIter)
                {
                    IConstDropZoneServerInfo &serverElem = serverIter->query();
                    serverElem.getServer(netAddr.clear());
                    if (!netAddr.isEmpty())
                        break;
                }
            }
        }

        dropZoneInfo->getDirectory(directory);
        if (directory.length() != 0)
        {
            absPath.set(directory.str());

            char absPathLastChar = absPath.charAt(absPath.length() - 1);
            const char pathSepChar = getPathSepChar(directory.str());
            if (relPath[0] != pathSepChar)
            {
                if (absPathLastChar != pathSepChar)
                    absPath.append(pathSepChar);
            }
            else
            {
                if (absPathLastChar == pathSepChar)
                    absPath.setLength(absPath.length() - 1);
            }
            absPath.append(relPath);
            return true;
        }
        else
        {
            SCMStringBuffer dropZoneName;
            ESPLOG(LogMin, "Found LZ '%s' without a directory attribute!", dropZoneInfo->getName(dropZoneName).str());
        }

    }

    return false;
}

bool CWsFileIOEx::onCreateFile(IEspContext &context, IEspCreateFileRequest &req, IEspCreateFileResponse &resp)
{
    context.ensureFeatureAccess(FILE_IO_URL, SecAccess_Write, ECLWATCH_ACCESS_TO_FILE_DENIED, "WsFileIO::CreateFile: Permission denied");

    StringBuffer result;
    const char* server = req.getDestDropZone();
    if (isEmptyString(server))
        throw MakeStringException(ECLWATCH_INVALID_INPUT, "Destination not specified");

    const char* destRelativePath = req.getDestRelativePath();
    if (isEmptyString(destRelativePath))
        throw MakeStringException(ECLWATCH_INVALID_INPUT, "Destination path not specified");

    resp.setDestDropZone(server);
    resp.setDestRelativePath(destRelativePath);

    StringBuffer destAbsPath;
    StringBuffer destNetAddr;
    if (!CheckServerAccess(server, destRelativePath, destNetAddr, destAbsPath))
        throw MakeStringException(ECLWATCH_INVALID_INPUT, "Failed to access the destination: %s %s.", server, destRelativePath);

    RemoteFilename rfn;
    SocketEndpoint ep;
    ep.set(destNetAddr);
    rfn.setPath(ep, destAbsPath);
    Owned<IFile> file = createIFile(rfn);

    fileBool isDir = file->isDirectory();
    if (isDir == foundYes)
        throw MakeStringException(ECLWATCH_INVALID_INPUT, "%s is a directory.", destRelativePath);

    if (!req.getOverwrite() && (isDir != notFound))
        throw MakeStringException(ECLWATCH_INVALID_INPUT, "%s exists.", destRelativePath);

    file->open(IFOcreate);
    result.appendf("%s has been created.", destRelativePath);
    resp.setResult(result.str());
    return true;
}

bool CWsFileIOEx::onReadFileData(IEspContext &context, IEspReadFileDataRequest &req, IEspReadFileDataResponse &resp)
{
    context.ensureFeatureAccess(FILE_IO_URL, SecAccess_Read, ECLWATCH_ACCESS_TO_FILE_DENIED, "WsFileIO::ReadFileData: Permission denied");

    StringBuffer result;
    const char* server = req.getDestDropZone();
    if (isEmptyString(server))
        throw MakeStringException(ECLWATCH_INVALID_INPUT, "Destination not specified");

    const char* destRelativePath = req.getDestRelativePath();
    if (isEmptyString(destRelativePath))
        throw MakeStringException(ECLWATCH_INVALID_INPUT, "Destination path not specified");

    resp.setDestDropZone(server);
    resp.setDestRelativePath(destRelativePath);

    StringBuffer destAbsPath;
    StringBuffer destNetAddr;
    if (!CheckServerAccess(server, destRelativePath, destNetAddr, destAbsPath))
        throw MakeStringException(ECLWATCH_INVALID_INPUT, "Failed to access the destination: %s %s.", server, destRelativePath);

    __int64 dataSize = req.getDataSize();
    if (dataSize < 1)
        throw MakeStringException(ECLWATCH_INVALID_INPUT, "Invalid data size.");

    __int64 offset = req.getOffset();
    if (offset < 0)
        throw MakeStringException(ECLWATCH_INVALID_INPUT, "Invalid offset.");

    resp.setDataSize(dataSize);
    resp.setOffset(offset);

    StringBuffer user;
    RemoteFilename rfn;
    SocketEndpoint ep;
    ep.set(destNetAddr);
    rfn.setPath(ep, destAbsPath);
    Owned<IFile> file = createIFile(rfn);
    fileBool isFile = file->isFile();
    if (isFile != foundYes)
        throw MakeStringException(ECLWATCH_INVALID_INPUT, "%s does not exist.", destRelativePath);

    Owned<IFileIO> io = file->open(IFOread);
    __int64 size = io->size();
    if (offset >= size)
        throw MakeStringException(ECLWATCH_INVALID_INPUT, "Invalid offset: file size = %" I64F "d.", size);

    __int64 dataToRead = size - offset;
    if (dataToRead > dataSize)
    {
        dataToRead = dataSize;
    }

    MemoryBuffer membuf;
    char* buf = (char*) membuf.reserve((int) dataToRead);
    if (io->read(offset, (int)dataToRead, buf) != dataToRead)
        throw MakeStringException(ECLWATCH_CANNOT_READ_FILE, "ReadFileData error.");

    resp.setData(membuf);
    resp.setResult("ReadFileData done.");
    LOG(MCprogress, unknownJob, "ReadFileData done: %s: %s %s", context.getUserID(user).str(), server, destRelativePath);

    return true;
}

bool CWsFileIOEx::onWriteFileData(IEspContext &context, IEspWriteFileDataRequest &req, IEspWriteFileDataResponse &resp)
{
    context.ensureFeatureAccess(FILE_IO_URL, SecAccess_Write, ECLWATCH_ACCESS_TO_FILE_DENIED, "WsFileIO::WriteFileData: Permission denied");

    StringBuffer result;
    const char* server = req.getDestDropZone();
    if (isEmptyString(server))
        throw MakeStringException(ECLWATCH_INVALID_INPUT, "Destination not specified");

    const char* destRelativePath = req.getDestRelativePath();
    if (isEmptyString(destRelativePath))
        throw MakeStringException(ECLWATCH_INVALID_INPUT, "Destination path not specified");

    MemoryBuffer& srcdata = (MemoryBuffer&)req.getData();
    if(srcdata.length() == 0)
        throw MakeStringException(ECLWATCH_INVALID_INPUT, "Source data not specified");

    __int64 offset = req.getOffset();
    bool append = req.getAppend();  
    if (append)
    {
        resp.setAppend(true);
    }
    else
    {
        resp.setOffset(offset);
    }

    resp.setDestDropZone(server);
    resp.setDestRelativePath(destRelativePath);

    StringBuffer destAbsPath;
    StringBuffer destNetAddr;
    if (!CheckServerAccess(server, destRelativePath, destNetAddr, destAbsPath))
        throw MakeStringException(ECLWATCH_INVALID_INPUT, "Failed to access the destination: %s %s.", server, destRelativePath);

    StringBuffer user;
    RemoteFilename rfn;
    SocketEndpoint ep;
    ep.set(destNetAddr);
    rfn.setPath(ep, destAbsPath);
    Owned<IFile> file = createIFile(rfn);
    fileBool isFile = file->isFile();
    if (isFile != foundYes)
        throw MakeStringException(ECLWATCH_INVALID_INPUT, "%s does not exist.", destRelativePath);

    if (append)
    {
        Owned<IFileIO> io = file->open(IFOread);
        offset = io->size();
    }

    Owned<IFileIO> fileio = file->open(IFOwrite);
    size32_t len = srcdata.length();
    if (fileio->write(offset, len, srcdata.readDirect(len)) != len)
        throw MakeStringException(ECLWATCH_CANNOT_WRITE_FILE, "WriteFileData error.");

    resp.setResult("WriteFileData done.");
    LOG(MCprogress, unknownJob, "WriteFileData done: %s: %s %s", context.getUserID(user).str(), server, destRelativePath);

    return true;
}


