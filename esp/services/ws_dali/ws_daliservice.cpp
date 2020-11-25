/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2020 HPCC Systems.

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

#ifdef _USE_OPENLDAP
#include "ldapsecurity.ipp"
#endif

#include "ws_daliservice.hpp"
#include "jlib.hpp"
#include "dautils.hpp"
#include "dasds.hpp"

#define REQPATH_EXPORTSDSDATA "/WSDali/Export"

static unsigned daliConnectTimeoutMs = 5000;
const char* daliFolder = "tempdalifiles" PATHSEPSTR;

static const char* remLeading(const char* s)
{
    if (*s == '/')
        s++;
    return s;
}

static IRemoteConnection* connectXPathOrFile(const char* path, bool safe, StringBuffer& xpath)
{
    CDfsLogicalFileName lfn;
    StringBuffer lfnPath;
    if ((strstr(path, "::") != nullptr) && !strchr(path, '/'))
    {
        lfn.set(path);
        lfn.makeFullnameQuery(lfnPath, DXB_File);
        path = lfnPath.str();
    }
    else if (strchr(path + ((*path == '/') ? 1 : 0),'/') == nullptr)
        safe = true;    // all root trees safe

    Owned<IRemoteConnection> conn = querySDS().connect(remLeading(path), myProcessSession(), safe ? 0 : RTM_LOCK_READ, daliConnectTimeoutMs);
    if (!conn && !lfnPath.isEmpty())
    {
        lfn.makeFullnameQuery(lfnPath.clear(), DXB_SuperFile);
        path = lfnPath.str();
        conn.setown(querySDS().connect(remLeading(path), myProcessSession(), safe? 0 : RTM_LOCK_READ, daliConnectTimeoutMs));
    }
    if (conn.get())
        xpath.append(path);
    return conn.getClear();
}

void CWSDaliEx::init(IPropertyTree* cfg, const char* process, const char* service)
{
    espProcess.set(process);
}

int CWSDaliSoapBindingEx::onGet(CHttpRequest* request, CHttpResponse* response)
{
    try
    {
#ifdef _USE_OPENLDAP
        request->queryContext()->ensureSuperUser(ECLWATCH_SUPER_USER_ACCESS_DENIED, "Access denied, administrators only.");
#endif
        if (wsdService->isDaliDetached())
            throw MakeStringException(ECLWATCH_CANNOT_CONNECT_DALI, "Dali detached.");

        StringBuffer path;
        request->getPath(path);

        if (!strnicmp(path.str(), REQPATH_EXPORTSDSDATA, sizeof(REQPATH_EXPORTSDSDATA) - 1))
        {
            exportSDSData(request, response);
            return 0;
        }
    }
    catch(IException* e)
    {
        onGetException(*request->queryContext(), request, response, *e);
        FORWARDEXCEPTION(*request->queryContext(), e,  ECLWATCH_INTERNAL_ERROR);
    }

    return CWSDaliSoapBinding::onGet(request,response);
}

void CWSDaliSoapBindingEx::exportSDSData(CHttpRequest* request, CHttpResponse* response)
{
    CDateTime now;
    now.setNow();
    time_t createTime = now.getSimple();

    StringBuffer peer, outFileNameWithPath;
    VStringBuffer outFileName("sds_for_%s_%ld", request->getPeer(peer).str(), createTime);
    outFileNameWithPath.set(daliFolder).append(outFileName);

    StringBuffer path, xpath, safeReq;
    request->getParameter("Path", path);
    request->getParameter("Safe", safeReq);
    bool safe = atoi(safeReq.str()) ? true : false;
    Owned<IRemoteConnection> conn = connectXPathOrFile(path, safe, xpath);
    if (!conn)
        throw MakeStringException(ECLWATCH_CANNOT_CONNECT_DALI, "Failed to connect Dali.");

    Owned<IFile> workingDir = createIFile(daliFolder);
    if (!workingDir->exists())
        workingDir->createDirectory();

    Owned<IPropertyTree> root = conn->getRoot();
    Owned<IFile> f = createIFile(outFileNameWithPath);
    Owned<IFileIO> io = f->open(IFOcreaterw);
    { //Force the fios to finish
        Owned<IFileIOStream> fios = createBufferedIOStream(io);
        toXML(root, *fios);
    }

    VStringBuffer headerStr("attachment;filename=%s", outFileName.str());
    IEspContext* context = request->queryContext();
    context->addCustomerHeader("Content-disposition", headerStr.str());

    response->setContent(createIOStream(io));
    response->setContentType(HTTP_TYPE_OCTET_STREAM);
    response->send();

    removeFileTraceIfFail(outFileNameWithPath);
}
