cmake_minimum_required(VERSION 3.20)
project(P3P3DS VERSION 0.1.0 LANGUAGES CXX)
include(cmake/ccache.cmake)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
endif()

# Runtime configured like the 3DS build (platform/3ds/CMakeLists.txt): no
# frontier diagnostics, events or transfer records. For host profiling of the
# shipped configuration (runner --sample) in a separate build directory; the
# tests that inspect events need the default OFF.
option(P3P_PRODUCTION_RUNTIME "Host runtime without diagnostics, as on 3DS" OFF)
if(P3P_PRODUCTION_RUNTIME)
    add_compile_definitions(PSPRECOMP_AOT_PRODUCTION_FASTPATHS=1 PSPRECOMP_NO_FRONTIER_DIAGNOSTICS=1
                            PSPRECOMP_CHAIN_NOINLINE=1)
    set(P3P_AOT_TRANSFER_RECORDS OFF CACHE BOOL "" FORCE)
    set(PSPRECOMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
endif()

# Configure PSPRecomp
option(PSPRECOMP_BUILD_TESTS "Build PSPRecomp upstream tests" ON)
option(PSPRECOMP_BUILD_PROFILE_TESTS "Build PSPRecomp profile tests" OFF)
add_subdirectory(recomp/PSPRecomp)

# Target directory for generated AOT files
set(P3P_GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${P3P_GENERATED_DIR}")
set(P3P_GENERATED_CPP "${P3P_GENERATED_DIR}/p3p_generated.cpp")

# Generation command using psp_recomp
set(P3P_ELF "${CMAKE_CURRENT_SOURCE_DIR}/profiles/p3p/game/eboot.elf")
# The decrypted EBOOT is local (tools/prepare_game.py). Without it (CI, a fresh
# clone) everything made from the game is left out: the AOT units, the PC
# runner and the tests that execute game code. The runtime, the tools, the
# P3P3DS Builder and the other tests still build.
if(EXISTS "${P3P_ELF}")
    set(P3P_HAVE_GAME ON)
else()
    set(P3P_HAVE_GAME OFF)
    message(STATUS "No ${P3P_ELF}: building without the game-derived targets")
endif()
set(P3P_FUNCTIONS_CSV "${CMAKE_CURRENT_SOURCE_DIR}/profiles/p3p/config/p3p_functions.csv")
find_package(Python3 COMPONENTS Interpreter REQUIRED)
set(P3P_FRONTIER_CSV "${CMAKE_CURRENT_SOURCE_DIR}/profiles/p3p/config/frontier_seeds.csv" CACHE FILEPATH
    "Managed frontier seed manifest merged into MANUAL-mode generation")
set(P3P_MERGED_CSV "${P3P_GENERATED_DIR}/frontier_functions.csv")

# AUTO: whole-.text generation from psp_analyze's global CFG, split into
# fixed-span translation units (profiles/p3p/config/aot_layout.cmake).
# MANUAL: legacy seed-manifest generation used by tools/chase_frontier.py.
set(P3P_AOT_MODE "AUTO" CACHE STRING "AOT generation mode: AUTO or MANUAL")
set_property(CACHE P3P_AOT_MODE PROPERTY STRINGS AUTO MANUAL)
set(P3P_AOT_OPT "-O2" CACHE STRING "Optimization flag for generated AOT units (GCC/Clang)")
include("${CMAKE_CURRENT_SOURCE_DIR}/profiles/p3p/config/aot_layout.cmake")

if(NOT P3P_HAVE_GAME)
    # No generated code without the EBOOT.
elseif(P3P_AOT_MODE STREQUAL "AUTO")
    set(P3P_AUTO_DIR "${P3P_GENERATED_DIR}/auto")
    # OFF drops the per-transfer rt.record_transfer diagnostics from the
    # generated code (frontier tooling then has no transfer trace).
    option(P3P_AOT_TRANSFER_RECORDS "Emit rt.record_transfer in generated AOT units" ON)
    set(P3P_RECOMP_FLAGS "")
    if(NOT P3P_AOT_TRANSFER_RECORDS)
        set(P3P_RECOMP_FLAGS "--no-transfer-records")
    endif()
    # Community CWCheat codes applied to the EBOOT before generation and load.
    set(P3P_CWCHEAT_INI "${CMAKE_CURRENT_SOURCE_DIR}/p3p/p3p-patches/ULUS10512.ini")
    set(P3P_CWCHEAT_SELECTION "${CMAKE_CURRENT_SOURCE_DIR}/profiles/p3p/config/cwcheat_patches.txt")
    set(P3P_PATCH_LIST "${P3P_GENERATED_DIR}/patches.txt")
    set(P3P_AUTO_SOURCES "${P3P_AUTO_DIR}/generated_registry.cpp")
    math(EXPR _p3p_last_unit "${P3P_AOT_UNIT_COUNT} - 1")
    foreach(_unit RANGE 0 ${_p3p_last_unit})
        string(LENGTH "${_unit}" _len)
        math(EXPR _pad "4 - ${_len}")
        string(REPEAT "0" ${_pad} _zeros)
        list(APPEND P3P_AUTO_SOURCES "${P3P_AUTO_DIR}/generated_unit_${_zeros}${_unit}.cpp")
    endforeach()
    add_custom_command(
        OUTPUT ${P3P_AUTO_SOURCES} "${P3P_AUTO_DIR}/generated_units.hpp"
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/cwcheat_patches.py" "${P3P_CWCHEAT_INI}" "${P3P_CWCHEAT_SELECTION}" "${P3P_PATCH_LIST}"
        COMMAND psp_recomp "${P3P_ELF}" --auto "${P3P_AUTO_DIR}" "${P3P_AOT_LOAD_BASE}" "${P3P_AOT_UNIT_SPAN}" "${P3P_PATCH_LIST}" ${P3P_RECOMP_FLAGS}
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_aot_layout.py" "${P3P_AUTO_DIR}" "${P3P_AOT_UNIT_COUNT}" "${P3P_AOT_UNIT_SPAN}"
        DEPENDS psp_recomp "${P3P_ELF}" "${CMAKE_CURRENT_SOURCE_DIR}/profiles/p3p/config/aot_layout.cmake"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_aot_layout.py" "${CMAKE_CURRENT_SOURCE_DIR}/tools/cwcheat_patches.py"
                "${P3P_CWCHEAT_INI}" "${P3P_CWCHEAT_SELECTION}"
        COMMENT "Generating whole-.text P3P AOT units (${P3P_AOT_UNIT_COUNT} x ${P3P_AOT_UNIT_SPAN} bytes)..."
        VERBATIM
    )
    add_library(p3p_aot STATIC ${P3P_AUTO_SOURCES})
    target_include_directories(p3p_aot PRIVATE recomp/PSPRecomp/include "${P3P_AUTO_DIR}")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        separate_arguments(_p3p_aot_opt NATIVE_COMMAND "${P3P_AOT_OPT}")
        target_compile_options(p3p_aot PRIVATE ${_p3p_aot_opt})
    endif()
    target_link_libraries(p3p_aot PUBLIC psprecomp_core)
    set(P3P_AOT_LINK p3p_aot)
    set(P3P_AOT_RUNNER_SOURCES)
else()
    add_custom_command(
        OUTPUT "${P3P_GENERATED_CPP}"
        BYPRODUCTS "${P3P_MERGED_CSV}"
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/chase_frontier.py" --merge "${P3P_FUNCTIONS_CSV}" "${P3P_FRONTIER_CSV}" "${P3P_MERGED_CSV}"
        COMMAND psp_recomp "${P3P_ELF}" "${P3P_MERGED_CSV}" "${P3P_GENERATED_CPP}"
        DEPENDS psp_recomp "${P3P_ELF}" "${P3P_FUNCTIONS_CSV}" "${P3P_FRONTIER_CSV}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/chase_frontier.py"
        COMMENT "Generating P3P recompiled translation units..."
        VERBATIM
    )
    set(P3P_AOT_LINK)
    set(P3P_AOT_RUNNER_SOURCES "${P3P_GENERATED_CPP}")
endif()

# Core P3P3DS Target-Agnostic Runtime & HLE Library
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/p3p3ds_sources.cmake")
add_library(p3p3ds_core ${P3P3DS_CORE_SOURCES})

target_include_directories(p3p3ds_core PUBLIC
    core/include
    recomp/PSPRecomp/include
)

target_link_libraries(p3p3ds_core PUBLIC
    psprecomp_core
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(p3p3ds_core PRIVATE -Wall -Wextra)
endif()

# PC Bootstrap Runner (linked with the game's generated code)
if(P3P_HAVE_GAME)
add_executable(p3p_pc_bootstrap
    platform/pc/main.cpp
    ${P3P_AOT_RUNNER_SOURCES}
)

target_include_directories(p3p_pc_bootstrap PRIVATE
    core/include
    recomp/PSPRecomp/include
)

target_link_libraries(p3p_pc_bootstrap PRIVATE
    ${P3P_AOT_LINK}
    p3p3ds_core
    psprecomp_core
)
if(WIN32)
    target_link_libraries(p3p_pc_bootstrap PRIVATE winmm psapi) # --sample: 1 ms timer, module list
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(p3p_pc_bootstrap PRIVATE -Wall -Wextra)
endif()
endif()

# Decoder Compatibility Verification Tool
add_executable(mnemonic_histogram experiments/p3p-analysis/mnemonic_histogram.cpp)
target_include_directories(mnemonic_histogram PRIVATE recomp/PSPRecomp/include)
target_link_libraries(mnemonic_histogram PRIVATE psprecomp_core)

add_executable(verify_p3p_decoder
    experiments/p3p-analysis/verify_decoder.cpp
)

target_include_directories(verify_p3p_decoder PRIVATE
    recomp/PSPRecomp/include
)

target_link_libraries(verify_p3p_decoder PRIVATE
    psprecomp_core
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(verify_p3p_decoder PRIVATE -Wall -Wextra)
endif()

# Focused SysMem HLE Unit Test
add_executable(test_hle_sysmem
    tests/test_hle_sysmem.cpp
)

target_include_directories(test_hle_sysmem PRIVATE
    core/include
    recomp/PSPRecomp/include
)

target_link_libraries(test_hle_sysmem PRIVATE
    p3p3ds_core
    psprecomp_core
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(test_hle_sysmem PRIVATE -Wall -Wextra)
endif()

# Focused ThreadMan HLE Unit Test
add_executable(test_hle_threadman
    tests/test_hle_threadman.cpp
)

target_include_directories(test_hle_threadman PRIVATE
    core/include
    recomp/PSPRecomp/include
)

target_link_libraries(test_hle_threadman PRIVATE
    p3p3ds_core
    psprecomp_core
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(test_hle_threadman PRIVATE -Wall -Wextra)
endif()

# CTest integration
enable_testing()

add_test(
    NAME p3p_hle_sysmem
    COMMAND test_hle_sysmem
)
set_tests_properties(p3p_hle_sysmem PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_test(
    NAME p3p_hle_threadman
    COMMAND test_hle_threadman
)
set_tests_properties(p3p_hle_threadman PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

if(P3P_HAVE_GAME)
add_test(
    NAME p3p_pc_bootstrap_smoke
    COMMAND p3p_pc_bootstrap --verify-bootstrap --max-dispatches 100000
)
set_tests_properties(p3p_pc_bootstrap_smoke PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_test(
    NAME p3p_decoder_compatibility
    COMMAND verify_p3p_decoder "${P3P_ELF}"
)
set_tests_properties(p3p_decoder_compatibility PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
endif()

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_FOUND)
    add_test(
        NAME p3p_analysis_comparison
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/experiments/p3p-analysis/compare_analysis.py"
    )
    set_tests_properties(p3p_analysis_comparison PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )

    add_test(
        NAME p3p_prepare_game_tests
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_prepare_game.py"
    )
    set_tests_properties(p3p_prepare_game_tests PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
endif()

# Focused dispatch, resumable setup execution and zero-valued VRAM write accounting.
add_executable(test_hle_graphics tests/test_hle_graphics.cpp)
add_executable(test_hle_umd tests/test_hle_umd.cpp)
add_executable(test_hle_audio tests/test_hle_audio.cpp)
target_link_libraries(test_hle_audio PRIVATE p3p3ds_core)
add_test(NAME p3p_hle_audio COMMAND test_hle_audio)
target_link_libraries(test_hle_umd PRIVATE p3p3ds_core)
add_test(NAME p3p_hle_umd COMMAND test_hle_umd)
target_link_libraries(test_hle_graphics PRIVATE p3p3ds_core)
add_test(NAME p3p_hle_graphics COMMAND test_hle_graphics)

set_tests_properties(p3p_hle_graphics PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
add_executable(p3p_frontier_check platform/pc/frontier_check.cpp)
target_link_libraries(p3p_frontier_check PRIVATE p3p3ds_core)
add_executable(test_frontier tests/test_frontier.cpp)
target_link_libraries(test_frontier PRIVATE p3p3ds_core)
add_test(NAME p3p_frontier_safety COMMAND test_frontier)
add_test(NAME p3p_frontier_workflow COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_chase_frontier.py")

# Interpreter vs AOT differential test (needs the local ELF).
if(P3P_HAVE_GAME AND P3P_AOT_MODE STREQUAL "AUTO")
    add_executable(test_interpreter_diff tests/test_interpreter_diff.cpp)
    target_link_libraries(test_interpreter_diff PRIVATE p3p_aot p3p3ds_core)
    add_test(NAME p3p_interpreter_differential COMMAND test_interpreter_diff "${P3P_ELF}")
    set_tests_properties(p3p_interpreter_differential PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
endif()

add_executable(test_threadman_sync tests/test_threadman_sync.cpp)
target_link_libraries(test_threadman_sync PRIVATE p3p3ds_core)
add_test(NAME p3p_threadman_sync COMMAND test_threadman_sync)

add_executable(test_iofilemgr tests/test_iofilemgr.cpp)
target_link_libraries(test_iofilemgr PRIVATE p3p3ds_core)
add_test(NAME p3p_iofilemgr COMMAND test_iofilemgr)
set_tests_properties(p3p_iofilemgr PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(test_modulemgr tests/test_modulemgr.cpp)
target_link_libraries(test_modulemgr PRIVATE p3p3ds_core)
add_test(NAME p3p_modulemgr COMMAND test_modulemgr)
set_tests_properties(p3p_modulemgr PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)

add_executable(test_sascore tests/test_sascore.cpp)
target_link_libraries(test_sascore PRIVATE p3p3ds_core)
add_test(NAME p3p_sascore COMMAND test_sascore)
add_executable(test_ge_renderer tests/test_ge_renderer.cpp)
target_link_libraries(test_ge_renderer PRIVATE p3p3ds_core)
add_test(NAME p3p_ge_renderer COMMAND test_ge_renderer)
# Builder (ISO -> .3dsx) pieces that run on the host and are unit-tested here.
add_library(p3p3ds_builder_core STATIC platform/pc/builder/psp_eboot.cpp)
target_include_directories(p3p3ds_builder_core PUBLIC platform/pc/builder)
if(WIN32)
    enable_language(RC)
    add_executable(p3p3ds_builder WIN32 platform/pc/builder/builder_main.cpp platform/pc/builder/pipeline.cpp platform/pc/builder/builder.rc)
    target_link_libraries(p3p3ds_builder PRIVATE p3p3ds_builder_core p3p3ds_core comctl32 comdlg32 shell32 ole32)
    target_compile_definitions(p3p3ds_builder PRIVATE UNICODE _UNICODE)
    if(MINGW)
        target_link_options(p3p3ds_builder PRIVATE -municode -static)
    endif()
else()
    # Console builder for Linux (statically linked like every host target above).
    add_executable(p3p3ds_builder platform/pc/builder/builder_main.cpp platform/pc/builder/pipeline.cpp)
    target_link_libraries(p3p3ds_builder PRIVATE p3p3ds_builder_core p3p3ds_core)
endif()
set_target_properties(p3p3ds_builder PROPERTIES OUTPUT_NAME "P3P3DS-Builder")
add_executable(test_psp_eboot tests/test_psp_eboot.cpp)
target_link_libraries(test_psp_eboot PRIVATE p3p3ds_builder_core p3p3ds_core)
file(GLOB P3P_LOCAL_ISOS "${CMAKE_CURRENT_SOURCE_DIR}/*.iso")
list(LENGTH P3P_LOCAL_ISOS _iso_count)
if(_iso_count GREATER 0)
    list(GET P3P_LOCAL_ISOS 0 P3P_LOCAL_ISO)
    add_test(NAME p3p_psp_eboot COMMAND test_psp_eboot "${P3P_LOCAL_ISO}")
else()
    add_test(NAME p3p_psp_eboot COMMAND test_psp_eboot)
endif()
add_executable(test_savedata tests/test_savedata.cpp)
target_link_libraries(test_savedata PRIVATE p3p3ds_core)
add_test(NAME p3p_savedata COMMAND test_savedata)
add_executable(test_ge_geometry tests/test_ge_geometry.cpp)
target_link_libraries(test_ge_geometry PRIVATE p3p3ds_core)
add_test(NAME p3p_ge_geometry COMMAND test_ge_geometry)
add_executable(test_mpeg tests/test_mpeg.cpp)
target_link_libraries(test_mpeg PRIVATE p3p3ds_core)
target_compile_definitions(test_mpeg PRIVATE P3P_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME p3p_mpeg COMMAND test_mpeg)
set_tests_properties(p3p_mpeg PROPERTIES SKIP_RETURN_CODE 77)
add_executable(test_misc_hle tests/test_misc_hle.cpp)
target_link_libraries(test_misc_hle PRIVATE p3p3ds_core)
add_test(NAME p3p_misc_hle COMMAND test_misc_hle)
add_executable(test_pcm_mixer tests/test_pcm_mixer.cpp)
add_test(NAME p3p_pcm_mixer COMMAND test_pcm_mixer)
add_executable(test_input tests/test_input.cpp)
target_link_libraries(test_input PRIVATE p3p3ds_core)
add_test(NAME p3p_input COMMAND test_input)
add_executable(p3p_autotest platform/pc/autotest.cpp)
target_link_libraries(p3p_autotest PRIVATE p3p3ds_core)
add_test(NAME p3p_autotest_baseline COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_autotests.py" --exe $<TARGET_FILE:p3p_autotest>)
set_tests_properties(p3p_autotest_baseline PROPERTIES SKIP_RETURN_CODE 77) # without references/pspautotests
add_test(NAME p3p_skills_mirror COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/sync_agent_skills.py" --check)
set_tests_properties(p3p_sascore PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
