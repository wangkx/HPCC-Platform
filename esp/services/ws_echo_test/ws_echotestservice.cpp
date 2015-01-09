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

#include "jlib.hpp"
#include "ws_echotestservice.hpp"
#ifdef _WIN32
#include "windows.h"
#endif

void CWsEchoTestEx::init(IPropertyTree *cfg, const char *process, const char *service)
{
}

bool CWsEchoTestEx::onEchoTest(IEspContext &context, IEspEchoTestRequest &req, IEspEchoTestResponse &resp)
{
    IEspTestMethodInfo &info = resp.updateMethodInfo();
    info.setName("EchoTest");

    const char* inputString = req.getInputString();
    if (!inputString || !*inputString)
    {
        info.setStatus("InputString not specified");
        return false;
    }

    info.setStatus("Method finished");

    resp.setEchoString(inputString);

    return true;
}
