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

#ifndef _DFUWU_HELPERS_HPP__
#define _DFUWU_HELPERS_HPP__

#ifndef dfuwu_decl
#define dfuwu_decl __declspec(dllimport)
#endif


interface IPropertyTree;

#include "jlib.hpp"

enum DFUspraytype
{
    DFUsprayfixed,
    DFUsprayVariable
};

struct WsFSSprayOptions
{
    StringBuffer prefix;
    int    maxConnections;
    int    throttle;
    int    transferBufferSize;

    bool   overwrite;
    bool   replicate;
    bool   noSplit;
    bool   noRecover;
    bool   push;
    bool   pull;
};

struct WsFSSprayInput
{
    WsFSSprayInput()
    {
        compress = false;
        wrap = false;
        replicateOffset = 1;
        options.noSplit = false;
        options.noRecover = false;
        options.push = false;
        options.pull = false;
    }

    StringBuffer sourceIP;
    StringBuffer sourcePath;
    MemoryBuffer sourceXML;

    int sourceFormat;                   //SprayVariable only
    StringBuffer sourceRowTag;          //SprayVariable only
    StringBuffer sourceCsvSeparate;     //SprayVariable only
    StringBuffer sourceCsvTerminate;    //SprayVariable only
    StringBuffer sourceCsvQuote;        //SprayVariable only

    StringBuffer destGroup;
    StringBuffer destLogicalName;
    StringBuffer queueLabel; //m_QueueLabel
    StringBuffer userName; //from context
    StringBuffer password; //from context
    StringBuffer encryptKey;
    StringBuffer decryptKey;

    int sourceRecordSize;  //SourceMaxRecordSize for SprayVariable
    int    replicateOffset;

    bool   compress;
    bool   wrap;        //sprayFixed only

    WsFSSprayOptions options;
};

struct WsFSCopyOptions
{
    WsFSCopyOptions()
    {
        noRecover = false;
        pull = false;
        push = false;
        ifnewer = false;
        transferBufferSize = -1;    //using default
        maxConnections = -1;        //using default
        throttle = -1;              //using default
    }

    int maxConnections;
    int throttle;
    int transferBufferSize;
    bool overwrite;
    bool replicate;
    bool noRecover;
    bool noSplit;
    bool pull;
    bool push;
    bool ifnewer;
};

struct WsFSCopyInput
{
    WsFSCopyInput()
    {
        compress = false;
        wrap = false;
        superCopy = false;
    }

    StringBuffer sourceLogicalName;
    StringBuffer destLogicalName;
    StringBuffer sourceDali;
    StringBuffer destGroup;
    StringBuffer sourceDiffKeyName;
    StringBuffer destDiffKeyName;
    StringBuffer sourceUserName;
    StringBuffer sourcePassword;
    StringBuffer userName; //from context
    StringBuffer password; //from context
    StringBuffer encryptKey;
    StringBuffer decryptKey;
    bool isRoxie;
    bool wrap;
    bool superCopy;
    bool compress;

    StringBuffer queueLabel; //m_QueueLabel
    Owned<IPropertyTree> directories;
    
    WsFSCopyOptions options;
};

extern dfuwu_decl void CreateCopyFileWorkunit(WsFSCopyInput& input, StringBuffer& wuid);
extern dfuwu_decl void CreateSprayFileWorkunit(DFUspraytype sprayType, WsFSSprayInput& input, StringBuffer& wuid);

#endif 