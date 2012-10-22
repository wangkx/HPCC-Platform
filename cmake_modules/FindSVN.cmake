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


# - Try to find the SVN library
# Once done this will define
#
#  SVN_FOUND - system has the SVN library
#  SVN_INCLUDE_DIR - the SVN include directory
#  SVN_LIBRARIES - The libraries needed to use SVN

IF (NOT SVN_FOUND)
  IF (NOT ${EXTERNALS_DIRECTORY} STREQUAL "")
    IF (UNIX)
      IF (${ARCH64BIT} EQUAL 1)
        SET (osincdir "subversion/include")
        SET (oslibdir "subversion/linux64_gcc4.1.1")
      ELSE()
        SET (osincdir "subversion/include")
        SET (oslibdir "subversion/linux32_gcc4.1.1")
      ENDIF()
    ELSEIF(WIN32)
        SET (osincdir "subversion/include")
        SET (oslibdir "subversion/lib")
    ELSE()
      SET (osincdir "unknown")
    ENDIF()
    
    IF (NOT ("${osincdir}" STREQUAL "unknown"))
      FIND_PATH (SVN_INCLUDE_DIR NAMES apr.h PATHS "${EXTERNALS_DIRECTORY}/${osincdir}" NO_DEFAULT_PATH)
      FIND_LIBRARY (SVN_LIBRARIES NAMES libapr-1 PATHS "${EXTERNALS_DIRECTORY}/${oslibdir}" NO_DEFAULT_PATH)
      FIND_LIBRARY (SVN_LIBRARIES NAMES libsvn_subr-1 PATHS "${EXTERNALS_DIRECTORY}/${oslibdir}" NO_DEFAULT_PATH)
    ENDIF()
  ENDIF()

  if (USE_NATIVE_LIBRARIES)
    # if we didn't find in externals, look in system include path
    FIND_PATH (SVN_INCLUDE_DIR NAMES apr.h)
    FIND_LIBRARY (SVN_LIBRARIES NAMES libapr-1)
    FIND_LIBRARY (SVN_LIBRARIES NAMES libsvn_subr-1)
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(SVN DEFAULT_MSG
    SVN_LIBRARIES 
    SVN_INCLUDE_DIR
  )
  IF (SVN_FOUND)
    IF(WIN32)
        STRING(REPLACE "libapr-1" "libaprutil-1" SVN_EXTRA1 "${SVN_LIBRARIES}")
        STRING(REPLACE "libapr-1" "libsvn_delta-1" SVN_EXTRA2 "${SVN_LIBRARIES}")
        STRING(REPLACE "libapr-1" "libsvn_diff-1" SVN_EXTRA3 "${SVN_LIBRARIES}")
        STRING(REPLACE "libapr-1" "libsvn_subr-1" SVN_EXTRA4 "${SVN_LIBRARIES}")
        STRING(REPLACE "libapr-1" "libsvn_wc-1" SVN_EXTRA5 "${SVN_LIBRARIES}")
        STRING(REPLACE "libapr-1" "libsvn_client-1" SVN_EXTRA6 "${SVN_LIBRARIES}")
        set (SVN_LIBRARIES ${SVN_LIBRARIES} ${SVN_EXTRA1} ${SVN_EXTRA2} ${SVN_EXTRA3} ${SVN_EXTRA4} ${SVN_EXTRA5} ${SVN_EXTRA6} )
    ENDIF()
  ENDIF()
ENDIF()
