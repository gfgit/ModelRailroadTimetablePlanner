
#Set Win32 resources
if (WIN32)
    configure_file(../packaging/windows/resources.rc.in ${CMAKE_BINARY_DIR}/resources/resources.rc)
    set(TRAINTIMETABLE_RESOURCES
        ${TRAINTIMETABLE_RESOURCES}
        ${CMAKE_BINARY_DIR}/resources/resources.rc
        )
endif()

add_subdirectory(app)
add_subdirectory(backgroundmanager)
add_subdirectory(db_metadata)
add_subdirectory(graph)
add_subdirectory(jobs)
add_subdirectory(lines)
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

# Set TrainTimetable info template file
set(TRAINTIMETABLE_SOURCES
    ${TRAINTIMETABLE_SOURCES}
    app/info.h.in
    )
configure_file(app/info.h.in ${CMAKE_BINARY_DIR}/include/info.h)

# Add executable
add_executable(${TRAINTIMETABLE_TARGET} WIN32
    ${TRAINTIMETABLE_SOURCES}
    ${TRAINTIMETABLE_UI_FILES}
    ${TRAINTIMETABLE_RESOURCES}
    )

# Set compiler options
if(MSVC)
    target_compile_options(
        ${TRAINTIMETABLE_TARGET}
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
        ${TRAINTIMETABLE_TARGET}
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
        ${TRAINTIMETABLE_TARGET}
        PRIVATE
        -rdynamic
        )
endif()

# Set include directories
target_include_directories(
    ${TRAINTIMETABLE_TARGET}
    PRIVATE
    ${SQLite3_INCLUDE_DIRS}
    ${ZIP_INCLUDE_DIR}
    ${CMAKE_BINARY_DIR}/include #For template files
    )

