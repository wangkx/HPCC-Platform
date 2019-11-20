/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2020 HPCC Systems.

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

#ifndef _ESPWIZ_ws_decoupledlogging_HPP__
#define _ESPWIZ_ws_decoupledlogging_HPP__

#include "ws_decoupledlogging_esp.ipp"
#include "espp.hpp"
#include "environment.hpp"
#include "logthread.hpp"

class CWSDecoupledLogSoapBindingEx : public CWSDecoupledLogSoapBinding
{
public:

    CWSDecoupledLogSoapBindingEx(http_soap_log_level level=hsl_none) : CWSDecoupledLogSoapBinding(level) { };

    CWSDecoupledLogSoapBindingEx(IPropertyTree* cfg, const char *bindname, const char *procname, http_soap_log_level level=hsl_none)
        : CWSDecoupledLogSoapBinding(cfg, bindname, procname, level) { };
};

class CWSDecoupledLogEx : public CWSDecoupledLog
{
    StringAttr espProcess, tankFile;
    IEspContainer* container;

    typedef std::vector<IUpdateLogThread*> LOGGING_AGENTTHREADS;
    LOGGING_AGENTTHREADS  loggingAgentThreads;

    IEspLogAgent* loadLoggingAgent(const char* name, const char* dll, const char* service, IPropertyTree* cfg);
    bool checkName(const char* name, StringArray& names, bool defaultValue);

public:
    IMPLEMENT_IINTERFACE;

    virtual void setContainer(IEspContainer* _container)
    {
        container = _container;
    }

    virtual void init(IPropertyTree* cfg, const char* process, const char* service);
    virtual bool onGetLogAgentSetting(IEspContext& context, IEspGetLogAgentSettingRequest& req, IEspGetLogAgentSettingResponse& resp);
    virtual bool onPauseLog(IEspContext& context, IEspPauseLogRequest& req, IEspPauseLogResponse& resp);
};

#endif //_ESPWIZ_ws_tranlogging_HPP__
