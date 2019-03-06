/*##############################################################################
#    HPCC SYSTEMS software Copyright (C) 2012 HPCC Systems®.
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

#include "jliball.hpp"

FILE* logfile = NULL;
const char* version = "1.1";

void usage()
{
    printf("failsafelog version %s. Usage:\n", version);
    puts("   failsafelog [action/options]");
    puts("");
    puts("Actions: select one");
    puts("   -h/-? : show this usage page");
    puts("   -c <manager> : check which log requests are finished or not.");
    puts("");
    puts("Options: ");
    puts("   -o <file-name> : file to put the outputs.");
    puts("   -from: ?");
    puts("   -to: ?");
    puts("   -version: print out soapplus version.");
}

enum FailSafeLogAction
{
    FPLA_CHECK = 0
};
/*
int doValidation = 0;
const char* xsdpath = NULL;

void createDirectories(const char* outdir, const char* url, bool bClient, bool bServer,
                       StringBuffer& outpath, StringBuffer& outpath1, StringBuffer& outpath2, 
                       StringBuffer& outpath3, StringBuffer& outpath4, StringBuffer& outpath5)
{
    Owned<IFile> out;
    if(outdir && *outdir)
    {
        out.setown(createIFile(outdir));
        if(out->exists())
        {
            if(!out->isDirectory())
                throw MakeStringException(0, "Output destination %s already exists, and it's not a directory", outdir);
        }
        outpath.append(outdir);
        if(outpath.charAt(outpath.length() - 1) != PATHSEPCHAR)
            outpath.append(PATHSEPCHAR);
    }
    else
    {
        outpath.append("SOAPPLUS");
        addFileTimestamp(outpath, false);
        outpath.append(PATHSEPCHAR);
    }

    outpath1.append(outpath).append("request").append(PATHSEPCHAR);
    outpath2.append(outpath).append("response_full").append(PATHSEPCHAR);
    outpath3.append(outpath).append("response_content").append(PATHSEPCHAR);
    outpath4.append(outpath).append("server_request").append(PATHSEPCHAR);
    outpath5.append(outpath).append("server_response").append(PATHSEPCHAR);

    if(bClient)
    {
        out.setown( createIFile(outpath1.str()) );
        if(out->exists())
        {
            if(!out->isDirectory())
                throw MakeStringException(0, "Output destination %s already exists, and it's not a directory", outpath1.str());
        }
        else
        {
            if(http_tracelevel > 0)
                fprintf(logfile, "Creating directory %s\n", outpath1.str());
            recursiveCreateDirectory(outpath1.str());
        }
        if(url && *url)
        {
            out.setown( createIFile(outpath2.str()) );
            if(out->exists())
            {
                if(!out->isDirectory())
                    throw MakeStringException(0, "Output destination %s already exists, and it's not a directory", outpath2.str());
            }
            else
            {
                if(http_tracelevel > 0)
                    fprintf(logfile, "Creating directory %s\n", outpath2.str());
                recursiveCreateDirectory(outpath2.str());
            }

            out.setown( createIFile(outpath3.str()) );
            if(out->exists())
            {
                if(!out->isDirectory())
                    throw MakeStringException(0, "Output destination %s already exists, and it's not a directory", outpath3.str());
            }
            else
            {
                if(http_tracelevel > 0)
                    fprintf(logfile, "Creating directory %s\n", outpath3.str());
                recursiveCreateDirectory(outpath3.str());
            }
        }
    }

    if(bServer)
    {
        if(http_tracelevel > 0)
            fprintf(logfile, "Creating directory %s\n", outpath4.str());
        recursiveCreateDirectory(outpath4.str());
        if(http_tracelevel > 0)
            fprintf(logfile, "Creating directory %s\n", outpath5.str());
        recursiveCreateDirectory(outpath5.str());
    }
}

int processRequest(IProperties* globals, SoapPlusAction action, const char* url, 
                   const char* in_fname, int port, const char* server_in_fname, 
                   const char* outdir, bool writeToFiles, const char* proxy_url_prefix, 
                   bool isEspLogFile, const char* outfilename)
{
    if (action == SPA_TestSchemaParser)
        doTestSchemaParser(globals, url, in_fname);

    Owned<IFileIO> fio1 = NULL;
    Owned<IFileIO> fio2 = NULL;
    Owned<IFileIO> fio3 = NULL;
    Owned<IFileIO> fio4 = NULL;
    
    StringBuffer outpath, outpath1, outpath2, outpath3, outpath4, outpath5;
    if (writeToFiles)
    {
        const bool bClient = action == SPA_CLIENT || action == SPA_BOTH;
        const bool bServer = action == SPA_SERVER || action == SPA_BOTH;
        createDirectories(outdir, url, bClient, bServer, outpath, outpath1, outpath2, outpath3, outpath4, outpath5);
    }

    if(action == SPA_SERVER)
    {
        SimpleServer server(globals, port, (server_in_fname && *server_in_fname)?server_in_fname:in_fname, writeToFiles?outpath.str():NULL, writeToFiles, -1);
        server.start();
    }
    else if(action == SPA_CLIENT)
    {
        Owned<HttpClient> client = new HttpClient(globals, url, in_fname, writeToFiles?outpath.str():NULL, outfilename, writeToFiles, doValidation, xsdpath, isEspLogFile);
        client->start();
    }
    else if(action == SPA_BOTH)
    {
        bool autogen = globals->getPropBool("autogen", false);
        const char* method = globals->queryProp("method");
        if(autogen && (!method || !*method))
        {
            throw MakeStringException(0, "In hybrid mode, you have to specify a method for automatically generating the request");
        }

        Owned<IFile> infile = NULL;
        Owned<IFile> server_infile = NULL;

        if(in_fname && *in_fname)
        {
            infile.setown(createIFile(in_fname));
            if(!infile->exists())
            {
                if(http_tracelevel > 0)
                    fprintf(logfile, "Input file/directory %s doesn't exist", in_fname);
                return -1;
            }
        }

        if(server_in_fname && *server_in_fname)
        {
            server_infile.setown(createIFile(server_in_fname));
            if(!server_infile->exists())
            {
                if(http_tracelevel > 0)
                    fprintf(logfile, "Server input file/directory %s doesn't exist", server_in_fname);
                return -1;
            }
        }
        
        if(infile.get() != NULL && infile->isDirectory())
        {
            Owned<IDirectoryIterator> di = infile->directoryFiles();
            if(di.get())
            {
                ForEach(*di)
                {
                    IFile &file = di->query();
                    if (file.isFile() && file.size() > 0)
                    {
                        const char* fname = file.queryFilename();
                        const char* slash = strrchr(fname, PATHSEPCHAR);
                        if(slash)
                            fname = slash;
                        StringBuffer sfname;
                        if(server_infile.get())
                        {
                            if(server_infile->isDirectory())
                            {
                                sfname.append(server_in_fname);
                                if(sfname.charAt(sfname.length() - 1) != PATHSEPCHAR)
                                    sfname.append(PATHSEPCHAR);
                                sfname.append(fname);
                            }
                            else
                            {
                                sfname.append(server_in_fname);
                            }
                            Owned<IFile> sf = createIFile(sfname.str());
                            if(!sf.get() || !sf->exists())
                            {
                                if(http_tracelevel > 0)
                                    fprintf(logfile, "Server input file %s doesn't exist. Make sure the server input directory's files match those of request input directory\n", sfname.str());
                                return -1;
                            }
                        }

                        ThreadedSimpleServer server(globals, port, sfname.str(), writeToFiles?outpath.str():NULL, writeToFiles,  1);
                        server.start();
                        HttpClient client(globals, url, file.queryFilename(), writeToFiles?outpath.str():NULL, outfilename, writeToFiles);
                        client.start();
                        if(http_tracelevel >= 5)
                            fprintf(logfile, "Waiting for the server thread to finish.\n");
                        server.join();
                        if(http_tracelevel >= 5)
                            fprintf(logfile, "Server thread finished.\n");
                    }   
                }
            }
        }   
        else
        {
            if(server_infile.get() && server_infile->isDirectory())
            {
                if(http_tracelevel > 0)
                    fprintf(logfile, "When request input is not directory, the server input must be a file\n");
                return -1;
            }
            ThreadedSimpleServer server(globals, port, server_in_fname, writeToFiles?outpath.str():NULL, writeToFiles,  1);
            server.start();
            HttpClient client(globals, url, in_fname, writeToFiles?outpath.str():NULL, outfilename, writeToFiles);
            client.start();
            if(http_tracelevel >= 5)
                fprintf(logfile, "Waiting for the server thread to finish.\n");
            server.join();
            if(http_tracelevel >= 5)
                fprintf(logfile, "Server thread finished.\n");
        }
    }
    else if(action == SPA_PROXY)
    {
        HttpProxy proxy(port, url, logfile, proxy_url_prefix);
        proxy.start();
    }
    else if(action == SPA_SOCKS)
    {
        SocksProxy sproxy(port, logfile);
        sproxy.start();
    }

    return 0;
}

bool isNumber(const char* str)
{
    if(!str || !*str)
        return false;

    int i = 0;
    char c;
    while((c=str[i++]) != '\0')
    {
        if(!isdigit(c))
            return false;
    }

    return true;
}

typedef MapStringTo<int> MapStrToInt;
*/
void getJobFiles(const char* logManager, StringBuffer& managerJobFile, StringArray& agentJobFileLocations)
{
    //Read logManagerDir and logManagerFailSafeLogsDir from env using logManager
    const char* logManagerDir = "/var/lib/HPCCSystems/scapps/";
    const char* logManagerFailSafeLogsDir="Log/FailSafeLogs";

    managerJobFile.set(logManagerDir);
    managerJobFile.append(logManagerFailSafeLogsDir);

    //Read logMAgentFailSafeLogsDirs from env using logManager
    StringArray logAgentFailSafeLogsDirs;
    logAgentFailSafeLogsDirs.append("Tran/FailSafeLogs");
    logAgentFailSafeLogsDirs.append("Royalty/FailSafeLogs");
    logAgentFailSafeLogsDirs.append("Deltabase/FailSafeLogs");
    logAgentFailSafeLogsDirs.append("Deltabase_RR/FailSafeLogs");
    ForEachItemIn(i, logAgentFailSafeLogsDirs)
    {
        StringBuffer logAgentFailSafeLogsDir = logManagerDir;
        logAgentFailSafeLogsDir.append(logAgentFailSafeLogsDirs.item(i));
    }
}

