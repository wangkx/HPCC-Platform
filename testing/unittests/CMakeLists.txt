################################################################################
#    Copyright (C) 2012 HPCC Systems®.
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


# Component: unittests
#####################################################
# Description:
# ------------
#    Cmake Input File for unittests
#####################################################

if (USE_CPPUNIT)

project( unitests )

include(${HPCC_SOURCE_DIR}/esp/scm/smcscm.cmake)

set (    SRCS
         ${ESPSCM_GENERATED_DIR}/common_esp.cpp
         ${ESPSCM_GENERATED_DIR}/ws_workunits_esp.cpp
         ${HPCC_SOURCE_DIR}/dali/sasha/sacmd.cpp
         unittests.cpp
         remotetests.cpp
         dalitests.cpp
         jlibtests.cpp
         sashatests.cpp
    )

include_directories (
         .
         ./../../system/include
         ./../../system/jlib
         ./../../system/mp
         ./../../common/remote
         ./../../dali/base
         ./../../dali/sasha
         ./../../system/security/shared
         ./../../common/deftype
         ./../../system/xmllib
         ./../../esp/platform
         ./../../esp/bindings
         ./../../esp/bindings/SOAP/xpp
         ./../../esp/bindings/http/client
    )

ADD_DEFINITIONS( -D_CONSOLE )

HPCC_ADD_EXECUTABLE ( unittests ${SRCS} )

install ( TARGETS unittests RUNTIME DESTINATION ${EXEC_DIR} )
target_link_libraries ( unittests
         jlib
         remote
         dalibase
         deftype
         esphttp
         ${CPPUNIT_LIBRARIES}
    )

endif ()