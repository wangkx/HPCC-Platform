/*##############################################################################
#    HPCC SYSTEMS software Copyright (C) 2020 HPCC Systems®.
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
#include <map>
#include <string>
#include "jliball.hpp"

const char* version = "1.0";

void usage()
{
    printf("esplogreader version %s.\n\nUsage:\n", version);
    printf("  esplogreader esp_log=<esp_log> out=<output_file>\n\n");
    printf("options:\n");
    printf("    from_msg=<from_msg_id>\n");
    printf("    to_msg=<to_msg_id>\n");
    printf("    skip_reqs=<skip_reqs> a comma separated string specifying which requests should not be read.\n");
    printf("....summary=1\n");
    printf("....short_list=1\n");
    printf("....next_days=0\n");
}

bool getInput(int argc, const char* argv[], IProperties* input)
{
    for (unsigned i = 1; i < argc; i++)
    {
        const char* arg = argv[i];
        if (strchr(arg, '='))
            input->loadProp(arg);
        else if (strieq(argv[i], "-v") || strieq(argv[i], "-version"))
        {
            printf("esplogreader version %s\n", version);
            return false;
        }
        else if (strieq(arg, "-h") || strieq(arg, "-?"))
        {
            usage();
            return false;
        }
        else
        {
            printf("Error: unknown command parameter: %s\n", argv[i]);
            usage();
            return false;
        }
    }
    return true;
}

class CESPActivity : public CInterface
{
public:
    CESPActivity() {};

    StringBuffer id, msgID, auth, activeReqs, startTime, endTime, PID, TID, req, reqParam, soapMethod, from, totalTime, warning;
    bool foundTxSummary = false;
};

class CESPSYS : public CInterface
{
public:
    CESPSYS() {};

    StringBuffer msgID, time, ramUsage, swpUsage, memUsage;
};

class CESPLogReader : public CSimpleInterface
{
    StringAttr outFileName, fromMSGID, toMSGID, command;
    StringBuffer msgIDsInWarning;
    StringArray espLogs, skipReqs;
    bool checkFromMSGID = false, checkToMSGID = false, summary = false;
    bool foundToMSGID = false;
    bool outputShortList =  false;
    unsigned columnNumTime = 0, columnNumMsgID = 0, columnNumPID = 0, columnNumTID = 0;
    unsigned countActivities = 0, countWarnings = 0, countNoTxSummary = 0;
    unsigned nextDays = 0;
    std::map<std::string, Owned<CESPSYS>> sysMap;
    std::map<std::string, Owned<CESPActivity>> activityMap;
    std::map<std::string, Owned<CESPActivity>> activityNoTxSummaryMap;
    std::map<std::string, unsigned> activityTypesCount;
    MapStringTo<bool> activitiesSkip;

    bool readColumnNum(const char* line);
    void parseLogColumn(const char* line, const unsigned columnNum, StringBuffer& columnStr);
    bool checkMSGID(const char* line);
    bool checkSkipReqs(const char* req);
    void parseFirstLine(const char* line, const char* firstLinePtr);
    void parseOtherLine(const char* line);
    void parseSYSLine(const char* line, CESPSYS* sysLine);
    void parseTxSummary(const char* line, CESPActivity* activity);
    void trimNewLine(StringBuffer& s);
    void trimLastChar(StringBuffer& s, const char c);
    void readOneDayLog(const char* logName, bool firstDay);
    void doSummary();
    void writeSummary(IFileIO* outFileIO, offset_t& pos);
    void writeSYS(IFileIO* outFileIO, offset_t& pos);
    void writeActivies(IFileIO* outFileIO, offset_t& pos);
public:
    CESPLogReader(IProperties* input, const char* _command) : command(_command)
    {
        espLogs.appendList(input->queryProp("esp_log"), ",");
        outFileName.set(input->queryProp("out"));
        if (input->hasProp("from_msg"))
        {
            fromMSGID.set(input->queryProp("from_msg"));
            checkFromMSGID = true;
        }
        if (input->hasProp("to_msg"))
        {
            toMSGID.set(input->queryProp("to_msg"));
            checkToMSGID = true;
        }
        if (input->hasProp("next_days"))
        {
            nextDays = input->getPropInt("next_days", 0);
        }
        if (input->hasProp("skip_reqs"))
        {
            skipReqs.appendList(input->queryProp("skip_reqs"), ",");
        }
        if (input->hasProp("summary"))
            summary = input->getPropBool("summary", false);
        outputShortList = input->getPropBool("short_list", false);
        printf("summary %d\n", summary);
    }

    void readLog();
    void writeAct();
};

bool CESPLogReader::readColumnNum(const char* line)
{
    bool foundFromHeader = true;
    unsigned logfields = getMessageFieldsFromHeader(line);
    if (logfields==0)   // No header line, so must be in legacy format
    {
        logfields = MSGFIELD_LEGACY;
        foundFromHeader = false;
    }

    columnNumTime = getPositionOfField(logfields, MSGFIELD_milliTime);
    columnNumMsgID = getPositionOfField(logfields, MSGFIELD_msgID);
    columnNumPID = getPositionOfField(logfields, MSGFIELD_process);
    columnNumTID = getPositionOfField(logfields, MSGFIELD_thread);
    return foundFromHeader;
}

void CESPLogReader::parseLogColumn(const char* line, unsigned columnNum, StringBuffer& columnStr)
{
    const char* bptr = line;
    while (columnNum-- && *bptr)
    {
        while(*bptr && *bptr!=' ') ++bptr;  // Skip field
        while(*bptr && *bptr==' ') ++bptr;  // Skip spaces
    }
    if (!*bptr)
        return;

    const char* eptr = bptr + 1;
    while(*eptr && *eptr!=' ') ++eptr;  // Skip field

    columnStr.append(eptr - bptr, bptr);
}

void CESPLogReader::parseFirstLine(const char* line, const char* firstLinePtr)
{
    Owned<CESPActivity> activity = new CESPActivity();
    parseLogColumn(line, columnNumMsgID, activity->msgID);
    parseLogColumn(line, columnNumPID, activity->PID);
    parseLogColumn(line, columnNumTID, activity->TID);
    activity->id.appendf("%s-%s", activity->PID.str(), activity->TID.str());

    const char* ptr = firstLinePtr + strlen("HTTP First Line: ");
    parseLogColumn(ptr, 1, activity->req);
    if (checkSkipReqs(activity->req))
    {
        activitiesSkip.setValue(activity->id, true);
    }
    else
    {
        const char* ptrParam = strchr(activity->req, '?');
        if (ptrParam)
        {
            activity->reqParam.set(ptrParam + 1);
            activity->req.setLength(ptrParam - activity->req.str());
        }
        parseLogColumn(line, columnNumTime, activity->startTime);
        activityMap.emplace(activity->id.str(), activity);
    }
}

void CESPLogReader::parseOtherLine(const char* line)
{
    //Find the Quoted content
    const char* bptr = line;
    while(*bptr && *bptr!='"') ++bptr;
    if (isEmptyString(bptr))
        return;
    if (isEmptyString(++bptr)) //skip the '"'
        return;

    if (!strncmp(bptr, "SYS: ", 5))
    {
        Owned<CESPSYS> sysLine = new CESPSYS();
        parseLogColumn(line, columnNumTime, sysLine->time);
        parseLogColumn(line, columnNumMsgID, sysLine->msgID);
        parseSYSLine(bptr + 5, sysLine);
        sysMap.emplace(sysLine->msgID.str(), sysLine);
        return;
    }

    StringBuffer id, PID, TID;
    parseLogColumn(line, columnNumPID, PID);
    parseLogColumn(line, columnNumTID, TID);
    id.appendf("%s-%s", PID.str(), TID.str());

    bool* found = activitiesSkip.getValue(id);
    if (found && *found)
        return;

    auto match = activityMap.find(id.str());
    if (match == activityMap.end())
        return;

    CESPActivity* activity = match->second;
    if (!strncmp(bptr, "POST ", 5))
    {
        parseLogColumn(bptr+5, 2, activity->from);
        trimNewLine(activity->from);
        trimLastChar(activity->from, '\n');
        trimLastChar(activity->from, '"');
    }
    else if (!strnicmp(bptr, "WARNING: ", 9))
    {
        activity->warning.set(bptr - 1);
        trimNewLine(activity->warning);
        trimLastChar(activity->warning, '\n');
    }
    else if (!strncmp(bptr, "TxSummary[", 10))
    {
        parseTxSummary(bptr+10, activity);
        activity->foundTxSummary = true;
    }
    else if (!strncmp(bptr, "SOAP method <", 13))
        parseLogColumn(bptr+13, 0, activity->soapMethod);
}

void CESPLogReader::parseSYSLine(const char* str, CESPSYS* sysLine)
{
    StringArray items;
    items.appendList(str, " ");
    ForEachItemIn(i, items)
    {
        const char* item = items.item(i);
        if (streq(item, "MU="))
            sysLine->memUsage.set(items.item(i+1));
        else if (!strncmp(item, "RAM=", 4))
            sysLine->ramUsage.set(item + 4);
        else if (!strncmp(item, "SWP=", 4))
            sysLine->swpUsage.set(item + 4);
    }
}

void CESPLogReader::parseTxSummary(const char* line, CESPActivity* activity)
{
    StringArray items;
    items.appendList(line, ";");
    ForEachItemIn(i, items)
    {
        const char* item = items.item(i);
        if (!strnicmp(item, "total=", 6))
            activity->totalTime.set(item + 6);
        else if (!strnicmp(item, "auth=", 5))
            activity->auth.set(item + 5);
        else if (!strnicmp(item, "activeReqs=", 11))
            activity->activeReqs.set(item + 11);
    }
}

void CESPLogReader::trimNewLine(StringBuffer& s)
{
    unsigned l = s.length();
    if ((l>1) && (s.charAt(l - 2) == '\r') && (s.charAt(l - 1) == '\n'))
        s.setLength(s.length() - 2);
}

void CESPLogReader::trimLastChar(StringBuffer& s, const char c)
{
    unsigned l = s.length();
    if ((l>0) && (s.charAt(l - 1) == c))
        s.setLength(l - 1);
}

bool CESPLogReader::checkSkipReqs(const char* req)
{
    ForEachItemIn(i, skipReqs)
    {
        const char* skipReq = skipReqs.item(i);
        if (WildMatch(req, skipReq, false))
            return true;
    }
    return false;
}

bool CESPLogReader::checkMSGID(const char* line)
{
    if (checkFromMSGID && !strncmp(line, fromMSGID.get(), fromMSGID.length()))
    {
        checkFromMSGID = false;
        return true;
    }
    if (checkToMSGID && !strncmp(line, toMSGID.get(), toMSGID.length()))
    {
        checkToMSGID = false;
        return true;
    }
    return false;
}

void CESPLogReader::readLog()
{
    //sleep(30);
    ForEachItemIn(i, espLogs)
    {
        const char* espLog = espLogs.item(i);
        readOneDayLog(espLog, i == 0);
        if (foundToMSGID)
            break;
    }

    if (summary)
        doSummary();
}

void CESPLogReader::readOneDayLog(const char* logName, bool firstDay)
{
    printf("Starting read() %s...\n", logName);

    Owned<IFile> rFile = createIFile(logName);
    Owned<IFileIO> rIO = rFile->open(IFOread);
    if (!rIO)
        throw makeStringExceptionV(-1, "Failed to open %s.", logName);

    OwnedIFileIOStream ios = createIOStream(rIO);
    Owned<IStreamLineReader> lineReader = createLineReader(ios, true);

    StringBuffer line;
    bool eof = lineReader->readLine(line.clear());
    if (eof)
        return;

    // Process header for log file format
    if (readColumnNum(line))
        eof = lineReader->readLine(line.clear());

    //unsigned count = 0;
    //while (!eof && (1000 > ++count))
    while (!eof)
    {
        if (checkFromMSGID && !checkMSGID(line))
        {
            eof = lineReader->readLine(line.clear());
            continue;
        }
        //bool foundToMSGID = false;
        if (checkToMSGID && checkMSGID(line))
            foundToMSGID = true;

        const char* firstLinePtr = strstr(line.str(), "HTTP First Line:");
        if (firstLinePtr)
            parseFirstLine(line, firstLinePtr);
        else
            parseOtherLine(line);

        if (foundToMSGID)
            break;

        eof = lineReader->readLine(line.clear());
    }
    printf("Finish read() %s...\n", logName);
}

void CESPLogReader::doSummary()
{
    printf("\nSummary ...\n");
    countActivities = activityMap.size();
    for (std::map<std::string, Owned<CESPActivity>>::iterator it=activityMap.begin(); it!=activityMap.end(); ++it)
    {
        CESPActivity* a = it->second;
        const char* typePtr = a->req.str();
        const char* bptr = typePtr + 1;
        while(*bptr && *bptr!='/') ++bptr;
        bptr++;
        while(*bptr && *bptr!='/') ++bptr;

        StringBuffer type;
        type.append(bptr - typePtr, typePtr);

        auto match = activityTypesCount.find(type.str());
        if (match == activityTypesCount.end())
            activityTypesCount.emplace(type.str(), 1);
        else
            match->second++;
        if (!a->warning.isEmpty())
        {
            msgIDsInWarning.append(a->msgID).append(",");
            countWarnings++;
        }
        if (!a->foundTxSummary)
        {
            /*msgIDsNoTxSummary.append(a->msgID);
            tIDsNoTxSummary.append(a->TID);
            firstLNoTxSummary.append(a->req);*/
            activityNoTxSummaryMap.emplace(it->first, LINK(a));
            countNoTxSummary++;
        }
    }
}

