# - Try to find the CHECK libraries
#  Once done this will define
#
#  CHECK_FOUND - system has check
#  CHECK_INCLUDE_DIRS - the check include directory
#  CHECK_LIBRARIES - check library
#
#  Copyright (c) 2007 Daniel Gollub <gollub@b1-systems.de>
#  Copyright (c) 2007-2009 Bjoern Ricks  <bjoern.ricks@gmail.com>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
# Look for the header file.

FIND_PATH(CHECK_INCLUDE_DIR NAMES check.h)
MARK_AS_ADVANCED(CHECK_INCLUDE_DIR)

# Look for the library.
FIND_LIBRARY(CHECK_LIBRARY NAMES 
    check 
)
MARK_AS_ADVANCED(CHECK_LIBRARY)

# handle the QUIETLY and REQUIRED arguments and set CHECK_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CHECK DEFAULT_MSG CHECK_LIBRARY CHECK_INCLUDE_DIR)

IF(CHECK_FOUND)
  SET(CHECK_LIBRARIES ${CHECK_LIBRARY})
  SET(CHECK_INCLUDE_DIRS ${CHECK_INCLUDE_DIR})
ENDIF(CHECK_FOUND)
