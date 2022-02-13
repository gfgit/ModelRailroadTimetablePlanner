
include(GNUInstallDirs)

include(DLL_Utils)

#Set Win32 resources
if (WIN32)
    configure_file(../packaging/windows/resources.rc.in ${CMAKE_BINARY_DIR}/resources/resources.rc)
    set(MR_TIMETABLE_PLANNER_RESOURCES
        ${MR_TIMETABLE_PLANNER_RESOURCES}
        ${CMAKE_BINARY_DIR}/resources/resources.rc
        )
endif()

add_subdirectory(app)
add_subdirectory(backgroundmanager)
add_subdirectory(db_metadata)
add_subdirectory(graph)
add_subdirectory(jobs)
add_subdirectory(odt_export)
add_subdirectory(printing)
add_subdirectory(rollingstock)
add_subdirectory(searchbox)
add_subdirectory(settings)
add_subdirectory(shifts)
add_subdirectory(sqlconsole)
add_subdirectory(sqlite3pp)
add_subdirectory(stations)
add_subdirectory(translations)
add_subdirectory(utils)
add_subdirectory(viewmanager)

# Set ModelRailroadTimetablePlanner info template file
set(MR_TIMETABLE_PLANNER_SOURCES
    ${MR_TIMETABLE_PLANNER_SOURCES}
    app/info.h.in
    )
configure_file(app/info.h.in ${CMAKE_BINARY_DIR}/include/info.h)

# Add executable
add_executable(${MR_TIMETABLE_PLANNER_TARGET} WIN32
    ${MR_TIMETABLE_PLANNER_SOURCES}
    ${MR_TIMETABLE_PLANNER_UI_FILES}
    ${MR_TIMETABLE_PLANNER_RESOURCES}
    )

# Set compiler options
if(MSVC)
    target_compile_options(
        ${MR_TIMETABLE_PLANNER_TARGET}
        PRIVATE
        /WX
        /wd4267
        /wd4244
        /experimental:external
        /external:anglebrackets
        /external:W0
        "$<$<CONFIG:RELEASE>:/O2>"
        "$<$<COMPILE_LANGUAGE:CXX>:/MP>"
        )
else()
    target_compile_options(
        ${MR_TIMETABLE_PLANNER_TARGET}
        PRIVATE
        "$<$<CONFIG:RELEASE>:-O2>"
        #-Werror
        -Wuninitialized
        -pedantic-errors
        -Wall
        -Wextra
        -Wno-unused-parameter
        -Wshadow
        )
endif()

if(UNIX AND NOT APPLE)
    target_link_options(
        ${MR_TIMETABLE_PLANNER_TARGET}
        PRIVATE
        -rdynamic
        )
endif()

# Set include directories
target_include_directories(
    ${MR_TIMETABLE_PLANNER_TARGET}
    PRIVATE
    ${SQLite3_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}/include #For template files
    )