void checkProgress(const char* logManager, const char* fromTime, const char* toTime, const char* outFile)
{
    if (isEmptyString(logManager))
        printf("Exception: no log manager specified.\n");

    unsigned yearFrom = 0, monthFrom, dayFrom, hourFrom, minuteFrom, secondFrom, nanoFrom;
    unsigned yearTo = 0, monthTo, dayTo, hourTo, minuteTo, secondTo, nanoTo;
    if (!isEmptyString(fromTime))
    {
        CDateTime dt;
        dt.setString(fromTime, nullptr, true);
        dt.getDate(yearFrom, monthFrom, dayFrom, true);
        dt.getTime(hourFrom, minuteFrom, secondFrom, nanoFrom, true);
    }
    if (!isEmptyString(toTime))
    {
        CDateTime dt;
        dt.setString(toTime, nullptr, true);
        dt.getDate(yearTo, monthTo, dayTo, true);
        dt.getTime(hourTo, minuteTo, secondTo, nanoTo, true);
    }

    StringBuffer managerJobFile;
    StringArray agentJobFileLocations;
    getJobFiles(logManager, managerJobFile, agentJobFileLocations);
    if (managerJobFile.isEmpty())
        printf("Exception: Failed to shared JobFile location.\n");
    if (agentJobFileLocations.length() < 1)
        printf("Exception: Failed to agent JobFile locations.\n");

    ForEachItemIn(i, agentJobFileLocations)
    {
        const char* location = agentJobFileLocations.item(i);
    }
}

