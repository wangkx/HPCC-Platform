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

# Component: mysql 
#####################################################
# Description:
# ------------
#    Cmake Input File for mysql
#####################################################

project( mysql ) 

set (    SRCS 
         mysql.cpp 
         pool.cpp 
    )

include_directories ( 
         ${HPCC_SOURCE_DIR}/system/include 
         ./include 
         ${HPCC_SOURCE_DIR}/system/jlib 
         ${HPCC_SOURCE_DIR}/esp/esplib 
         ${MYSQL_INCLUDE_DIR}
    )

ADD_DEFINITIONS( -D_USRDLL -DMYSQL_EXPORTS )

HPCC_ADD_LIBRARY( mysql SHARED ${SRCS} )
install ( TARGETS mysql RUNTIME DESTINATION bin LIBRARY DESTINATION lib )
target_link_libraries ( mysql 
         ${MYSQL_LIBRARIES}
         jlib
    )
