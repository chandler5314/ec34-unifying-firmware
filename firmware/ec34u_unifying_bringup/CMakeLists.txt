cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ec34u_unifying_bringup C)

set(EC34U_ROOT ${CMAKE_CURRENT_LIST_DIR}/../..)

target_include_directories(app PRIVATE
    ${EC34U_ROOT}/unifying-prototype/include
)

target_sources(app PRIVATE
    src/main.c
    ${EC34U_ROOT}/unifying-prototype/src/ec34u_unifying.c
    ${EC34U_ROOT}/unifying-prototype/src/ec34u_unifying_crypto.c
    ${EC34U_ROOT}/unifying-prototype/src/ec34u_radio_esb_zephyr.c
)
