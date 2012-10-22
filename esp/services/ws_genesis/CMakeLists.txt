################################################################################
#    HPCC SYSTEMS software Copyright (C) 2012 HPCC Systems.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
################################################################################

# Component: ws_genesis
#####################################################
# Description:
# ------------
#    Cmake Input File for ws_genesis
#####################################################

project( ws_genesis )

include(${HPCC_SOURCE_DIR}/esp/scm/smcscm.cmake)
include(${HPCC_SOURCE_DIR}/esp/scm/additional.cmake)

set (    SRCS
         ${ESPSCM_GENERATED_DIR}/common_esp.cpp
         ${ESPSCM_GENERATED_DIR}/ws_genesis_esp.cpp
         ${HPCC_SOURCE_DIR}/esp/scm/ws_genesis.ecm
         ws_genesisPlugin.cpp
         ws_genesisService.cpp
         ws_genesisService.hpp
         genesis_errors.h
    )

include_directories (
         ./../../../system/include
         ./../../../system/jlib
         ./../../../system/xmllib
         ./../../../system/security/shared
         ./../../../system/security/securesocket
         ./../../../system/svnlib
         ./../../platform
         ./../../clients
         ./../../bindings
         ./../../bindings/SOAP/xpp
         ${SVN_INCLUDE_DIR}
    )

ADD_DEFINITIONS( -D_USRDLL )

HPCC_ADD_LIBRARY( ws_genesis SHARED ${SRCS} )
install ( TARGETS ws_genesis DESTINATION ${OSSDIR}/lib )
target_link_libraries ( ws_genesis
         jlib
         ${XALAN_LIBRARIES} ${XERCES_LIBRARIES}
         xmllib
         esphttp
         svnlib
    )