int main(int argc, char** argv)
{
    InitModuleObjects();

    const char* logManager = nullptr;
    const char* outFile = nullptr;
    const char* fromTime = nullptr;
    const char* toTime = nullptr;

    FailSafeLogAction action = FPLA_CHECK;

    int i = 1;
    while(i<argc)
    {
        if(stricmp(argv[i], "-h") == 0 || stricmp(argv[i], "-?") == 0)
        {
            usage();
            return 0;
        }
        else if (stricmp(argv[i], "-c") == 0)
        {
            i++;
            logManager = argv[i++];
        }
        else if (stricmp(argv[i], "-o") == 0)
        {
            i++;
            outFile = argv[i++];
        }
        else if (stricmp(argv[i], "-from") == 0)
        {
            i++;
            fromTime = argv[i++];
        }
        else if(stricmp(argv[i], "-to") == 0)
        {
            i++;
            toTime = argv[i++];
        }
        else if(stricmp(argv[i], "-version") == 0)
        {
            printf("failsafelog tool version %s\n", version);
            return 0;
        }
        else
        {
            printf("Error: unknown command parameter: %s\n", argv[i]);
            usage();
            return 0;
        }
    }

    try
    {
        if (action == FPLA_CHECK)
            checkProgress(logManager, fromTime, toTime, outFile);
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

