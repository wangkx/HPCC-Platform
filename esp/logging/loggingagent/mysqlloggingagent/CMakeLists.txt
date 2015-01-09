################################################################################
#    HPCC SYSTEMS software Copyright (C) 2014 HPCC Systems.
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

# Component: mysqllogagent

project( mysqllogagent )

include_directories (
     ${HPCC_SOURCE_DIR}/system/include
     ${HPCC_SOURCE_DIR}/system/jlib
     ${HPCC_SOURCE_DIR}/system/security/shared
     ${HPCC_SOURCE_DIR}/system/xmllib
     ${HPCC_SOURCE_DIR}/system/mp
     ${HPCC_SOURCE_DIR}/system/security/securesocket
     ${HPCC_SOURCE_DIR}/common/environment
     ${HPCC_SOURCE_DIR}/dali/base
     ${HPCC_SOURCE_DIR}/esp/platform
     ${HPCC_SOURCE_DIR}/esp/bindings
     ${HPCC_SOURCE_DIR}/esp/bindings/SOAP/Platform
     ${HPCC_SOURCE_DIR}/esp/bindings/SOAP/xpp
     ${HPCC_SOURCE_DIR}/esp/bindings/SOAP/client
     ${HPCC_SOURCE_DIR}/esp/esplib
     ${HPCC_SOURCE_DIR}/esp/services
     ${HPCC_SOURCE_DIR}/esp/clients
     ${HPCC_SOURCE_DIR}/esp/clients/LoggingClient
     ${HPCC_SOURCE_DIR}/esp/logging
     ${MYSQL_INCLUDE_DIR}
     ./..
)

ADD_DEFINITIONS( -D_USRDLL  -DMYSQLLOGGINGLISTENER_EXPORTS -DMYSQLLOGAGENT_EXPORTS )

set ( SRCS
    mysqllogagent.cpp
    dbfieldmap.cpp
)

HPCC_ADD_LIBRARY( mysqllogagent SHARED ${SRCS} )

install ( TARGETS mysqllogagent RUNTIME DESTINATION bin LIBRARY DESTINATION lib )
add_dependencies (mysqllogagent espscm)
target_link_libraries ( mysqllogagent
    esphttp
    jlib
    mysql
)
