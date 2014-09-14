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

# Component: utmulticast

#####################################################
# Description:
# ------------
#    Cmake Input File for utmulticast
#####################################################


project( utmulticast )

set (    SRCS
         utmulticast.cpp
    )

include_directories (
         ./../../system/include
         ./../../system/jlib
    )

ADD_DEFINITIONS ( -D_CONSOLE )

HPCC_ADD_EXECUTABLE ( utmulticast ${SRCS} )
#install ( TARGETS utmulticast RUNTIME DESTINATION ${EXEC_DIR} )
target_link_libraries ( utmulticast
         jlib
    )


