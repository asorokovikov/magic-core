set(USE_LLD ON)

if (USE_LLD)
    set(USE_LD "lld-14" CACHE STRING "Linker to use e.g. gold, lld")
    #    set(USE_LD "${USE_LD}" CACHE STRING "Linker to use e.g. gold, lld")
else()
    set(USE_LD "gold" CACHE STRING "Linker to use e.g. gold, lld")
endif()

set(CUSTOM_LINKER ${USE_LD})

if (CUSTOM_LINKER)
    execute_process(COMMAND ${CMAKE_C_COMPILER} -fuse-ld=${CUSTOM_LINKER} -Wl,--version
            ERROR_QUIET OUTPUT_VARIABLE LD_VERSION)

    if (("${CUSTOM_LINKER}" MATCHES "gold") AND ("${LD_VERSION}" MATCHES "GNU gold"))
        set(CUSTOM_LD_OK ON CACHE INTERNAL CUSTOM_LD_OK)
        message(STATUS "Using GNU gold linker")
    elseif (("${CUSTOM_LINKER}" MATCHES "lld") AND ("${LD_VERSION}" MATCHES "LLD"))
        set(CUSTOM_LD_OK ON CACHE INTERNAL CUSTOM_LD_OK)
        message(STATUS "Using LLVM lld linker")
    endif ()

    if (CUSTOM_LD_OK)
        string(APPEND CMAKE_EXE_LINKER_FLAGS " -fuse-ld=${CUSTOM_LINKER}")
        string(APPEND CMAKE_SHARED_LINKER_FLAGS " -fuse-ld=${CUSTOM_LINKER}")
    else ()
        message(STATUS "Custom linker isn't available, using the default system linker.")
    endif ()
endif ()