void CESPLogReader::writeSummary(IFileIO* outFileIO, offset_t& pos)
{
    printf("\nSummary:\n");
    pos +=  outFileIO->write(pos, 11, "\nSummary:\n\n");
    for (std::map<std::string, unsigned>::iterator it=activityTypesCount.begin(); it!=activityTypesCount.end(); ++it)
    {
        VStringBuffer line("%s %u\n", it->first.c_str(), it->second);
        pos +=  outFileIO->write(pos, line.length(), line.str());
    }

    StringBuffer line1;
    line1.setf("\nTotal:%u\n", countActivities);
    pos +=  outFileIO->write(pos, line1.length(), line1);
    line1.setf("Warnings:%u\n", countWarnings);
    if (countWarnings > 0)
        line1.appendf("  %s\n", msgIDsInWarning.str());
    pos +=  outFileIO->write(pos, line1.length(), line1);
    line1.setf("TxSummary not found:%u\n", countNoTxSummary);
    pos +=  outFileIO->write(pos, line1.length(), line1);
    //if (countNoTxSummary > 0)
    //    line1.appendf("  %s\n", msgIDsNoTxSummary.str());
    //pos +=  outFileIO->write(pos, line1.length(), line1);
    //pos +=  outFileIO->write(pos, 7, "------\n");
    unsigned count = 1;
    for (std::map<std::string, Owned<CESPActivity>>::iterator it=activityNoTxSummaryMap.begin(); it!=activityNoTxSummaryMap.end(); ++it)
    {
        CESPActivity* a = it->second;
        StringBuffer line;
        line.appendf("%u: %s;%s;%s;%s;%s\n", count++, a->msgID.str(), a->startTime.str(), a->PID.str(), a->TID.str(), a->req.str());
        pos +=  outFileIO->write(pos, line.length(), line.str());
    }
}

