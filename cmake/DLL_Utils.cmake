# Small helper for DLL file

function(get_dll_library_from_import_library LIB_VAR OUT_VAR)
    # Locate DLL file
    # Sometimes we link to import libraries '*.dll.a' or '*.lib'
    # When installing we need the real '*.dll' file
    # Try to locate it in same directory or in library path

    get_filename_component(TEMP_EXT ${LIB_VAR} EXT)

    if(${TEMP_EXT} MATCHES ".dll.a" OR ${TEMP_EXT} MATCHES ".lib")
        # Get filename without extension and then add '.dll'
        get_filename_component(TEMP_NAME ${LIB_VAR} NAME_WE)
        set(TEMP_NAME "${TEMP_NAME}.dll")

        # Get file path
        get_filename_component(TEMP_PATH ${LIB_VAR} DIRECTORY)

        # Try new file in same path
        set(TEMP_PATH "${TEMP_PATH}/${TEMP_NAME}")

        if(NOT EXISTS ${TEMP_PATH})
            # Doesn't exist, try to find it in other directory, same name
            # Searche also in CMAKE_LIBRARY_PATH which is not used by default in find_file(...)
            find_file(TEMP_PATH_2 NAMES ${TEMP_NAME} PATHS ${CMAKE_LIBRARY_PATH})
            set(TEMP_PATH ${TEMP_PATH_2})

            # find_file caches the variable but this causes problems
            # with subsequent calls reading old value instead of finding a new file
            unset(TEMP_PATH_2 CACHE)
            unset(TEMP_PATH_2)
        endif()

        unset(TEMP_NAME)

    elseif(${TEMP_EXT} MATCHES ".dll")
        # Library is already a *.dll, use it directly
        set(TEMP_PATH ${LIB_VAR})
    endif()

    set("${OUT_VAR}" ${TEMP_PATH} PARENT_SCOPE)

    unset(TEMP_PATH)
    unset(TEMP_EXT)
endfunction()
