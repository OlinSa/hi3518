if("${live555_PROVIDER}" STREQUAL "module")
{
    if(NOT LIVE555_ROOT_DIR)
        set(LIVE555_ROOT_DIR ${PROJECT_SOURCE_DIR}/package/live555)
    endif()
    if(EXISTS "${LIVE555_ROOT_DIR}/CMakeLists.txt")
        # include_directories()
        add_subdirectory(LIVE555_ROOT_DIR)

        if(TARGET live555lib)
            set(_LIVE555_LIBRARIES live555)
            # set(_LIVE555_INCLUDE_DIR ""
    endif()
}