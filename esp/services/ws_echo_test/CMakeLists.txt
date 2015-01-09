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


# Component: ws_echotest
#####################################################
# Description:
# ------------
#    Cmake Input File for ws_echotest
#####################################################

project( ws_echotest )

include(${HPCC_SOURCE_DIR}/esp/scm/espscm.cmake)

set (    SRCS
         ${ESPSCM_GENERATED_DIR}/ws_echo_test_esp.cpp
         ws_echotestplugin.cpp
         ws_echotestservice.cpp
    )

include_directories (
         ./../../platform
         ./../common
         ./../../../system/include
         ./../../../system/jlib
         ./../../../system/security/shared
         ./../../../system/security/securesocket
         ./../../../system/xmllib
         ./../../bindings
         ./../../bindings/SOAP/xpp
    )

ADD_DEFINITIONS( -D_USRDLL )

HPCC_ADD_LIBRARY( ws_echotest SHARED ${SRCS} )
install ( TARGETS ws_echotest RUNTIME DESTINATION ${EXEC_DIR} LIBRARY DESTINATION ${LIB_DIR} )
add_dependencies (ws_echotest espscm)
target_link_libraries ( ws_echotest
         jlib
         ${XALAN_LIBRARIES} ${XERCES_LIBRARIES}
         xmllib
    )
