/*##############################################################################
#    HPCC SYSTEMS software Copyright (C) 2012 HPCC Systems.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
##############################################################################
 */

#include "http.hpp"
#include "jliball.hpp"
#include "xsdparser.hpp"
#include "xmldiff.hpp"

const char* version = "1.1";

void usage()
{
    printf("Lexis-Nexis soapplus version %s. Usage:\n", version);
    puts("   soapplus [action/options]");
    puts("");
    puts("Actions: select one");
    puts("   -h/-? : show this usage page");
    puts("   -stress <threads> <seconds> : run stress test, with the specified number of threads, for the specified number of seconds.");
    puts("");
    puts("Options: ");
    puts("   -url <[http|https]://[user:passwd@]host:port/path>: the url to send requests to. For esp, the path is normally the service name, not the method name. For examle, it could be WsADL, WsAccurint, WsIdentity etc.");
    puts("        For esp, the path is normally the service name, not the method name. For e.g., WsAccurint, WsIdentity etc.");
    puts("   -version: print out soapplus version.");
}

struct Request
{
    const char* inFiles = NULL;
    const char* outFile = NULL;
    const char* stopLineID = NULL;
}

struct ESPTransaction: public CInterface, implements IInterface
{
    IMPLEMENT_IINTERFACE;
    ESPTransaction() {};

    unsigned procId;
    unsigned threadId;
    StringAttr logId;
    StringAttr from;
    StringAttr clientVersion;

    unsigned activeReqs;
    StringAttr timeTakenMS;
    StringAttr dateStart;
    StringAttr timeStart;
    StringAttr message;

    StringAttr httpMethod;
    StringAttr httpURL;
    StringAttr soapMethod;
};

bool beginWith(const char*& ln, const char* pat)
{
    size32_t sz = strlen(pat);
    if (memicmp(ln, pat, sz))
        return false;

    ln += sz;
    return true;
}

void addFrom(char* pLog, unsigned procId, unsigned threadId, IArrayOf<ESPTransaction>& transactions)
{
    ForEachItemIn(i, transactions)
    {
        ESPTransaction& trans = transactions.item(i);
        if ((procId != trans.procId) || (threadId != trans.threadId))
            continue;
        trans.from.set(pLog);
    }
}

void addSOAPMethod(char* pLog, unsigned procId, unsigned threadId, IArrayOf<ESPTransaction>& transactions)
{
    ForEachItemIn(i, transactions)
    {
        ESPTransaction& trans = transactions.item(i);
        if ((procId != trans.procId) || (threadId != trans.threadId))
            continue;
        trans.soapMethod.set(pLog);
    }
}

void addClientVersion(char* pLog, unsigned procId, unsigned threadId, IArrayOf<ESPTransaction>& transactions)
{
    ForEachItemIn(i, transactions)
    {
        ESPTransaction& trans = transactions.item(i);
        if ((procId != trans.procId) || (threadId != trans.threadId))
            continue;
        trans.clientVersion.set(pLog);
    }
}

void addSummary(char* pLog, unsigned procId, unsigned threadId, IArrayOf<ESPTransaction>& transactions)
{
    ForEachItemIn(i, transactions)
    {
        ESPTransaction& trans = transactions.item(i);
        if ((procId != trans.procId) || (threadId != trans.threadId))
            continue;

        StringArray summaryFields;
        summaryFields.appendList(pLog, ";");
        ForEachItemIn(ii, summaryFields)
        {
            const char* field = summaryFields.item(i);
            if (beginWith(field, "activeReqs="))
                trans.activeReqs = atoi(field);
            else if (beginWith(field, "total="))
                trans.timeTakenMS.set(field);
        }
        outputTransaction(trans);
    }
}

void addMore(char* pLog, unsigned procId, unsigned threadId, IArrayOf<ESPTransaction>& transactions)
{
    ForEachItemIn(i, transactions)
    {
        ESPTransaction& trans = transactions.item(i);
        if ((procId != trans.procId) || (threadId != trans.threadId))
            continue;
        if (trans.from.length() < 1)
        {
            StringArray fields;
            fields.appendList(pLog, " ");
            if ((fields.length() > 3) && strieq(fields.item(2), "from"))
                trans.from.set(fields.item(3));
        }
    }
}

