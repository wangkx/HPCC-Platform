################################################################################
#    HPCC SYSTEMS software Copyright (C) 2015 HPCC Systems.
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


# Component: ws_pdm 
#####################################################
# Description:
# ------------
#    Cmake Input File for ws_pdm
#####################################################

project( ws_pdm ) 

include(${HPCC_SOURCE_DIR}/esp/scm/espscm.cmake)

set (    SRCS 
         ${ESPSCM_GENERATED_DIR}/ws_pdm_esp.cpp
         ws_pdmplugin.cpp 
         ws_pdmservice.cpp 
    )

include_directories ( 
         ./../.. 
         ./../../platform 
         ./../../../system/jlib 
         ./../../services 
         ./../../../system/xmllib 
         ./../../../system/security/shared 
         ./../../../system/security/securesocket 
         ./../../../system/include 
         ./../../clients 
         ./../../bindings 
         ./../../bindings/SOAP/xpp
         ./../../pdmlib
    )

# NOTE - this should not be needed, it's the result of poor encapsulation and using CLdapSecManager directly 
include_directories ( ${OPENLDAP_INCLUDE_DIR} )

ADD_DEFINITIONS( -D_USRDLL )

HPCC_ADD_LIBRARY( ws_pdm SHARED ${SRCS} )
install ( TARGETS ws_pdm RUNTIME DESTINATION ${EXEC_DIR} LIBRARY DESTINATION ${LIB_DIR} COMPONENT Runtime )
add_dependencies (ws_pdm espscm)
target_link_libraries ( ws_pdm 
         jlib
         xmllib 
         esphttp 
         securesocket 
    )