void CESPLogReader::writeActivies(IFileIO* outFileIO, offset_t& pos)
{
    pos +=  outFileIO->write(pos, 12, "\nActivies:\n\n");

    VStringBuffer columns("\nstartTime: msgID;PID;TID;activeReqs;req;reqParam;soapMethod;from;totalTime;auth;warning\n\n");
    pos +=  outFileIO->write(pos, columns.length(), columns.str());

    for (std::map<std::string, Owned<CESPActivity>>::iterator it=activityMap.begin(); it!=activityMap.end(); ++it)
    {
        CESPActivity* a = it->second;

        if (outputShortList && a->foundTxSummary && a->warning.isEmpty())
            continue;

        StringBuffer line;
        line.appendf("%s: %s;%s;%s;%s;%s;%s;",
            a->startTime.str(), a->msgID.str(), a->PID.str(), a->TID.str(), a->activeReqs.str(), a->req.str(), a->reqParam.str());
        line.appendf("%s;%s;%s;%s;%s\n",
            a->soapMethod.str(), a->from.str(), a->totalTime.str(), a->auth.str(), a->warning.str());
        pos +=  outFileIO->write(pos, line.length(), line.str());
    }

    pos +=  outFileIO->write(pos, columns.length(), columns.str());
    pos +=  outFileIO->write(pos, 7, "------\n");
}

