################################################################################
#    HPCC SYSTEMS software Copyright (C) 2020 HPCC Systems®.
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

# Component: esplogreader 

#####################################################
# Description:
# ------------
#    Cmake Input File for esplogreader
#####################################################


project( esplogreader )

# include(${HPCC_SOURCE_DIR}/esp/scm/espscm.cmake)

set (    SRCS
         main.cpp
    )

include_directories (
         ${CMAKE_BINARY_DIR}
         ${CMAKE_BINARY_DIR}/oss
         ${HPCC_SOURCE_DIR}/system/include
         ${HPCC_SOURCE_DIR}/system/jlib
         ${HPCC_SOURCE_DIR}/system/security/shared
         ${HPCC_SOURCE_DIR}/system/security/securesocket
         ${HPCC_SOURCE_DIR}/system/xmllib
    )

ADD_DEFINITIONS( -D_CONSOLE )

HPCC_ADD_EXECUTABLE ( esplogreader ${SRCS} )
install ( TARGETS esplogreader RUNTIME DESTINATION ${EXEC_DIR} COMPONENT Runtime)

target_link_libraries ( esplogreader
         jlib
    )
IF (USE_OPENSSL)
    target_link_libraries ( esplogreader
         securesocket
    )
ENDIF()

if ( PLATFORM )
    install ( PROGRAMS esplogreader.install DESTINATION etc/init.d/install COMPONENT Runtime )
    install ( PROGRAMS esplogreader.uninstall DESTINATION etc/init.d/uninstall COMPONENT Runtime )
endif()
