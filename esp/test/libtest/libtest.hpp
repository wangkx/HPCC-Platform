/*##############################################################################
    HPCC SYSTEMS software Copyright (C) 2014 HPCC Systems.
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

#if !defined(__LIBTEST_HPP__)
#define __LIBTEST_HPP__

#include "libtestcommon.hpp"

class CLibTest : public CInterface, implements ILibTest
{
    StringAttr parentName;
public:
    IMPLEMENT_IINTERFACE;

    CLibTest(void) {};
    virtual ~CLibTest(void) {};

    virtual bool init(IPropertyTree* cfg, const char* parent);
    virtual void echoTest(IEspContext &context, IEspEchoTestRequest& req, IEspEchoTestResponse& resp);
};

#endif // !defined(__LIBTEST_HPP__)
