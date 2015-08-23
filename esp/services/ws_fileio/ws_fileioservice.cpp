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
#include "thorxmlwrite.hpp"
#include "ws_fileioservice.hpp"
#ifdef _WIN32
#include "windows.h"
#endif
#ifdef _USE_ZLIB
#include "zcrypt.hpp"
#endif
#include "exception_util.hpp"

#define FILE_IO_URL     "FileIOAccess"
const char* TEMPZIPDIR = "tempzipfiles";

void CWsFileIOEx::init(IPropertyTree *cfg, const char *process, const char *service)
{
}

bool CWsFileIOEx::CheckServerAccess(const char* server, const char* relPath, StringBuffer& netAddr, StringBuffer& absPath)
{
    if (!server || (server[0] == 0) || !relPath || (relPath[0] == 0))
        return false;
    Owned<IEnvironmentFactory> factory = getEnvironmentFactory();

    Owned<IConstEnvironment> env;
    env.setown(factory->openEnvironment());

    Owned<IPropertyTree> pEnvRoot = &env->getPTree();
    IPropertyTree* pEnvSoftware = pEnvRoot->queryPropTree("Software");
    Owned<IPropertyTree> pRoot = createPTreeFromXMLString("<Environment/>");
    IPropertyTree* pSoftware = pRoot->addPropTree("Software", createPTree("Software"));
    if (pEnvSoftware && pSoftware)
    {
        Owned<IPropertyTreeIterator> it = pEnvSoftware->getElements("DropZone");
        ForEach(*it)
        {
            const char *zonename = it->query().queryProp("@computer");
            if (!strcmp(zonename, "."))
                zonename = "localhost";

            if (zonename && *zonename)
            {
                StringBuffer xpath;
                xpath.appendf("Hardware/Computer[@name='%s']/@netAddress", zonename);
                char* addr = (char*) pEnvRoot->queryProp(xpath.str());
                if (addr && *addr)
                {           
                    StringBuffer sNetAddr;
                    if (strcmp(addr, "."))
                    {       
                        sNetAddr.append(addr);
                    }
                    else
                    {
                        StringBuffer ipStr;
                        IpAddress ipaddr = queryHostIP();
                        ipaddr.getIpText(ipStr);
                        if (ipStr.length() > 0)
                        {
//#define MACHINE_IP "10.239.219.9"
#ifdef MACHINE_IP
                            sNetAddr.append(MACHINE_IP);
#else
                            sNetAddr.append(ipStr.str());
#endif
                        }
                    }
                    bool dropzoneFound = false;
                    if (!stricmp(zonename, server))
                    {
                        dropzoneFound = true;
                    }
                    else if (!stricmp(sNetAddr.str(), server))
                    {
                        dropzoneFound = true;
                    }
                    
                    if (!dropzoneFound)
                    {
                        continue;
                    }

                    char ch = '\\';
                    Owned<IConstMachineInfo> machine = env->getMachineByAddress(addr);
                    //Owned<IConstEnvironment> env = factory->openEnvironment();
                    //Owned<IConstMachineInfo> machine = getMachineByAddress(pEnvRoot, env, addr);

                    if (machine && (machine->getOS() == MachineOsLinux || machine->getOS() == MachineOsSolaris))
                    {
                        ch = '/';
                    }
                    
                    StringBuffer dir;
                    IPropertyTree* pDropZone = pSoftware->addPropTree("DropZone", &it->get());
                    pDropZone->getProp("@directory", dir);
                    if (dir.length() > 0)
                    {
                        if (relPath[0] != ch)
                        {
                            absPath.appendf("%s%c%s", dir.str(), ch, relPath);
                        }
                        else
                        {
                            absPath.appendf("%s%s", dir.str(), relPath);
                        }
                        netAddr = sNetAddr;
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

bool CWsFileIOEx::onCreateFile(IEspContext &context, IEspCreateFileRequest &req, IEspCreateFileResponse &resp)
{
    context.validateFeatureAccess(FILE_IO_URL, SecAccess_Write, true);

    StringBuffer result;
    const char* server = req.getDestDropZone();
    if (!server || (server[0] == 0))
    {
        resp.setResult("Destination not specified");
        return true;
    }

    const char* destRelativePath = req.getDestRelativePath();
    if (!destRelativePath || (destRelativePath[0] == 0))
    {
        resp.setResult("Destination path not specified");
        return true;
    }

    resp.setDestDropZone(server);
    resp.setDestRelativePath(destRelativePath);

    StringBuffer destAbsPath;
    StringBuffer destNetAddr;
    if (!CheckServerAccess(server, destRelativePath, destNetAddr, destAbsPath))
    {
        result.appendf("Failed to access the destination: %s %s.", server, destRelativePath);
        resp.setResult(result.str());
        return true;
    }

    RemoteFilename rfn;
    SocketEndpoint ep;
    ep.set(destNetAddr);
    rfn.setPath(ep, destAbsPath);
    Owned<IFile> file = createIFile(rfn);

    fileBool isDir = file->isDirectory();
    if (isDir == foundYes)
    {
        result.appendf("Failure: %s is a directory.", destRelativePath);
        resp.setResult(result.str());
        return true;
    }

    if (!req.getOverwrite() && (isDir != notFound))
    {
        result.appendf("Failure: %s exists.", destRelativePath);
        resp.setResult(result.str());
        return true;
    }

    file->open(IFOcreate);
    result.appendf("%s has been created.", destRelativePath);
    resp.setResult(result.str());
    return true;
}

bool CWsFileIOEx::onReadFileData(IEspContext &context, IEspReadFileDataRequest &req, IEspReadFileDataResponse &resp)
{
    context.validateFeatureAccess(FILE_IO_URL, SecAccess_Read, true);

    StringBuffer result;
    const char* server = req.getDestDropZone();
    if (!server || (server[0] == 0))
    {
        resp.setResult("Destination not specified");
        return true;
    }

    const char* destRelativePath = req.getDestRelativePath();
    if (!destRelativePath || (destRelativePath[0] == 0))
    {
        resp.setResult("Destination path not specified");
        return true;
    }

    resp.setDestDropZone(server);
    resp.setDestRelativePath(destRelativePath);

    StringBuffer destAbsPath;
    StringBuffer destNetAddr;
    if (!CheckServerAccess(server, destRelativePath, destNetAddr, destAbsPath))
    {
        result.appendf("Failed to access the destination: %s %s.", server, destRelativePath);
        resp.setResult(result.str());
        return true;
    }

    __int64 dataSize = req.getDataSize();
    __int64 offset = req.getOffset();
    resp.setDataSize(dataSize);
    resp.setOffset(offset);
    if (dataSize < 1)
    {
        resp.setResult("Invalid data size.");
        return true;
    }

    if (offset < 0)
    {
        resp.setResult("Invalid offset.");
        return true;
    }

    StringBuffer user;
    RemoteFilename rfn;
    SocketEndpoint ep;
    ep.set(destNetAddr);
    rfn.setPath(ep, destAbsPath);
    Owned<IFile> file = createIFile(rfn);
    fileBool isFile = file->isFile();
    if (isFile != foundYes)
    {
        result.appendf("%s does not exist.", destRelativePath);
        resp.setResult(result.str());
        return true;
    }

    Owned<IFileIO> io = file->open(IFOread);
    __int64 size = io->size();
    if (offset >= size)
    {
        result.appendf("Invalid offset: file size = %" I64F "d.", size);
        resp.setResult(result.str());
        return true;
    }

    __int64 dataToRead = size - offset;
    if (dataToRead > dataSize)
    {
        dataToRead = dataSize;
    }

    MemoryBuffer membuf;
    char* buf = (char*) membuf.reserve((int) dataToRead);
    if (io->read(offset, (int)dataToRead, buf) != dataToRead)
    {
        resp.setResult("ReadFileData error.");
        LOG(MCprogress, unknownJob, "ReadFileData error: %s: %s %s", context.getUserID(user).str(), server, destRelativePath);
    }
    else
    {
        resp.setData(membuf);
        resp.setResult("ReadFileData done.");
        LOG(MCprogress, unknownJob, "ReadFileData done: %s: %s %s", context.getUserID(user).str(), server, destRelativePath);
    }

    return true;
}

bool CWsFileIOEx::onWriteFileData(IEspContext &context, IEspWriteFileDataRequest &req, IEspWriteFileDataResponse &resp)
{
    context.validateFeatureAccess(FILE_IO_URL, SecAccess_Write, true);

    StringBuffer result;
    const char* server = req.getDestDropZone();
    if (!server || (server[0] == 0))
    {
        resp.setResult("Destination not specified");
        return true;
    }

    const char* destRelativePath = req.getDestRelativePath();
    if (!destRelativePath || (destRelativePath[0] == 0))
    {
        resp.setResult("Destination path not specified");
        return true;
    }

    MemoryBuffer& srcdata = (MemoryBuffer&)req.getData();
    if(srcdata.length() == 0)
    {
        resp.setResult("Source data not specified");
        return true;
    }

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
    {
        result.appendf("Failed to access the destination: %s %s.", server, destRelativePath);
        resp.setResult(result.str());
        return true;
    }

    StringBuffer user;
    RemoteFilename rfn;
    SocketEndpoint ep;
    ep.set(destNetAddr);
    rfn.setPath(ep, destAbsPath);
    Owned<IFile> file = createIFile(rfn);
    fileBool isFile = file->isFile();
    if (isFile != foundYes)
    {
        result.appendf("%s does not exist.", destRelativePath);
        resp.setResult(result.str());
        return true;
    }

    if (append)
    {
        Owned<IFileIO> io = file->open(IFOread);
        offset = io->size();
    }

    Owned<IFileIO> fileio = file->open(IFOwrite);
    size32_t len = srcdata.length();
    if (fileio->write(offset, len, srcdata.readDirect(len)) != len)
    {
        resp.setResult("WriteFileData error.");
        LOG(MCprogress, unknownJob, "WriteFileData error: %s: %s %s", context.getUserID(user).str(), server, destRelativePath);
    }
    else
    {
        resp.setResult("WriteFileData done.");
        LOG(MCprogress, unknownJob, "WriteFileData done: %s: %s %s", context.getUserID(user).str(), server, destRelativePath);
    }

    return true;
}

//#define SAVETOFILE_TEST
void readTableContentCSV(IEspSaveTableToFileRequest& req, StringBuffer& buf)
{
#ifdef SAVETOFILE_TEST
    StringArray headers;
    headers.append("h1");
    headers.append("h2");
    headers.append("h3");
    IArrayOf<IEspRow> rows;
    StringArray row01, row02;
    row01.append("c11");
    row01.append("c12");
    row01.append("c13");
    row02.append("c21");
    row02.append("c22");
    row02.append("c23");
    Owned<IEspRow> row1 = new CRow("", "");
    row1->setCells(row01);
    Owned<IEspRow> row2 = new CRow("", "");
    row2->setCells(row02);
    rows.append(*row1.getLink());
    rows.append(*row2.getLink());
#endif
#ifndef SAVETOFILE_TEST
    StringArray& headers = req.getHeaders();
#endif
    ForEachItemIn(i, headers)
    {
        if (buf.length())
            buf.append(",");
        buf.append(headers.item(i));
    }
    if (buf.length())
        buf.append("\n");

#ifndef SAVETOFILE_TEST
    IArrayOf<IConstRow>& rows = req.getTableRows();
#endif
    ForEachItemIn(ri, rows)
    {
        StringArray& cells = rows.item(ri).getCells();
        bool firstCell = true;
        ForEachItemIn(ci, cells)
        {
            if (!firstCell)
                buf.append(",");
            else
                firstCell = false;
            buf.append(cells.item(ci));
        }
        buf.append("\n");
    }
}

void readTableContentXMLJSON(IEspSaveTableToFileRequest& req, IXmlWriter &writer)
{
#ifdef SAVETOFILE_TEST
    StringArray headers;
    headers.append("h1");
    headers.append("h2");
    headers.append("h3");
    IArrayOf<IEspRow> rows;
    StringArray row01, row02;
    row01.append("c11");
    row01.append("c12");
    row01.append("c13");
    row02.append("c21");
    row02.append("c22");
    row02.append("c23");
    Owned<IEspRow> row1 = new CRow("", "");
    row1->setCells(row01);
    Owned<IEspRow> row2 = new CRow("", "");
    row2->setCells(row02);
    rows.append(*row1.getLink());
    rows.append(*row2.getLink());
#endif
#ifndef SAVETOFILE_TEST
    StringArray& headers = req.getHeaders();
#endif
    unsigned headerCount = headers.length();

    writer.outputBeginNested("Table", true);
    writer.outputBeginNested("Rows", true);
#ifndef SAVETOFILE_TEST
    IArrayOf<IConstRow>& rows = req.getTableRows();
#endif
    ForEachItemIn(ri, rows)
    {
        StringArray& cells = rows.item(ri).getCells();
        unsigned cellCount = cells.length();
        writer.outputBeginNested("Row", false);
        ForEachItemIn(ci, cells)
        {
            const char* header = headers.item(ci);
            if (!header || !*header || (cellCount != headerCount))
                header = "Cell";
            const char* cell = cells.item(ci);
            if (!cell)
                cell = "";
            writer.outputString(strlen(cell), cell, header);
        }
        writer.outputEndNested("Row");
    }
    writer.outputEndNested("Rows");
    writer.outputEndNested("Table");
}

void readTableContent(IEspSaveTableToFileRequest& req, CSaveTableToFileContentFormats contentFormat, MemoryBuffer& buf)
{
    StringBuffer content;
    if (contentFormat == CSaveTableToFileContentFormats_XML)
    {
        Owned<CommonXmlWriter> writer = CreateCommonXmlWriter(XWFexpandempty);
        readTableContentXMLJSON(req, *writer);

        const char *str = writer->str();
        unsigned len = writer->length();
        if (len && str[len-1]=='\n')
            len--;

        content.append("<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n<?xml-stylesheet href=\"../esp/xslt/xmlformatter.xsl\" type=\"text/xsl\"?>\n");
        content.append(len, str);
    }
    else if (contentFormat == CSaveTableToFileContentFormats_JSON)
    {
        Owned<CommonJsonWriter> writer = new CommonJsonWriter(0);
        writer->outputBeginRoot();
        readTableContentXMLJSON(req, *writer);
        writer->outputEndRoot();
        content.set(writer->str());
    }
    else //CSV
        readTableContentCSV(req, content);
    buf.append(content.length(), content.str());
}

void saveToFile(IEspContext& context, const char* fileName, CSaveTableToFileFormats fileFormat,
    CSaveTableToFileContentFormats contentFormat, MemoryBuffer& buf, IEspSaveTableToFileResponse& resp)
{
    if (fileFormat == CSaveTableToFileFormats_Plain)
    {
        StringBuffer headerStr("attachment;");
        if (fileName && *fileName)
        {
            const char* pFileName = strrchr(fileName, PATHSEPCHAR);
            if (pFileName)
                headerStr.appendf("filename=%s", pFileName+1);
            else
                headerStr.appendf("filename=%s", fileName);
        }

        MemoryBuffer buf0;
        unsigned i = 0;
        char* p = (char*) buf.toByteArray();
        while (i < buf.length())
        {
            if (p[0] != 10)
                buf0.append(p[0]);
            else
                buf0.append(0x0d);

            p++;
            i++;
        }
        resp.setThefile(buf);
        if (contentFormat == CSaveTableToFileContentFormats_XML)
            resp.setThefile_mimetype(HTTP_TYPE_TEXT_XML);
        else if (contentFormat == CSaveTableToFileContentFormats_JSON)
            resp.setThefile_mimetype("application/json");
        else //CSV
            resp.setThefile_mimetype("text/csv");
        context.addCustomerHeader("Content-disposition", headerStr.str());
        return;
    }

#ifndef _USE_ZLIB
    throw MakeStringException(ECLWATCH_CANNOT_COMPRESS_DATA,"The data cannot be compressed.");
#else
    StringBuffer fileNameStr, headerStr("attachment;");
    if (fileName && fileName)
    {
        fileNameStr.append(fileName);
        headerStr.append("filename=").append(fileName).append((fileFormat == CSaveTableToFileFormats_GZIP) ? ".gz" : ".zip");
    }
    else
        fileNameStr.append("file");

    VStringBuffer ifname("%s%sT%xAT%x", TEMPZIPDIR, PATHSEPSTR, (unsigned)(memsize_t)GetCurrentThreadId(), msTick());
    ifname.append((fileFormat == CSaveTableToFileFormats_GZIP)? "" : ".zip");

    IZZIPor* Zipor = createZZIPor();
    int ret = 0;
    if (fileFormat == CSaveTableToFileFormats_GZIP)
        ret = Zipor->gzipToFile(buf.length(), (void*)buf.toByteArray(), ifname.str());
    else
        ret = Zipor->zipToFile(buf.length(), (void*)buf.toByteArray(), fileNameStr.str(), ifname.str());
    releaseIZ(Zipor);

    if (ret < 0)
    {
        Owned<IFile> rFile = createIFile(ifname.str());
        if (rFile->exists())
            rFile->remove();
        throw MakeStringException(ECLWATCH_CANNOT_COMPRESS_DATA,"The data cannot be compressed.");
    }

    Owned <IFile> rf = createIFile(ifname.str());
    if (!rf->exists())
        throw MakeStringException(ECLWATCH_CANNOT_COMPRESS_DATA,"The data cannot be compressed.");

    MemoryBuffer out;
    Owned <IFileIO> fio = rf->open(IFOread);
    read(fio, 0, (size32_t) rf->size(), out);
    resp.setThefile(out);
    fio.clear();
    rf->remove();

    resp.setThefile_mimetype((fileFormat == CSaveTableToFileFormats_GZIP) ? "application/x-gzip" : "application/zip");
    context.addCustomerHeader("Content-disposition", headerStr.str());
#endif
}

bool CWsFileIOEx::onSaveTableToFile(IEspContext& context, IEspSaveTableToFileRequest& req, IEspSaveTableToFileResponse& resp)
{
    try
    {
        context.validateFeatureAccess(FILE_IO_URL, SecAccess_Read, true);

        CSaveTableToFileFormats fileFormat = req.getFileFormat();
        if (fileFormat == SaveTableToFileFormats_Undefined)
            throw MakeStringException(ECLWATCH_INVALID_INPUT,"FileFormat not defined.");

        CSaveTableToFileContentFormats contentFormat = req.getContentFormat();
        if (contentFormat == SaveTableToFileContentFormats_Undefined)
            throw MakeStringException(ECLWATCH_INVALID_INPUT,"ContentFormat not defined.");

        MemoryBuffer buf;
        readTableContent(req, contentFormat, buf);
        saveToFile(context, req.getFileName(), fileFormat, contentFormat, buf, resp);
    }
    catch(IException* e)
    {
        FORWARDEXCEPTION(context, e,  ECLWATCH_INTERNAL_ERROR);
    }
    return true;
}
