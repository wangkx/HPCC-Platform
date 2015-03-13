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

#if !defined __PDMLIBSHARE_HPP__
#define __PDMLIBSHARE_HPP__

#pragma warning (disable : 4786)

#ifdef WIN32
    #ifdef PDMSERVICELIB_EXPORTS
        #define PDMSERVICELIB_API __declspec(dllexport)
    #else
        #define PDMSERVICELIB_API __declspec(dllimport)
    #endif
#else
    #define PDMSERVICELIB_API
#endif

#define LIBPDMSERVICELIB "PDMServiceLib"

#include "jiface.hpp"
#include "esp.hpp"
#include "ws_pdm_esp.ipp"

interface IPDMServiceLib : implements IInterface
{
    virtual bool init(IPropertyTree* loggingConfig, const char* parent) = 0;
    virtual void echoTest(IEspContext &context, IEspEchoTestRequest& req, IEspEchoTestResponse& resp) = 0;
    virtual bool createDeploymentPackage(IEspContext &context, IEspCreateDeploymentPackageRequest &req, IEspCreateDeploymentPackageResponse &resp) = 0;
    virtual bool updateDeploymentPackage(IEspContext &context, IEspUpdateDeploymentPackageRequest &req, IEspUpdateDeploymentPackageResponse &resp) = 0;
    virtual bool deleteDeploymentPackage(IEspContext &context, IEspDeleteDeploymentPackageRequest &req, IEspDeleteDeploymentPackageResponse &resp) = 0;
    virtual bool deploy(IEspContext &context, IEspDeployRequest &req, IEspDeployResponse &resp) = 0;
    virtual bool undeploy(IEspContext &context, IEspUndeployRequest &req, IEspUndeployResponse &resp) = 0;
    virtual bool start(IEspContext &context, IEspStartRequest &req, IEspStartResponse &resp) = 0;
    virtual bool stop(IEspContext &context, IEspStopRequest &req, IEspStopResponse &resp) = 0;
};

typedef IPDMServiceLib* (*newPDMService_t_)();

#endif // !defined __PDMLIBSHARE_HPP__
