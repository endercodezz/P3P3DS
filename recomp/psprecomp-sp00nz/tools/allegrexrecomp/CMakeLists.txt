# allegrexrecomp — the recompiler CLI.
#
# Split into a library plus a thin main() so the tests can link the decoder,
# the container parsers and the crypto directly without shelling out to the
# executable.

add_library(allegrex_core STATIC
    decode.c
    container.c
    analyze.c
    emit.c
    keys.c
    crypto/aes.c
    crypto/cmac.c
    crypto/sha1.c
    crypto/kirk.c
)

target_include_directories(allegrex_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(allegrexrecomp
    main.c
)

target_link_libraries(allegrexrecomp PRIVATE allegrex_core psprecomp)
