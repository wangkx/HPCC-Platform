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

#if !defined __LIBTESTCOMMONDEF_HPP__
#define __LIBTESTCOMMONDEF_HPP__

#pragma warning (disable : 4786)

#ifdef WIN32
    #ifdef LIBTEST_EXPORTS
        #define LIBTEST_API __declspec(dllexport)
    #else
        #define LIBTEST_API __declspec(dllimport)
    #endif
#else
    #define LIBTEST_API
#endif

#define LIBTESTLIB "LibTestLib"

#include "jiface.hpp"
#include "esp.hpp"
#include "ws_echo_test_esp.ipp"

interface ILibTest : implements IInterface
{
    virtual bool init(IPropertyTree* loggingConfig, const char* parent) = 0;
    virtual void echoTest(IEspContext &context, IEspEchoTestRequest& req, IEspEchoTestResponse& resp) = 0;
};

typedef ILibTest* (*newLibTest_t_)();

#endif // !defined __LIBTESTCOMMONDEF_HPP__
