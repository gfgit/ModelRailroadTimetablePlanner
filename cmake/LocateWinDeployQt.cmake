# Locate windeployqt

if (NOT TARGET windeployqt_exe)
    add_executable(windeployqt_exe IMPORTED)

    # Default exe name
    set(WINDEPLOYQT_EXE_NAME "windeployqt.exe")

    if(WINDEPLOYQT_EXE_DIR)
        # If we have explicitly set directory use it
        set(WINDEPLOYQT_EXE_TMP "${WINDEPLOYQT_EXE_DIR}/${WINDEPLOYQT_EXE_NAME}")
    endif()
    if((NOT EXISTS ${WINDEPLOYQT_EXE_TMP}) AND QT_QMAKE_EXECUTABLE)
        # If we have QMake, it should be in same folder
        get_filename_component(WINDEPLOYQT_EXE_DIR ${QT_QMAKE_EXECUTABLE} DIRECTORY)
        set(WINDEPLOYQT_EXE_TMP "${WINDEPLOYQT_EXE_DIR}/${WINDEPLOYQT_EXE_NAME}")
    endif()
    if((NOT EXISTS ${WINDEPLOYQT_EXE_TMP}) AND Qt6_DIR)
        # If we have Qt6_DIR, go up and select 'bin' folder
        get_filename_component(WINDEPLOYQT_EXE_DIR "${Qt6_DIR}/../../../bin" REALPATH)
        set(WINDEPLOYQT_EXE_TMP "${WINDEPLOYQT_EXE_DIR}/${WINDEPLOYQT_EXE_NAME}")
    endif()

    if(EXISTS ${WINDEPLOYQT_EXE_TMP})
        message("Found ${WINDEPLOYQT_EXE_TMP}")
    else()
        message("windeployqt NOT FOUND")
        set(WINDEPLOYQT_EXE_TMP NOTFOUND)
    endif()

    set_target_properties(windeployqt_exe PROPERTIES
        IMPORTED_LOCATION ${WINDEPLOYQT_EXE_TMP}
    )

    unset(WINDEPLOYQT_EXE_TMP)
    unset(WINDEPLOYQT_EXE_DIR)
endif()
