cmake_minimum_required(VERSION 3.20)
project(P3P3DS VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
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
set(P3P_FUNCTIONS_CSV "${CMAKE_CURRENT_SOURCE_DIR}/profiles/p3p/config/p3p_functions.csv")

add_custom_command(
    OUTPUT "${P3P_GENERATED_CPP}"
    COMMAND psp_recomp "${P3P_ELF}" "${P3P_FUNCTIONS_CSV}" "${P3P_GENERATED_CPP}"
    DEPENDS psp_recomp "${P3P_ELF}" "${P3P_FUNCTIONS_CSV}"
    COMMENT "Generating P3P recompiled translation units..."
    VERBATIM
)

# PC Bootstrap Runner
add_executable(p3p_pc_bootstrap
    platform/pc/main.cpp
    "${P3P_GENERATED_CPP}"
)

target_include_directories(p3p_pc_bootstrap PRIVATE
    recomp/PSPRecomp/include
)

target_link_libraries(p3p_pc_bootstrap PRIVATE
    psprecomp_core
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(p3p_pc_bootstrap PRIVATE -Wall -Wextra)
endif()

# Decoder Compatibility Verification Tool
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

# CTest integration
enable_testing()

add_test(
    NAME p3p_pc_bootstrap_smoke
    COMMAND p3p_pc_bootstrap --verify-milestone
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

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_FOUND)
    add_test(
        NAME p3p_analysis_comparison
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/experiments/p3p-analysis/compare_analysis.py"
    )
    set_tests_properties(p3p_analysis_comparison PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
endif()