# Set link libraries
target_link_libraries(
    ${TRAINTIMETABLE_TARGET}
    PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Svg
    Qt5::PrintSupport
    ${SQLite3_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${ZIP_LIBRARY}
    )

if (WIN32)
    target_link_libraries(
        ${TRAINTIMETABLE_TARGET}
        PRIVATE
        DbgHelp
        )
endif()

if(UNIX AND NOT APPLE)
    install(TARGETS ${TRAINTIMETABLE_TARGET} RUNTIME DESTINATION bin)
endif()

## Enable Crashpad if found
#if (GoogleCrashpad_FOUND)
#  set(OLIVE_DEFINITIONS ${OLIVE_DEFINITIONS} USE_CRASHPAD)

#  target_include_directories(
#    ${OLIVE_TARGET}
#    PRIVATE
#    ${CRASHPAD_INCLUDE_DIRS}
#  )

#  target_link_libraries(
#    ${OLIVE_TARGET}
#    PRIVATE
#    ${CRASHPAD_LIBRARIES}
#  )

#  set(OLIVE_CRASH_TARGET "olive-crashhandler")

#  set(OLIVE_CRASH_SOURCES
#    dialog/crashhandler/crashhandler.h
#    dialog/crashhandler/crashhandler.cpp
#    dialog/crashhandler/crashhandlermain.cpp
#  )

#  if (WIN32)
#    add_executable(
#      ${OLIVE_CRASH_TARGET}
#      WIN32
#      ${OLIVE_CRASH_SOURCES}
#    )
#  else()
#    add_executable(
#      ${OLIVE_CRASH_TARGET}
#      ${OLIVE_CRASH_SOURCES}
#    )
#  endif()

#  target_include_directories(
#    ${OLIVE_CRASH_TARGET}
#    PRIVATE
#    ${CRASHPAD_INCLUDE_DIRS}
#    )

#  target_link_libraries(
#    ${OLIVE_CRASH_TARGET}
#    PRIVATE
#    Qt5::Core
#    Qt5::Gui
#    Qt5::Widgets
#    Qt5::Network
#    ${CRASHPAD_LIBRARIES}
#  )

#  set(CRASHPAD_HANDLER "crashpad_handler${CMAKE_EXECUTABLE_SUFFIX}")
#  set(MINIDUMP_STACKWALK "minidump_stackwalk${CMAKE_EXECUTABLE_SUFFIX}")

#  if(UNIX AND NOT APPLE)
#    install(TARGETS ${OLIVE_CRASH_TARGET} RUNTIME DESTINATION bin)
#    install(PROGRAMS ${CRASHPAD_LIBRARY_DIRS}/${CRASHPAD_HANDLER} DESTINATION bin)
#    install(PROGRAMS ${BREAKPAD_BIN_DIR}/${MINIDUMP_STACKWALK} DESTINATION bin)
#  endif()

#  if(APPLE)
#    # Move crash handler executables inside Mac app bundle
#    add_custom_command(TARGET ${OLIVE_CRASH_TARGET} POST_BUILD
#      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${OLIVE_CRASH_TARGET} $<TARGET_FILE_DIR:${OLIVE_TARGET}>
#      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CRASHPAD_LIBRARY_DIRS}/${CRASHPAD_HANDLER} $<TARGET_FILE_DIR:${OLIVE_TARGET}>
#      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BREAKPAD_BIN_DIR}/${MINIDUMP_STACKWALK} $<TARGET_FILE_DIR:${OLIVE_TARGET}>
#    )
#  endif()
#endif()

# Set compiler definitions
target_compile_definitions(${TRAINTIMETABLE_TARGET} PRIVATE ${TRAINTIMETABLE_DEFINITIONS})

## Doxygen documentation ##
if(DOXYGEN_FOUND)
    set(DOXYGEN_PROJECT_NAME ${APP_DISPLAY_NAME})
    set(DOXYGEN_PROJECT_LOGO ${APP_ICON})
    set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/docs")
    set(DOXYGEN_EXTRACT_ALL "YES")
    set(DOXYGEN_EXTRACT_PRIVATE "YES")
    set(DOXYGEN_DOT_GRAPH_MAX_NODES 100)
    doxygen_add_docs(docs ALL ${TRAINTIMETABLE_SOURCES})
endif()
## Doxygen end ##

## Update/Release translations ##

#(Run this target before installing installing and every time you update translations)
add_custom_target(RELEASE_TRANSLATIONS ALL
    COMMENT "Running translations it_IT...")

if(UPDATE_TS)
    # Run 'lupdate' to parse C++ and UI files and extract translatable strings
    if(UPDATE_TS_KEEP_OBSOLETE)
        add_custom_command(TARGET RELEASE_TRANSLATIONS
            POST_BUILD
            COMMAND ${Qt5_LUPDATE_EXECUTABLE} ARGS ${CMAKE_SOURCE_DIR}/src -ts ${TRAINTIMETABLE_TS_FILES}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src
            COMMENT "Updating translations"
            VERBATIM)
    else()
        add_custom_command(TARGET RELEASE_TRANSLATIONS
            POST_BUILD
            COMMAND ${Qt5_LUPDATE_EXECUTABLE} ARGS ${CMAKE_SOURCE_DIR}/src -ts ${TRAINTIMETABLE_TS_FILES} -no-obsolete
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src
            COMMENT "Updating translations"
            VERBATIM)
    endif()

endif()

# For each .ts file release a .qm file
foreach(TS_FILE ${TRAINTIMETABLE_TS_FILES})
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
install(TARGETS ${TRAINTIMETABLE_TARGET}
    RUNTIME
    DESTINATION ${CMAKE_INSTALL_PREFIX})

# Copy SVG icon
install(FILES ${CMAKE_SOURCE_DIR}/files/icons/lightning/lightning.svg
    DESTINATION ${CMAKE_INSTALL_PREFIX}/icons)

# For each .ts file install corrensponding .qm file
foreach(TS_FILE ${TRAINTIMETABLE_TS_FILES})
    get_filename_component(QM_FILE_NAME ${TS_FILE} NAME_WLE)
    install(FILES
        "${CMAKE_BINARY_DIR}/src/translations/${QM_FILE_NAME}.qm"
        DESTINATION ${CMAKE_INSTALL_PREFIX}/translations OPTIONAL)
endforeach()

if(WIN32)
    # Copy SQlite3 DLL
    install(PROGRAMS ${SQLite3_LIBRARIES} DESTINATION ${CMAKE_INSTALL_PREFIX})

    # Copy ZLib DLL
    install(PROGRAMS ${ZLIB_LIBRARIES} DESTINATION ${CMAKE_INSTALL_PREFIX})

    # Copy libzip DLL
    install(PROGRAMS ${ZIP_LIBRARY} DESTINATION ${CMAKE_INSTALL_PREFIX})

    if(RUN_WINDEPLOYQT)
        if(NOT WINDEPLOYQT_EXE)
            message(FATAL_ERROR "In order to run windeployqt you must first set th exe path in WINDEPLOYQT_EXE")
        endif()

        install(CODE "
            message(STATUS \"Running windeployqt ${WINDEPLOYQT_EXE}\")
            execute_process(COMMAND ${WINDEPLOYQT_EXE} ${CMAKE_INSTALL_PREFIX}
                WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}
                OUTPUT_VARIABLE WINDEPLOYQT_EXE_RESULT
                ERROR_VARIABLE WINDEPLOYQT_EXE_RESULT)

            message(STATUS \${WINDEPLOYQT_EXE_RESULT})

            message(STATUS \"windeployqt Done.\")
            ")
    endif()
endif()

## Install end ##
