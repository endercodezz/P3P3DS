# New Nintendo 3DS build of the P3P3DS runner (.3dsx).
#
# Configure with devkitPro's toolchain file, from the repository root:
#   cmake -S platform/3ds -B build/3ds -G Ninja \
#         -DCMAKE_TOOLCHAIN_FILE=<devkitPro>/cmake/3DS.cmake -DCMAKE_BUILD_TYPE=Release
#   cmake --build build/3ds
# The host tools (psp_recomp) come from the host build (P3P_HOST_BUILD,
# default build/), and the decrypted EBOOT from profiles/p3p/game/eboot.elf;
# both stay local. The EBOOT is embedded in the .3dsx romfs because the AOT
# code is generated from exactly that file: the resulting .3dsx contains game
# code and is for personal testing only, never for distribution.
cmake_minimum_required(VERSION 3.20)
project(p3p3ds_3ds C CXX)

if(NOT NINTENDO_3DS)
    message(FATAL_ERROR "Configure with -DCMAKE_TOOLCHAIN_FILE=<devkitPro>/cmake/3DS.cmake")
endif()

get_filename_component(P3P_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(P3P_HOST_BUILD "${P3P_ROOT}/build" CACHE PATH "Host build directory providing psp_recomp")
set(P3P_ELF "${P3P_ROOT}/profiles/p3p/game/eboot.elf" CACHE FILEPATH "Decrypted ULUS-10512 EBOOT")
set(P3P_AOT_OPT "-Os" CACHE STRING "Optimization flags for the generated AOT units")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_program(P3P_PSP_RECOMP NAMES psp_recomp psp_recomp.exe HINTS "${P3P_HOST_BUILD}/recomp/PSPRecomp" NO_CMAKE_FIND_ROOT_PATH REQUIRED)
include("${P3P_ROOT}/profiles/p3p/config/aot_layout.cmake")
include("${P3P_ROOT}/cmake/p3p3ds_sources.cmake")

# Code size is the constraint on 3DS (docs/3DS_PLATFORM.md section 8.5): compile
# out chain counters and frontier diagnostics, keep cross-unit chains out of
# line. These change Runtime's layout, so they apply to every target.
add_compile_definitions(PSPRECOMP_AOT_PRODUCTION_FASTPATHS=1 PSPRECOMP_NO_FRONTIER_DIAGNOSTICS=1
                        PSPRECOMP_CHAIN_NOINLINE=1)

# ---- AOT generation (host tool), without per-transfer diagnostics ----------
set(GEN_DIR "${CMAKE_BINARY_DIR}/generated/auto")
set(PATCH_LIST "${CMAKE_BINARY_DIR}/generated/patches.txt")
file(MAKE_DIRECTORY "${GEN_DIR}")
set(AOT_SOURCES "${GEN_DIR}/generated_registry.cpp")
math(EXPR _last "${P3P_AOT_UNIT_COUNT} - 1")
foreach(_unit RANGE 0 ${_last})
    string(LENGTH "${_unit}" _len)
    math(EXPR _pad "4 - ${_len}")
    string(REPEAT "0" ${_pad} _zeros)
    list(APPEND AOT_SOURCES "${GEN_DIR}/generated_unit_${_zeros}${_unit}.cpp")
endforeach()
add_custom_command(
    OUTPUT ${AOT_SOURCES} "${GEN_DIR}/generated_units.hpp"
    COMMAND "${Python3_EXECUTABLE}" "${P3P_ROOT}/tools/cwcheat_patches.py" "${P3P_ROOT}/p3p/p3p-patches/ULUS10512.ini"
            "${P3P_ROOT}/profiles/p3p/config/cwcheat_patches.txt" "${PATCH_LIST}"
    COMMAND "${P3P_PSP_RECOMP}" "${P3P_ELF}" --auto "${GEN_DIR}" "${P3P_AOT_LOAD_BASE}" "${P3P_AOT_UNIT_SPAN}" "${PATCH_LIST}" --no-transfer-records
    COMMAND "${Python3_EXECUTABLE}" "${P3P_ROOT}/tools/verify_aot_layout.py" "${GEN_DIR}" "${P3P_AOT_UNIT_COUNT}" "${P3P_AOT_UNIT_SPAN}"
    DEPENDS "${P3P_PSP_RECOMP}" "${P3P_ELF}" "${P3P_ROOT}/profiles/p3p/config/aot_layout.cmake"
            "${P3P_ROOT}/profiles/p3p/config/cwcheat_patches.txt"
    COMMENT "Generating P3P AOT units for 3DS (${P3P_AOT_UNIT_COUNT} x ${P3P_AOT_UNIT_SPAN} bytes)..."
    VERBATIM
)

list(TRANSFORM PSPRECOMP_RUNTIME_SOURCES PREPEND "${P3P_ROOT}/")
list(TRANSFORM P3P3DS_CORE_SOURCES PREPEND "${P3P_ROOT}/")

add_library(psprecomp_runtime STATIC ${PSPRECOMP_RUNTIME_SOURCES})
target_include_directories(psprecomp_runtime PUBLIC "${P3P_ROOT}/recomp/PSPRecomp/include")
# Direct dispatch table covers at most 4 MiB of guest code (the EBOOT image);
# entries registered elsewhere are looked up in the hash map.
target_compile_definitions(psprecomp_runtime PRIVATE PSPRECOMP_DIRECT_WINDOW_MAX=0x400000u)

add_library(p3p_aot STATIC ${AOT_SOURCES})
target_include_directories(p3p_aot PRIVATE "${GEN_DIR}")
target_link_libraries(p3p_aot PUBLIC psprecomp_runtime)
separate_arguments(_aot_opt NATIVE_COMMAND "${P3P_AOT_OPT}")
target_compile_options(p3p_aot PRIVATE ${_aot_opt})

add_library(p3p3ds_core STATIC ${P3P3DS_CORE_SOURCES})
target_include_directories(p3p3ds_core PUBLIC "${P3P_ROOT}/core/include" "${P3P_ROOT}/profiles/p3p/config")
target_link_libraries(p3p3ds_core PUBLIC psprecomp_runtime)

execute_process(COMMAND git -C "${P3P_ROOT}" rev-parse --short HEAD
                OUTPUT_VARIABLE P3P_BUILD_ID OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
add_executable(p3p3ds main.cpp)
target_compile_definitions(p3p3ds PRIVATE P3P3DS_BUILD_ID="${P3P_BUILD_ID}")
target_link_libraries(p3p3ds PRIVATE p3p_aot p3p3ds_core)

# romfs: the decrypted EBOOT the AOT code was generated from.
set(ROMFS_DIR "${CMAKE_BINARY_DIR}/romfs")
file(MAKE_DIRECTORY "${ROMFS_DIR}")
configure_file("${P3P_ELF}" "${ROMFS_DIR}/eboot.elf" COPYONLY)

ctr_generate_smdh(OUTPUT "${CMAKE_BINARY_DIR}/p3p3ds.smdh"
    NAME "P3P3DS"
    DESCRIPTION "Persona 3 Portable recompilation"
    AUTHOR "enderlit aka endercodezz")
ctr_create_3dsx(p3p3ds SMDH "${CMAKE_BINARY_DIR}/p3p3ds.smdh" ROMFS "${ROMFS_DIR}")
