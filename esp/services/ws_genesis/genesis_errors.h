/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2012 HPCC Systems.

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

#ifndef GENESIS_ERRORS_H
#define GENESIS_ERRORS_H

#include "errorlist.h"

#define GS_MISSING_PARAM        GS_ERROR_START
#define GS_HOST_EXISTS          GS_ERROR_START+1
#define GS_SUBNET_EXISTS        GS_ERROR_START+2
#define GS_USER_EXISTS          GS_ERROR_START+3
#define GS_USER_GROUP_EXISTS    GS_ERROR_START+4
#define GS_CLUSTER_EXISTS       GS_ERROR_START+5
#define GS_ENVIRONMENT_EXISTS   GS_ERROR_START+6
#define GS_DELETE_NOT_FOUND     GS_ERROR_START+7

#endif
