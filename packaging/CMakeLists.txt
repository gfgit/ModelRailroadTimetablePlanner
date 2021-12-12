# NSIS Installer

set(MAKENSIS_EXE "makensis")

if(WIN32)
    configure_file(windows/NSIS/constants.nsh.in ${CMAKE_BINARY_DIR}/NSIS/constants.nsh @ONLY)
    configure_file(windows/NSIS/installer.nsi ${CMAKE_BINARY_DIR}/NSIS/installer.nsi COPYONLY)
endif()

add_custom_target(NSIS
    DEPENDS ${MR_TIMETABLE_PLANNER_TARGET} # First build the application
    COMMAND ${MAKENSIS_EXE} installer.nsi
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/NSIS
    COMMENT "Building NSIS Win32 installer..."
    SOURCES #Make this files visible in IDE editor
    windows/NSIS/constants.nsh.in
    windows/NSIS/installer.nsi
    windows/resources.rc.in
    VERBATIM)