bool readLine(char* line, Request& req, IArrayOf<ESPTransaction>& transactions, IFileIO* outIO)
{
    StringArray fields;
    fields.appendList(line, " ");
    if (fields.length() < 6)
        return true;
    const char* logId = fields.item(0);
    if (!logId || (!transactions.length() && !streq(logId, "00000002")))
        return true;

    unsigned procId = atoi(fields.item(3));
    unsigned threadId = atoi(fields.item(4));
    const char* log = fields.item(5);
    const char* pLog = log;
    if (beginWith(pLog, "HTTP First Line: "))
    {
        StringArray httpFields;
        httpFields.appendList(pLog, " ");
        if (httpFields.length() < 3)
            return true;

        const char* url = httpFields.item(1);
        if (beginWith(url, "/esp/files/"))
            return true;
        Owned<ESPTransaction> tran = new ESPTransaction();
        tran->logId.set(logId);
        tran->procId = procId;
        tran->threadId = threadId;
        tran->dateStart.set(fields.item(1));
        tran->timeStart.set(fields.item(2));
        tran->httpMethod.set(httpFields.item(0));
        tran->httpURL.set(url);
        transactions.append(*tran.getClear());
    }
    else if (beginWith(pLog, "SOAP request from "))
    {
        addFrom(pLog, procId, threadId, transactions);
    }
    else if (beginWith(pLog, "SOAP method <"))
    {
        addSOAPMethod(pLog, procId, threadId, transactions);
    }
    else if (beginWith(pLog, "Client version: "))
    {
        addClientVersion(pLog, procId, threadId, transactions);
    }
    else if (beginWith(pLog, ""TxSummary["))
    {
        addSummary(pLog, procId, threadId, transactions);
    }
    else
    {
        addMore(pLog, procId, threadId, transactions);
    }

    return true;
}

int main(int argc, char** argv)
{
    InitModuleObjects();

    Request req;
    while(i<argc)
    {
        if(stricmp(argv[i], "-h") == 0 || stricmp(argv[i], "-?") == 0)
        {
            usage();
            return 0;
        }
        else if (stricmp(argv[i], "-i") == 0)
        {
            i++;
            req.inFiles = argv[i++];
        }
        else if (stricmp(argv[i], "-o") == 0)
        {
            i++;
            req.outFile = argv[i++];
        }
        else if (stricmp(argv[i], "-l") == 0)
        {
            i++;
            req.stopLineID = argv[i++];
        }
        else
        {
            printf("Error: unknown command parameter: %s\n", argv[i]);
            usage();
            return 0;
        }
    }

    if (!req.inFiles || !*req.inFiles)
    {
        printf("Error: no log file specified");
        usage();
        return 0;
    }

    if (!req.outFile || !*req.outFile)
    {
        printf("Error: no output file specified");
        usage();
        return 0;
    }

    try
    {
        Owned<IFile> out = createIFile(req.outFile);
        if (!out)
        {
            printf("Error: cannot create output file");
            return 0;
        }
        Owned<IFileIO> outIO = out->open(IFOcreate);
        if (!outIO)
        {
            printf("Error: cannot open output file");
            return 0;
        }
//            io->write(0, 10, "abcdefghij");

        IArrayOf<ESPTransaction> transactions;
        StringArray fileNames;
        fileNames.appendList(req.inFiles, "|");
        ForEachItemIn(i,fileNames)
        {
            const char* fileName = fileNames.item(i);
            if (!fileName || !*fileName)
                continue;

            FILE* f = fopen(fileName, "r");
            if (!f)
                continue;

            char buffer[10240];
            while(fgets(buffer,sizeof(buffer)-1,f))
            {
                if(!strlen(buffer))
                    break;

                reachStopLine = readLine(buffer, req, transactions, outIO);
                if (reachStopLine)
                    break;
            }
            fclose(f);
            if (reachStopLine)
                break;
        }
    }
    catch(IException *excpt)
    {
        StringBuffer errMsg;
        printf("Exception: %d:%s\n", excpt->errorCode(), excpt->errorMessage(errMsg).str());
    }
    catch(...)
    {
        printf("Unknown exception\n");
    }
    releaseAtoms();

    return 0;
}

