cmake_minimum_required(VERSION 3.17)
include(CMakeDependentOption)

project(ModelRailroadTimetablePlanner VERSION 6.2.1 LANGUAGES CXX)

option(UPDATE_TS "Update translations" OFF)
option(UPDATE_TS_KEEP_OBSOLETE "Keep obsolete entries when updating translations" ON)
option(BUILD_DOXYGEN "Build Doxygen documentation" OFF)

if (WIN32)
    option(RUN_WINDEPLOYQT "Run windeployqt after executable is installed" ON)
endif()

## Defines ##

set(DB_FORMAT_VERSION 8)

set(APP_PRODUCT_NAME "ModelRailroadTimetablePlanner")
set(APP_DISPLAY_NAME "Model Railroad Timetable Planner")
set(APP_COMPANY_NAME "Train Software")
set(APP_COMPANY_NAME_LOWER "trainsoftware")

set(PROJECT_HOMEPAGE_URL "https://github.com/gfgit/ModelRailroadTimetablePlanner")
set(APP_HELP_URL ${PROJECT_HOMEPAGE_URL})
set(APP_UPDATE_URL ${PROJECT_HOMEPAGE_URL})

string(CONCAT PROJECT_DESCRIPTION "${APP_DISPLAY_NAME} is a cross-platform C++\n"
                        "application with Qt GUI for model railway timetable scheduling.\n\n"
                        "It lets you create and manage model railway sessions\n"
                        "and provides all documents useful for driving and dispatching\n"
                        "trains on big layouts like in FREMO Meetings.\n\n"
                        "Main features:\n"
                        " * Railway timetable graph per each line in SVG, PDF or printed\n"
                        " * Group jobs (trains) in work shift\n"
                        " * Export booklets in ODT (LibreOffice Writer) for shifts and stations\n"
                        " * Importation of rollingstock pieces from\n"
                        "   other sessions or ODS Spreadsheet")

set(PROJECT_DESCRIPTION_SHORT "Timetables for model railroads")

set(APP_ICON ${CMAKE_SOURCE_DIR}/files/icons/icon.ico)

# Create main application target
set(MR_TIMETABLE_PLANNER_TARGET "mrtplanner")

## defines end ##

## CUSTOM CONFIGURATION ##

option(CONFIG_GLOBAL_TRY_CATCH "Global try/catch at main()" OFF)
option(CONFIG_NO_DEBUG_CALL_TRACE "Disable scope call trace messages" OFF)
option(CONFIG_PRINT_DBG_MSG "Debug messages (some)" ON)
option(CONFIG_ENABLE_BACKGROUND_MANAGER "Enable background task manager" ON)
cmake_dependent_option(CONFIG_SEARCHBOX_MODE_ASYNC "Use thread to search for jobs" ON "CONFIG_ENABLE_BACKGROUND_MANAGER" OFF)
option(CONFIG_ENABLE_AUTO_TIME_RECALC "Automatic recalculation of travel times based on rollingstock speed, experimental" OFF)
option(CONFIG_ENABLE_USER_QUERY "Enable SQL console" OFF)

if(CONFIG_GLOBAL_TRY_CATCH)
    set(MR_TIMETABLE_PLANNER_DEFINITIONS ${MR_TIMETABLE_PLANNER_DEFINITIONS} -DGLOBAL_TRY_CATCH)
endif()

if(CONFIG_NO_DEBUG_CALL_TRACE)
    set(MR_TIMETABLE_PLANNER_DEFINITIONS ${MR_TIMETABLE_PLANNER_DEFINITIONS} -DNO_DEBUG_CALL_TRACE)
endif()

if(CONFIG_PRINT_DBG_MSG)
    set(MR_TIMETABLE_PLANNER_DEFINITIONS ${MR_TIMETABLE_PLANNER_DEFINITIONS} -DPRINT_DBG_MSG)
endif()

if(CONFIG_ENABLE_BACKGROUND_MANAGER)
    set(MR_TIMETABLE_PLANNER_DEFINITIONS ${MR_TIMETABLE_PLANNER_DEFINITIONS} -DENABLE_BACKGROUND_MANAGER)
endif()

if(CONFIG_SEARCHBOX_MODE_ASYNC)
    set(MR_TIMETABLE_PLANNER_DEFINITIONS ${MR_TIMETABLE_PLANNER_DEFINITIONS} -DSEARCHBOX_MODE_ASYNC)
endif()

if(CONFIG_ENABLE_AUTO_TIME_RECALC)
    set(MR_TIMETABLE_PLANNER_DEFINITIONS ${MR_TIMETABLE_PLANNER_DEFINITIONS} -DENABLE_AUTO_TIME_RECALC)
endif()

if(CONFIG_ENABLE_USER_QUERY)
    set(MR_TIMETABLE_PLANNER_DEFINITIONS ${MR_TIMETABLE_PLANNER_DEFINITIONS} -DENABLE_USER_QUERY)
endif()

## Config end ##

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(MR_TIMETABLE_PLANNER_DEFINITIONS ${MR_TIMETABLE_PLANNER_DEFINITIONS} -DAPPVERSION="${PROJECT_VERSION}" -DQT_DEPRECATED_WARNINGS)

# Include our custom FindXXX moudules in '/cmake' subdirectory
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
find_package(ssplib)

# Prefer config files shipped with 'libzip'
# If not found, it will fallback to out custom FindLibZip.cmake module
# Located in '/cmake' subdirectory
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG true)
find_package(LibZip)
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG false)

if(BUILD_DOXYGEN)
    find_package(Doxygen)
endif()

include(LocateWinDeployQt)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(src)
add_subdirectory(packaging)

include(Packing)
