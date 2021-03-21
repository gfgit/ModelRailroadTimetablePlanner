cmake_minimum_required(VERSION 3.5)
include(CMakeDependentOption)

project(TrainTimetable VERSION 5.25.0 LANGUAGES CXX)

option(UPDATE_TS "Update translations" OFF)
option(UPDATE_TS_KEEP_OBSOLETE "Keep obsolete entries when updating translations" ON)
option(BUILD_DOXYGEN "Build Doxygen documentation" OFF)

if (WIN32)
    option(RUN_WINDEPLOYQT "Run windeployqt after executable is installed" ON)
endif()

## Defines ##

set(DB_FORMAT_VERSION 8)

set(APP_PRODUCT_NAME "TrainTimeTable")
set(APP_DISPLAY_NAME "Train Timetable")
set(APP_COMPANY_NAME "Train Software")

set(PROJECT_HOMEPAGE_URL "www.pollofrittomachebuono.altervista.org")
set(APP_HELP_URL ${APP_ABOUT_URL})
set(APP_UPDATE_URL ${APP_ABOUT_URL})

set(PROJECT_DESCRIPTION "${APP_DISPLAY_NAME} lets you create and manage model railway sessions")

set(APP_ICON ${CMAKE_SOURCE_DIR}/files/icons/icon.ico)

# Create main application target
set(TRAINTIMETABLE_TARGET "train-timetable")

## defines end ##

# NSIS Installer
if(WIN32)
    configure_file(packaging/windows/NSIS/constants.nsh.in ${CMAKE_BINARY_DIR}/NSIS/constants.nsh @ONLY)
    configure_file(packaging/windows/NSIS/installer.nsi ${CMAKE_BINARY_DIR}/NSIS/installer.nsi COPYONLY)
endif()

add_custom_target(NSIS
    DEPENDS ${TRAINTIMETABLE_TARGET}
    #COMMAND TODO add NSIS compiler
    SOURCES
    packaging/windows/NSIS/constants.nsh.in
    packaging/windows/NSIS/installer.nsi
    packaging/windows/resources.rc.in
    VERBATIM)

## CUSTOM CONFIGURATION ##

option(CONFIG_GLOBAL_TRY_CATCH "Global try/catch at main()" OFF)
option(CONFIG_NO_DEBUG_CALL_TRACE "Disable scope call trace messages" OFF)
option(CONFIG_PRINT_DBG_MSG "Debug messages (some)" ON)
option(CONFIG_ENABLE_BACKGROUND_MANAGER "Enable background task manager" ON)
cmake_dependent_option(CONFIG_ENABLE_RS_CHECKER "Enable rollingstock checker" ON "CONFIG_ENABLE_BACKGROUND_MANAGER" OFF)
cmake_dependent_option(CONFIG_SEARCHBOX_MODE_ASYNC "Use thread to search for jobs" ON "CONFIG_ENABLE_BACKGROUND_MANAGER" OFF)
option(CONFIG_ENABLE_AUTO_TIME_RECALC "Automatic recalculation of travel times based on rollingstock speed, experimental" OFF)
option(CONFIG_ENABLE_USER_QUERY "Enable SQL console" OFF)

if(CONFIG_GLOBAL_TRY_CATCH)
    set(TRAINTIMETABLE_DEFINITIONS ${TRAINTIMETABLE_DEFINITIONS} -DGLOBAL_TRY_CATCH)
endif()

if(CONFIG_NO_DEBUG_CALL_TRACE)
    set(TRAINTIMETABLE_DEFINITIONS ${TRAINTIMETABLE_DEFINITIONS} -DNO_DEBUG_CALL_TRACE)
endif()

if(CONFIG_PRINT_DBG_MSG)
    set(TRAINTIMETABLE_DEFINITIONS ${TRAINTIMETABLE_DEFINITIONS} -DPRINT_DBG_MSG)
endif()

if(CONFIG_ENABLE_BACKGROUND_MANAGER)
    set(TRAINTIMETABLE_DEFINITIONS ${TRAINTIMETABLE_DEFINITIONS} -DENABLE_BACKGROUND_MANAGER)
endif()

if(CONFIG_ENABLE_RS_CHECKER)
    set(TRAINTIMETABLE_DEFINITIONS ${TRAINTIMETABLE_DEFINITIONS} -DENABLE_RS_CHECKER)
endif()

if(CONFIG_SEARCHBOX_MODE_ASYNC)
    set(TRAINTIMETABLE_DEFINITIONS ${TRAINTIMETABLE_DEFINITIONS} -DSEARCHBOX_MODE_ASYNC)
endif()

if(CONFIG_ENABLE_AUTO_TIME_RECALC)
    set(TRAINTIMETABLE_DEFINITIONS ${TRAINTIMETABLE_DEFINITIONS} -DENABLE_AUTO_TIME_RECALC)
endif()

if(CONFIG_ENABLE_USER_QUERY)
    set(TRAINTIMETABLE_DEFINITIONS ${TRAINTIMETABLE_DEFINITIONS} -DENABLE_USER_QUERY)
endif()

## Config end ##

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(TRAINTIMETABLE_DEFINITIONS ${TRAINTIMETABLE_DEFINITIONS} -DAPPVERSION="${PROJECT_VERSION}" -DQT_DEPRECATED_WARNINGS)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(Qt5 REQUIRED
    COMPONENTS
    Core
    Gui
    Widgets
    Svg
    PrintSupport
    LinguistTools)

find_package(SQLite3)
find_package(ZLIB)

# Locate libzip
if(NOT ZIP_INCLUDE_DIR OR NOT ZIP_LIBRARY)
    set(ZIP_INCLUDE_DIR NOTFOUND CACHE PATH "Path to libzip include directory, contains 'zip.h' file.")
    set(ZIP_LIBRARY NOTFOUND CACHE FILEPATH "Path to libzip library (shared DLL usually libzip.dll).")
    message(FATAL_ERROR "libzip NOT FOUND (set ZIP_INCLUDE_DIR and ZIP_LIBRARY)")
endif()

if(BUILD_DOXYGEN)
    find_package(Doxygen)
endif()


#Locate windeployqt
if(WIN32 AND RUN_WINDEPLOYQT AND NOT WINDEPLOYQT_EXE)
    set(WINDEPLOYQT_EXE_TMP NOTFOUND)
    message("Searching windeployqt executable")
    if(QT_QMAKE_EXECUTABLE)
        get_filename_component(WINDEPLOYQT_DIR ${QT_QMAKE_EXECUTABLE} DIRECTORY)
        set(WINDEPLOYQT_EXE_TMP "${WINDEPLOYQT_DIR}/windeployqt.exe")
    endif()
    if(NOT EXISTS ${WINDEPLOYQT_EXE_TMP} AND Qt5_DIR)
        get_filename_component(WINDEPLOYQT_EXE_TMP "${Qt5_DIR}/../../../bin/windeployqt.exe" REALPATH)
    endif()

    if(EXISTS ${WINDEPLOYQT_EXE_TMP})
        message("Found ${WINDEPLOYQT_EXE_TMP}")
    else()
        message("windeployqt NOT FOUND")
        set(WINDEPLOYQT_EXE_TMP NOTFOUND)
    endif()
    set(WINDEPLOYQT_EXE ${WINDEPLOYQT_EXE_TMP} CACHE FILEPATH "windeployqt executable file path.")
    unset(WINDEPLOYQT_EXE_TMP)
    unset(WINDEPLOYQT_DIR)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(src)
add_subdirectory(packaging)

