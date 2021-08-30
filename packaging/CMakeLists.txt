# Add out NSIS template to CMake
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/packaging ${CMAKE_MODULES_PATH})

# Setup CPack variables

set(CPACK_PACKAGE_VENDOR ${APP_COMPANY_NAME})

set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CMAKE_PROJECT_NAME})

set(CPACK_RESOURCE_FILE_LICENSE ${APP_LICENSE})

#################################################
# NSIS
#################################################

set(CPACK_GENERATOR NSIS)

set(CPACK_PACKAGE_EXECUTABLES "${MR_TIMETABLE_PLANNER_TARGET};${APP_DISPLAY_NAME}")

set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)

# Convert path to escaped (double) backslash like 'C:\\...' for NSIS
#string(REPLACE "/" "\\\\" CPACK_PACKAGE_ICON ${CPACK_PACKAGE_ICON})

#################################################
# Add windeployqt artifacts to installer
#################################################

# Get install prefix with escaped backslash for NSIS
string(REPLACE "/" "\\\\" INST_DIR_ESC ${CMAKE_INSTALL_PREFIX})

# Some DLL are already installed by CMake, do not count them twice
set(EXCLUDED_DLLS "")
string(APPEND EXCLUDED_DLLS "/x sqlite3.dll")
string(APPEND EXCLUDED_DLLS " /x libzip.dll")
string(APPEND EXCLUDED_DLLS " /x libzlib.dll")

set(CMD_LIST_NSIS "")

# Add top level DLLs
list(APPEND CMD_LIST_NSIS "File ${EXCLUDED_DLLS} \\\"${INST_DIR_ESC}\\\\*.dll\\\"")

# Add icon engines
list(APPEND CMD_LIST_NSIS "SetOutPath \\\"$INSTDIR\\\\iconengines\\\"")
list(APPEND CMD_LIST_NSIS "File \\\"${INST_DIR_ESC}\\\\iconengines\\\\*.dll\\\"")

# Add image formats
list(APPEND CMD_LIST_NSIS "SetOutPath \\\"$INSTDIR\\\\imageformats\\\"")
list(APPEND CMD_LIST_NSIS "File \\\"${INST_DIR_ESC}\\\\imageformats\\\\*.dll\\\"")

# Add platforms
list(APPEND CMD_LIST_NSIS "SetOutPath \\\"$INSTDIR\\\\platforms\\\"")
list(APPEND CMD_LIST_NSIS "File \\\"${INST_DIR_ESC}\\\\platforms\\\\*.dll\\\"")

# Add print support
list(APPEND CMD_LIST_NSIS "SetOutPath \\\"$INSTDIR\\\\printsupport\\\"")
list(APPEND CMD_LIST_NSIS "File \\\"${INST_DIR_ESC}\\\\printsupport\\\\*.dll\\\"")

# Add styles
list(APPEND CMD_LIST_NSIS "SetOutPath \\\"$INSTDIR\\\\styles\\\"")
list(APPEND CMD_LIST_NSIS "File \\\"${INST_DIR_ESC}\\\\styles\\\\*.dll\\\"")

# Add translations
list(APPEND CMD_LIST_NSIS "SetOutPath \\\"$INSTDIR\\\\translations\\\"")
list(APPEND CMD_LIST_NSIS "File \\\"${INST_DIR_ESC}\\\\translations\\\\qt_*.qm\\\"")

# Go back to main install directory so CPack can continue
list(APPEND CMD_LIST_NSIS "SetOutPath \\\"$INSTDIR\\\"")

# Join to new-line separated string
list(JOIN CMD_LIST_NSIS "\n" CMD_EXTRA_NSIS)

set(CPACK_NSIS_EXTRA_PREINSTALL_COMMANDS ${CMD_EXTRA_NSIS})

#################################################
# Add windeployqt artifacts to Uninstaller
#################################################

set(CMD_LIST_NSIS "")

# Remove top level DLLs
list(APPEND CMD_LIST_NSIS "Delete \\\"$INSTDIR\\\\*.dll\\\"")

# Remove icon engines
list(APPEND CMD_LIST_NSIS "Delete \\\"$INSTDIR\\\\iconengines\\\\*.dll\\\"")
list(APPEND CMD_LIST_NSIS "RMDir \\\"$INSTDIR\\\\iconengines\\\"")

# Remove image formats
list(APPEND CMD_LIST_NSIS "Delete \\\"$INSTDIR\\\\imageformats\\\\*.dll\\\"")
list(APPEND CMD_LIST_NSIS "RMDir \\\"$INSTDIR\\\\imageformats\\\"")

# Remove platforms
list(APPEND CMD_LIST_NSIS "Delete \\\"$INSTDIR\\\\platforms\\\\*.dll\\\"")
list(APPEND CMD_LIST_NSIS "RMDir \\\"$INSTDIR\\\\platforms\\\"")

# Remove print support
list(APPEND CMD_LIST_NSIS "Delete \\\"$INSTDIR\\\\printsupport\\\\*.dll\\\"")
list(APPEND CMD_LIST_NSIS "RMDir \\\"$INSTDIR\\\\printsupport\\\"")

# Remove styles
list(APPEND CMD_LIST_NSIS "Delete \\\"$INSTDIR\\\\styles\\\\*.dll\\\"")
list(APPEND CMD_LIST_NSIS "RMDir \\\"$INSTDIR\\\\styles\\\"")

# Remove translations
list(APPEND CMD_LIST_NSIS "Delete \\\"$INSTDIR\\\\translations\\\\qt_*.qm\\\"")
list(APPEND CMD_LIST_NSIS "RMDir \\\"$INSTDIR\\\\translations\\\"")

# Join to new-line separated string
list(JOIN CMD_LIST_NSIS "\n" CMD_EXTRA_NSIS)

set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS ${CMD_EXTRA_NSIS})


include(CPack)
