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

#ifndef _ESPWIZ_HttpStreamWriter_HPP__
#define _ESPWIZ_HttpStreamWriter_HPP__

#include "jliball.hpp"
#include "bindutil.hpp"
#include "exception_util.hpp"

const unsigned defaultSendChuckSize = 10000;

interface IHttpStreamWriter : extends IInterface
{
    virtual void write(const char* file, const char* fileName, const char* netAddress) = 0;
};

class CHttpStreamWriter : implements IHttpStreamWriter, public CInterface
{
    IEspContext* context;
    CHttpResponse* response = nullptr;

    void addContentDispositionHeader(const char* file, const char* fileName);

public:
    IMPLEMENT_IINTERFACE;

    CHttpStreamWriter(IEspContext* _context, CHttpResponse* _response) : context(_context), response(_response) { }

    virtual void write(const char* file, const char* fileName, const char* netAddress) override;
};

#endif //_ESPWIZ_HttpStreamWriter_HPP__

