# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindLibZip
-----------

.. versionadded:: 3.14

Find the Zip libraries, v3

IMPORTED targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` target:

``LibZip::LibZip``

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables if found:

``LibZip_INCLUDE_DIRS``
  where to find zip.h, etc.
``LibZip_LIBRARIES``
  the libraries to link against to use SQLite3.
``LibZip_VERSION``
  version of the LibZip library found
``LibZip_FOUND``
  TRUE if found

#]=======================================================================]

# Look for the necessary header
find_path(LibZip_INCLUDE_DIR NAMES zip.h)
mark_as_advanced(LibZip_INCLUDE_DIR)

# Look for the necessary library
find_library(LibZip_LIBRARY NAMES libzip)
mark_as_advanced(LibZip_LIBRARY)

# Extract version information from the header file
if(LibZip_INCLUDE_DIR)
    file(STRINGS ${LibZip_INCLUDE_DIR}/zipconf.h _ver_line
         REGEX "^#define LIBZIP_VERSION  *\"[0-9]+\\.[0-9]+\\.[0-9]+\""
         LIMIT_COUNT 1)
    string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+"
           LibZip_VERSION "${_ver_line}")
    unset(_ver_line)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibZip
    REQUIRED_VARS LibZip_INCLUDE_DIR LibZip_LIBRARY
    VERSION_VAR LibZip_VERSION)

# Create the imported target
if(LibZip_FOUND)
    set(LibZip_INCLUDE_DIRS ${LibZip_INCLUDE_DIR})
    set(LibZip_LIBRARIES ${LibZip_LIBRARY})
    if(NOT TARGET LibZip::LibZip)
        add_library(LibZip::LibZip UNKNOWN IMPORTED)
        set_target_properties(LibZip::LibZip PROPERTIES
            IMPORTED_LOCATION             "${LibZip_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${LibZip_INCLUDE_DIR}")
    endif()
endif()
