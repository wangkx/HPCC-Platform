################################################################################
#    Copyright (C) <2010>  <LexisNexis Risk Data Management Inc.>
#
#    All rights reserved. This program is NOT PRESENTLY free software: you can NOT redistribute it and/or modify
#    it under the terms of the GNU Affero General Public License as
#    published by the Free Software Foundation, either version 3 of the
#    License, or (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU Affero General Public License for more details.
#
#    You should have received a copy of the GNU Affero General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.
################################################################################


# Component: ws_orbiz 
#####################################################
# Description:
# ------------
#    Cmake Input File for ws_orbiz
#####################################################

project( ws_orbiz ) 

include(${HPCC_SOURCE_DIR}/esp/scm/espscm.cmake)

set (    SRCS 
         ${ESPSCM_GENERATED_DIR}/ws_orbiz_esp.cpp 
         orbizdatamanager.cpp 
         mysqldatamanager.cpp 
         ws_orbizplugin.cpp 
         ws_orbizservice.cpp 
    )

include_directories ( 
         ${CMAKE_BINARY_DIR}/oss
         ${HPCC_SOURCE_DIR}/esp/esplib 
         ${HPCC_SOURCE_DIR}/esp/platform 
         ${HPCC_SOURCE_DIR}/system/jlib 
         ${HPCC_SOURCE_DIR}/esp/services 
         ${HPCC_SOURCE_DIR}/system/xmllib 
         ${HPCC_SOURCE_DIR}/common/deftype 
         ${HPCC_SOURCE_DIR}/ecl/hql 
         ${HPCC_SOURCE_DIR}/system/security/securesocket 
         ${HPCC_SOURCE_DIR}/system/include 
         ${HPCC_SOURCE_DIR}/esp/clients 
         ${HPCC_SOURCE_DIR}/system/security/shared 
         ${HPCC_SOURCE_DIR}/esp/bindings 
         ${HPCC_SOURCE_DIR}/system/mysql
         ${HPCC_SOURCE_DIR}/esp/bindings/SOAP/xpp 
         ${MYSQL_INCLUDE_DIR}
    )

ADD_DEFINITIONS( -D_USRDLL )

HPCC_ADD_LIBRARY( ws_orbiz SHARED ${SRCS} )
install ( TARGETS ws_orbiz RUNTIME DESTINATION bin LIBRARY DESTINATION lib )
add_dependencies (ws_orbiz espscm)
target_link_libraries ( ws_orbiz 
         jlib
         xmllib 
         esphttp 
         mp 
         hrpc 
         nbcd 
         eclrtl 
         deftype 
         hql 
         remote 
         dalibase 
         securesocket 
    )

IF (USE_MYSQL_REPOSITORY)
  target_link_libraries ( ws_orbiz  mysql)
ENDIF()
