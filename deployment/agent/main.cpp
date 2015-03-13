/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2015 HPCC Systems.

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

#include "dpagent.hpp"
#include "jliball.hpp"
#include "environment.hpp"

const char* version = "1.0";

void usage()
{
    printf("Lexis-Nexis deployment agent version %s. Usage:\n", version);
    puts("   dpagent [action/options]");
    puts("");
    puts("Actions: select one");
    puts("   -h/-? : show this usage page");
    puts("   -dp : do HPCC deplyment. This is the default action.");
    puts("   -rdp : do re-deplyment.");
    puts("   -start : start HPCC.");
    puts("   -restart : re-start HPCC.");
    puts("   -stop : stop HPCC.");
    puts("   -c : send command.");
    puts("");
    puts("Options: ");
    puts("   -control <command> : input command (for client mode)");
    puts("   -i <file name> : input file containing the requests (for client mode)");
    puts("   -o <file name> : log file.");
    puts("   -host : remove IP.");
    puts("   -p : port to listen on or send to (default 8003).");
    puts("   -d <trace-level> : 0 no tracing, >=1 a little tracing, >=5 some tracing, >=10 all tracing. If -w is specified, default tracelevel is 5, if -stress is specified, it's 1, otherwise it's 10.");
}

enum DeployAgentAction
{
    DAA_Deploymant = 0,
    DAA_Redeploymant = 1,
    DAA_Start = 2,
    DAA_Stop = 3,
    DAA_Restart = 4,
    DAA_Command = 5
};

IConstEnvironment* getEnv()
{
    Owned<IEnvironmentFactory> factory = getEnvironmentFactory();
    Owned<IConstEnvironment> env = factory->openEnvironment();
    return (env.getClear());
}

void readInputData(const char* fileName, MemoryBuffer& data)
{
    if(!fileName || !*fileName)
        throw MakeStringException(-1,"readInputData: file name not specified");
    Owned <IFile> rf = createIFile(fileName);
    if (!rf->exists())
        throw MakeStringException(-1, "Cannot open file %s.",fileName);
    MemoryBuffer out;
    Owned <IFileIO> fio = rf->open(IFOread);
    if (!fio)
        throw MakeStringException(-1,"Cannot read file %s.",fileName);
    read(fio, 0, (size32_t) rf->size(), data);
}

int main(int argc, char** argv)
{
    InitModuleObjects();

    DeployAgentAction action = DAA_Deploymant;
    FILE* outFile = NULL;
    StringBuffer command, inFile, host;
    int port = 8003;

    int i = 1;
    while(i<argc)
    {
        if(!stricmp(argv[i], "-h") || !stricmp(argv[i], "-?"))
        {
            usage();
            return 0;
        }
        else if (!stricmp(argv[i], "-echo"))
        {
            echoTest = true;
        }
        else if (!stricmp(argv[i], "-c"))
        {
            action = DAA_Command;
        }
        else if (!stricmp(argv[i], "-rdp"))
        {
            action = DAA_Redeploymant;
        }
        else if (!stricmp(argv[i], "-start"))
        {
            action = DAA_Start;
        }
        else if (!stricmp(argv[i], "-restart"))
        {
            action = DAA_Restart;
        }
        else if (!stricmp(argv[i], "-stop"))
        {
            action = DAA_Stop;
        }
        else if (!stricmp(argv[i], "-i"))
        {
            inFile.set(argv[++i]);
        }
        else if (!stricmp(argv[i], "-o"))
        {
            if( (outFile = fopen(argv[++i], "ab")) == NULL )
                throw MakeStringException(-1, "Cannot open file %s.", argv[i]);
        }
        else if (!stricmp(argv[i], "-control"))
        {
            command.set(argv[++i]);
        }
        else if (!stricmp(argv[i], "-host"))
        {
            host.set(argv[++i]);
        }
        else if (!stricmp(argv[i], "-p"))
        {
            port = atoi(argv[++i]);
        }
        else if (!stricmp(argv[i], "-d"))
        {
            traceLevel = atoi(argv[++i]);
        }
        i++;
    }

    try
    {
        DPAgent agent(outFile);
        if (action == DAA_Command)
        {
            MemoryBuffer data;
            if (command.length())
            {
                data.append(command);
                agent.sendData(data, host.str(), port);
            }
            else if (inFile.length())
            {
                readInputData(inFile.str(), data);
                agent.sendData(data, host.str(), port);
            }
        }
        else
        {
            agent.start(port);
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

