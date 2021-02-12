/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2021 HPCC Systems®.

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

#ifndef _ESPWIZ_ws_container_HPP__
#define _ESPWIZ_ws_container_HPP__

#ifdef _USE_OPENLDAP
#include "ldapsecurity.ipp"
#endif

#include "ws_container_esp.ipp"
#include "TpWrapper.hpp"

class CWsContainerEx : public CWsContainer
{
    CTpWrapper tpWrapper;

public:
    IMPLEMENT_IINTERFACE;
    virtual ~CWsContainerEx() {};
    virtual void init(IPropertyTree *cfg, const char *process, const char *service) {};

     virtual bool onServiceQuery(IEspContext &context, IEspServiceQueryRequest &req, IEspServiceQueryResponse &resp) override;
};

#endif //_ESPWIZ_ws_container_HPP__

