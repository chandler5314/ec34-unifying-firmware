add_library(ec34u_unifying
    src/ec34u_unifying.c
    src/ec34u_unifying_crypto.c
    src/ec34u_radio_esb_zephyr.c
)

target_include_directories(ec34u_unifying PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_compile_features(ec34u_unifying PUBLIC c_std_11)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ec34u_unifying PRIVATE -Wall -Wextra -Werror)
endif()

add_executable(ec34u_protocol_smoke
    ../tests/protocol_smoke.c
)

target_link_libraries(ec34u_protocol_smoke PRIVATE ec34u_unifying)
