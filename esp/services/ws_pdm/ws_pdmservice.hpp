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

#ifndef _ESPWIZ_ws_pdm_HPP__
#define _ESPWIZ_ws_pdm_HPP__

#include "pdmlibshare.hpp"

class CWsPDMServiceEx : public CWsPDMService
{
    Owned<IPDMServiceLib> pdmServiceLib;
    void loadPDMServiceLib(const char * libName);

public:
    IMPLEMENT_IINTERFACE;

    virtual void init(IPropertyTree *cfg, const char *process, const char *service);

    virtual bool onEchoTest(IEspContext &context, IEspEchoTestRequest &req, IEspEchoTestResponse &resp);

    virtual bool onCreateDeploymentPackage(IEspContext &context, IEspCreateDeploymentPackageRequest &req, IEspCreateDeploymentPackageResponse &resp);
    virtual bool onUpdateDeploymentPackage(IEspContext &context, IEspUpdateDeploymentPackageRequest &req, IEspUpdateDeploymentPackageResponse &resp);
    virtual bool onDeleteDeploymentPackage(IEspContext &context, IEspDeleteDeploymentPackageRequest &req, IEspDeleteDeploymentPackageResponse &resp);
    virtual bool onDeploy(IEspContext &context, IEspDeployRequest &req, IEspDeployResponse &resp);
    virtual bool onUndeploy(IEspContext &context, IEspUndeployRequest &req, IEspUndeployResponse &resp);
    virtual bool onStart(IEspContext &context, IEspStartRequest &req, IEspStartResponse &resp);
    virtual bool onStop(IEspContext &context, IEspStopRequest &req, IEspStopResponse &resp);
};

#endif //_ESPWIZ_ws_pdm_HPP__