# Set link libraries
target_link_libraries(
    ${MR_TIMETABLE_PLANNER_TARGET}
    PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Svg
    Qt5::PrintSupport
    ${SQLite3_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ssplib::ssplib
    )

# Link LibZip
if(TARGET libzip::zip)
    # LibZip was found with Config Package
    # Include directory setup is automatic

    target_link_libraries(
        ${MR_TIMETABLE_PLANNER_TARGET}
        PRIVATE
        libzip::zip
        )

    get_target_property(LibZip_LIBRARY_TO_INSTALL libzip::zip LOCATION)
else()
    # LibZip was found with our Find Module
    # Set include directories manually
    target_include_directories(
        ${MR_TIMETABLE_PLANNER_TARGET}
        PRIVATE
        ${LibZip_INCLUDE_DIRS}
        )

    target_link_libraries(
        ${MR_TIMETABLE_PLANNER_TARGET}
        PRIVATE
        ${LibZip_LIBRARIES}
        )
    get_dll_library_from_import_library(${LibZip_LIBRARIES} LibZip_LIBRARY_TO_INSTALL)
endif()

if (WIN32)
    target_link_libraries(
        ${MR_TIMETABLE_PLANNER_TARGET}
        PRIVATE
        DbgHelp
        )
endif()

# Set compiler definitions
target_compile_definitions(${MR_TIMETABLE_PLANNER_TARGET} PRIVATE ${MR_TIMETABLE_PLANNER_DEFINITIONS})

## Doxygen documentation ##
if(DOXYGEN_FOUND)
    set(DOXYGEN_PROJECT_NAME ${APP_DISPLAY_NAME})
    set(DOXYGEN_PROJECT_LOGO ${APP_ICON})
    set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/docs")
    set(DOXYGEN_EXTRACT_ALL "YES")
    set(DOXYGEN_EXTRACT_PRIVATE "YES")
    set(DOXYGEN_DOT_GRAPH_MAX_NODES 100)
    doxygen_add_docs(docs ALL ${MR_TIMETABLE_PLANNER_SOURCES})
endif()
## Doxygen end ##

## Update/Release translations ##

#(Run this target before installing and every time you update translations)
add_custom_target(RELEASE_TRANSLATIONS ALL
    COMMENT "Running translations it_IT...")

if(UPDATE_TS)
    # Run 'lupdate' to parse C++ and UI files and extract translatable strings
    if(UPDATE_TS_KEEP_OBSOLETE)
        add_custom_command(TARGET RELEASE_TRANSLATIONS
            POST_BUILD
            COMMAND ${Qt5_LUPDATE_EXECUTABLE} ARGS ${CMAKE_SOURCE_DIR}/src -ts ${MR_TIMETABLE_PLANNER_TS_FILES}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src
            COMMENT "Updating translations"
            VERBATIM)
    else()
        add_custom_command(TARGET RELEASE_TRANSLATIONS
            POST_BUILD
            COMMAND ${Qt5_LUPDATE_EXECUTABLE} ARGS ${CMAKE_SOURCE_DIR}/src -ts ${MR_TIMETABLE_PLANNER_TS_FILES} -no-obsolete
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src
            COMMENT "Updating translations"
            VERBATIM)
    endif()

endif()

# For each .ts file release a .qm file
foreach(TS_FILE ${MR_TIMETABLE_PLANNER_TS_FILES})
    get_filename_component(QM_FILE_NAME ${TS_FILE} NAME_WLE)
    set(QM_FILE "${CMAKE_BINARY_DIR}/src/translations/${QM_FILE_NAME}.qm")
    message("Generating translation: ${QM_FILE}")
    add_custom_command(TARGET RELEASE_TRANSLATIONS
        POST_BUILD
        COMMAND ${Qt5_LRELEASE_EXECUTABLE} ARGS ${CMAKE_CURRENT_SOURCE_DIR}/${TS_FILE} -qm ${QM_FILE}
        COMMENT "Translating ${QM_FILE_NAME}"
        VERBATIM)
endforeach()

## Update/Release translations end ##

## Install and Deploy ##

# Copy executable
install(TARGETS ${MR_TIMETABLE_PLANNER_TARGET})

# Copy SVG icons
install(FILES ${CMAKE_SOURCE_DIR}/files/icons/lightning/lightning.svg
    DESTINATION ${CMAKE_INSTALL_BINDIR}/icons)

install(FILES ${CMAKE_SOURCE_DIR}/files/icons/reverse_direction/reverse_direction.svg
    DESTINATION ${CMAKE_INSTALL_BINDIR}/icons)

# For each .ts file install corrensponding .qm file
foreach(TS_FILE ${MR_TIMETABLE_PLANNER_TS_FILES})
    get_filename_component(QM_FILE_NAME ${TS_FILE} NAME_WLE)
    install(FILES
        "${CMAKE_BINARY_DIR}/src/translations/${QM_FILE_NAME}.qm"
        DESTINATION ${CMAKE_INSTALL_BINDIR}/translations OPTIONAL)
endforeach()

if(WIN32)
    # Copy SQlite3 DLL
    get_dll_library_from_import_library(${SQLite3_LIBRARIES} SQLite3_LIBRARY_TO_INSTALL)
    install(PROGRAMS ${SQLite3_LIBRARY_TO_INSTALL} DESTINATION ${CMAKE_INSTALL_BINDIR})

    # Copy ZLib DLL
    get_dll_library_from_import_library(${ZLIB_LIBRARIES} ZLIB_LIBRARY_TO_INSTALL)
    install(PROGRAMS ${ZLIB_LIBRARY_TO_INSTALL} DESTINATION ${CMAKE_INSTALL_BINDIR})

    # Copy libzip DLL
    install(PROGRAMS ${LibZip_LIBRARY_TO_INSTALL} DESTINATION ${CMAKE_INSTALL_BINDIR})

    if(RUN_WINDEPLOYQT)
        if(NOT TARGET windeployqt_exe)
            message(FATAL_ERROR "In order to run windeployqt you must first set the exe path in WINDEPLOYQT_EXE_DIR")
        endif()

        # Use [[...]] to delay variable expansion
        install(CODE "
            message(STATUS \"Running windeployqt ${windeployqt_exe} ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}\")
            execute_process(COMMAND ${windeployqt_exe} ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}
                WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}
                OUTPUT_VARIABLE WINDEPLOYQT_EXE_RESULT
                ERROR_VARIABLE WINDEPLOYQT_EXE_RESULT)

            message(STATUS \"${WINDEPLOYQT_EXE_RESULT}\")

            message(STATUS \"${windeployqt_exe} Done.\")
            ")
    endif()
endif()

if(UNIX AND NOT APPLE)
    set(LINUX_ICON_DIR_BASE "/usr/share/icons/hicolor")

    # Install PNG icons
    set(MY_ICON_SIZES "16;24;32;64;128;256;512")
    foreach(ICON_SIZE ${MY_ICON_SIZES})
        install(FILES ${CMAKE_SOURCE_DIR}/packaging/linux/icons/train_${ICON_SIZE}.png
            DESTINATION ${LINUX_ICON_DIR_BASE}/${ICON_SIZE}x${ICON_SIZE}/apps/
            RENAME ${MR_TIMETABLE_PLANNER_TARGET}.png)

        install(FILES ${CMAKE_SOURCE_DIR}/packaging/linux/icons/train_${ICON_SIZE}.png
            DESTINATION ${LINUX_ICON_DIR_BASE}/${ICON_SIZE}x${ICON_SIZE}/mimetypes/
            RENAME application-x-${MR_TIMETABLE_PLANNER_TARGET}-session-sqlite3.png)
    endforeach()

    # Install SVG icon
    install(FILES ${CMAKE_SOURCE_DIR}/packaging/linux/icons/train.svg
        DESTINATION ${LINUX_ICON_DIR_BASE}/scalable/apps/
        RENAME ${MR_TIMETABLE_PLANNER_TARGET}.svg)

    install(FILES ${CMAKE_SOURCE_DIR}/packaging/linux/icons/train.svg
        DESTINATION ${LINUX_ICON_DIR_BASE}/scalable/mimetypes/
        RENAME application-vnd.${APP_COMPANY_NAME_LOWER}-${MR_TIMETABLE_PLANNER_TARGET}-session-sqlite3.svg)

    # Install desktop file
    install(FILES ${CMAKE_SOURCE_DIR}/packaging/linux/${MR_TIMETABLE_PLANNER_TARGET}.desktop
        DESTINATION /usr/share/applications/)

    # Install mime type association
    #vnd.trainsoftware-mrtplanner.xml
    install(FILES ${CMAKE_SOURCE_DIR}/packaging/linux/mime/vnd.${APP_COMPANY_NAME_LOWER}-${MR_TIMETABLE_PLANNER_TARGET}.xml
        DESTINATION /usr/share/mime/packages)
endif(UNIX AND NOT APPLE)

## Install end ##
