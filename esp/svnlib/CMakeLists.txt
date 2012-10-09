################################################################################
#    Copyright (C) 2011 HPCC Systems.
#
#    All rights reserved. This program is free software: you can redistribute it and/or modify
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

# Component: svnlib 
#####################################################
# Description:
# ------------
#    Cmake Input File for svnlib
#####################################################

project( svnlib ) 

set (SVN_INCLUDE_DIR "C:\\seisint_src\\svncpp2012\\include")

IF (UNIX)
  IF (${ARCH64BIT} EQUAL 1)
    SET (SVN_LIBRARIES "linux64_gcc4.1.1")
  ELSE()
    SET (SVN_LIBRARIES "linux32_gcc4.1.1")
  ENDIF()
ELSEIF(WIN32)
  SET (SVN_LIBRARIES "C:\\seisint_src\\svncpp2012\\lib\\Debug\\libsvn_client-1.lib"
  "C:\\seisint_src\\svncpp2012\\lib\\Debug\\libapr-1.lib"
  "C:\\seisint_src\\svncpp2012\\lib\\Debug\\libaprutil-1.lib"
  "C:\\seisint_src\\svncpp2012\\lib\\Debug\\libsvn_delta-1.lib"
  "C:\\seisint_src\\svncpp2012\\lib\\Debug\\libsvn_diff-1.lib"
  "C:\\seisint_src\\svncpp2012\\lib\\Debug\\libsvn_subr-1.lib"
  "C:\\seisint_src\\svncpp2012\\lib\\Debug\\libsvn_wc-1.lib")
ELSE()
  SET (SVN_LIBRARIES "unknown")
ENDIF()

set(LIB_LIBRARIES ${SVN_LIBRARIES})

set (    SRCS 
    ${SCM_GENERATED_INCLUDES}
        apr.cpp
        client.cpp
        client_annotate.cpp
        client_cat.cpp
        client_diff.cpp
        client_ls.cpp
        client_modify.cpp
        client_property.cpp
        client_status.cpp
        context.cpp
        datetime.cpp
        dirent.cpp
        entry.cpp
        exception.cpp
        info.cpp
        log_entry.cpp
        path.cpp
        pool.cpp
        property.cpp
        revision.cpp
        status.cpp
        status_selection.cpp
        targets.cpp
        url.cpp
        wc.cpp
        md5.cpp
        md5wrapper.cpp
        MySVN.cpp
        SVNContextListener.cpp
        SVNObjects.cpp
    )

set (    INCLUDES
        )


include_directories ( 
         ../../system/win32 
         ../../system/include 
         ../../system/jlib
         ../../system/scm
         ${SVN_INCLUDE_DIR}
    )

ADD_DEFINITIONS( -DLOGMSGCOMPONENT=2 -D_USRDLL -DSVNLIB_EXPORTS )

HPCC_ADD_LIBRARY( svnlib ${SRCS} )
install ( TARGETS svnlib DESTINATION ${OSSDIR}/lib )
target_link_libraries ( svnlib
         ${LIB_LIBRARIES}
         jlib
    )
