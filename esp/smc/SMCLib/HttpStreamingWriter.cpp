/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2020 HPCC Systems®.

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

#include "httpbinding.hpp"
#include "HttpStreamingWriter.hpp"

void CHttpStreamWriter::addContentDispositionHeader(const char* file, const char* fileName)
{
    StringBuffer attachmentName;
    if (isEmptyString(fileName))
        splitFilename(file, nullptr, nullptr, &attachmentName, nullptr);
    else
        attachmentName.set(fileName);

    VStringBuffer contentDispositionHeader("attachment;filename=%s", attachmentName.str());
    context->addCustomerHeader("Content-disposition", contentDispositionHeader.str());
}

void CHttpStreamWriter::write(const char* file, const char* fileName, const char* netAddress)
{
    OwnedIFile rFile;
    if (isEmptyString(netAddress))
        rFile.setown(createIFile(file));
    else
    {
        RemoteFilename rfn;
        rfn.setRemotePath(file);
        SocketEndpoint ep(netAddress);
        rfn.setIp(ep);
        rFile.setown(createIFile(rfn));
    }
    if (!rFile)
        throw MakeStringException(ECLWATCH_CANNOT_OPEN_FILE, "Failed to find %s.", file);

    OwnedIFileIO rIO = rFile->openShared(IFOread,IFSHfull);
    if (rIO)
        throw MakeStringException(ECLWATCH_CANNOT_OPEN_FILE, "Failed to open %s.", file);

    addContentDispositionHeader(file, fileName);
    response->setContent(createIOStream(rIO));  
    response->setContentType(HTTP_TYPE_OCTET_STREAM);
    response->send();
}