void CESPLogReader::writeSYS(IFileIO* outFileIO, offset_t& pos)
{
    pos +=  outFileIO->write(pos, 7, "\nSYS:\n\n");

    VStringBuffer columns("\ntime: mem;ram;swp;msgID\n\n");
    pos +=  outFileIO->write(pos, columns.length(), columns.str());

    for (std::map<std::string, Owned<CESPSYS>>::iterator it=sysMap.begin(); it!=sysMap.end(); ++it)
    {
        CESPSYS* a = it->second;

        StringBuffer line;
        line.appendf("%s: %s;%s;%s;%s\n", a->time.str(), a->memUsage.str(), a->ramUsage.str(), a->swpUsage.str(), a->msgID.str());
        pos +=  outFileIO->write(pos, line.length(), line.str());
    }

    pos +=  outFileIO->write(pos, columns.length(), columns.str());
    pos +=  outFileIO->write(pos, 7, "------\n");
}

void CESPLogReader::writeAct()
{
    //sleep(30);
    printf("Starting write() %s ...\n", outFileName.str());
    Owned<IFile> outFile = createIFile(outFileName);
    Owned<IFileIO> outFileIO = outFile->open(IFOcreaterw);
    if (!outFileIO)
        throw makeStringExceptionV(-1, "Failed to open %s.", outFileName.str());

    offset_t pos = outFileIO->write(0, command.length(), command.get());
    pos +=  outFileIO->write(pos, 2, "\n\n");

    writeSYS(outFileIO, pos);

    writeActivies(outFileIO, pos);

    if (summary)
        writeSummary(outFileIO, pos);
    printf("Finish write() %s ...\n", outFileName.str());
}

bool processRequest(IProperties* input, const char* command)
{
    const char* espLog = input->queryProp("esp_log");
    if (isEmptyString(espLog))
    {
        printf("Error: 'esp_log' not specified\n");
        return false;
    }

    const char* outFile = input->queryProp("out");
    if (isEmptyString(outFile))
    {
        printf("Error: 'out' not specified\n");
        return false;
    }

    printf("esplogreader version %s\n", version);
    Owned<CESPLogReader> reader = new CESPLogReader(input, command);
    reader->readLog();
    reader->writeAct();

    return true;
}

int main(int argc, const char** argv)
{
    InitModuleObjects();

    StringBuffer command;
    for(unsigned counter=0; counter<argc; counter++)
        command.appendf((counter == 0) ? "%s" : " %s", argv[counter]);

    Owned<IProperties> input = createProperties(true);
    if (!getInput(argc, argv, input))
    {
        releaseAtoms();
        return 0;
    }

    try
    {
        processRequest(input, command);
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
