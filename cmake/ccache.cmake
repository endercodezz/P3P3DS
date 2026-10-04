# Compile through ccache when it is installed (P3P_USE_CCACHE=OFF disables it).
# Helps rebuilds after a clean, a reconfigure or a checkout of earlier sources;
# a change to a header every generated unit includes still recompiles them all.
# Cache location and size come from ccache's own settings (CCACHE_DIR, ...);
# shell.nix points CCACHE_DIR at .cache/ccache inside the workspace.
option(P3P_USE_CCACHE "Compile through ccache when it is installed" ON)
if(P3P_USE_CCACHE AND NOT CMAKE_CXX_COMPILER_LAUNCHER)
    find_program(P3P_CCACHE ccache NO_CMAKE_FIND_ROOT_PATH)
    if(P3P_CCACHE)
        set(CMAKE_C_COMPILER_LAUNCHER "${P3P_CCACHE}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${P3P_CCACHE}")
        message(STATUS "Compiling through ccache: ${P3P_CCACHE}")
    endif()
endif()
