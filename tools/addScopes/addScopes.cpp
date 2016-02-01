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

#include "seclib.hpp"
#include "ldapsecurity.hpp"
#include "jliball.hpp"

#ifndef _WIN32
#include <unistd.h>
#endif

bool readALine(OwnedIFileIOStream& ios, StringBuffer& line)
{
    line.clear();
    loop
    {
        char c;
        size32_t numRead = ios->read(1, &c);
        if (!numRead)
            return false;
        line.append(c);
        if (c=='\n')
            break;
    }
    return true;
}

bool checkReqLine(StringBuffer& line, IFileIO* rIO)
{
    if (line.length() < 8)
        return false;
    const char* ptr = line.str();
    ptr += (line.length() - 7);
    if (strieq(ptr, "request"))
        return true;
    return false;
}

bool writeReqItem(StringBuffer& line, IFileIO* rIO)
{
    if (streq(line.str(), "}"))
        return false;

    const char* ptr = line.str();
    while (ptr[0] == ' ')
        ptr++;
    if (ptr[0] == '[')
    {
        ptr++;
        while (ptr[0] != ']')
            ptr++;
        ptr += 2;
    }
    StringArray words;
    qlist.appendListUniq(ptr, " ");
    if (!strieq(words.item(0), "string") && !strieq(words.item(0), "bool") &&
        !strieq(words.item(0), "int") && !strieq(words.item(0), "int64") &&
        !strieq(words.item(0), "nonNegativeInteger"))
    return true;
}

int main(int argc, char* argv[])
{
    InitModuleObjects();

    try
    {
        Owned<IFile> outFile = createIFile(argv[2]));
        OwnedIFileIO oIO = outFile->openShared(IFOwrite,IFSHfull);

        Owned<IFile> inFile = createIFile(argv[1]));
        Owned<IDirectoryIterator> di = inFile->directoryFiles(NULL, false, true);
        ForEach(*di)
        {
            StringBuffer fname;
            di->getName(fname);
            if (!fname.length())
                continue;
            if (!strieq(fname, "ws_access.ecm") && !strieq(fname, "ws_account.ecm") &&
                !strieq(fname, "ws_dfu.ecm") && !strieq(fname, "ws_dfuXref.ecm") &&
                !strieq(fname, "ws_fs.ecm") && !strieq(fname, "ws_fileio.ecm") &&
                !strieq(fname, "ws_machine.ecm") && !strieq(fname, "ws_packageprocess.ecm") &&
                !strieq(fname, "ws_smc.ecm") && !strieq(fname, "ws_topology.ecm") &&
                !strieq(fname, "ws_workunits.ecm"))
            {
                continue;
            }
            IFile& ecmFile = di->get();
            OwnedIFileIO rIO = ecmFile.openShared(IFOread,IFSHfull);
            if(rIO)
            {
                OwnedIFileIOStream ios = createBufferedIOStream(rIO);

                bool inReq = false;
                StringBuffer line;
                while(1)
                {
                    if (!readALine(ios, line))
                        break;
                    if (!line.length())
                        continue;
                    if (!inReq)
                        inReq = checkReqLine(line, oIO);
                    else
                        inReq = writeReqItem(line, oIO);
                }
            }
        }
    }
    catch(IException* e)
    {
        StringBuffer errmsg;
        e->errorMessage(errmsg);
        printf("%s\n", errmsg.str());
    }
    catch(...)
    {
        printf("Unknown exception\n");
    }

    releaseAtoms();
    return 0;
}
