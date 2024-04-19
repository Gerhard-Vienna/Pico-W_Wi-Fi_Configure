cmake_minimum_required(VERSION 3.13)
set(PICO_BOARD pico_w)

find_package(Perl)
if(NOT PERL_FOUND)
    message(FATAL_ERROR "Perl is needed for generating the fsdata.c file")
endif()


include(pico_sdk_import.cmake)
project(wifi_configure_project C CXX ASM)

set(PROGRAM_NAME wifi_configure)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

add_compile_options(
    -Wall
    -Werror
    -Wno-main
)

# Initial creation of my_fsdata.c from files in wifi_setup/fs
set(MAKE_FS_DATA_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/wifi_setup/external/makefsdata)
message("Running makefsdata script")
execute_process(
    COMMAND perl ${MAKE_FS_DATA_SCRIPT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/wifi_setup
    ECHO_OUTPUT_VARIABLE
    ECHO_ERROR_VARIABLE
)
file(RENAME wifi_setup/fsdata.c wifi_setup/my_fsdata.c)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(${PROGRAM_NAME}
    main.c
    tcp_test_server.c
    wifi_setup/access_point.c
    wifi_setup/dhcp_server.c
    wifi_setup/http_server.c
    wifi_setup/flash_program.c
)

target_include_directories(${PROGRAM_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/.
    ${CMAKE_CURRENT_LIST_DIR}/wifi_setup
)
target_link_libraries(${PROGRAM_NAME}
    pico_cyw43_arch_lwip_threadsafe_background
    pico_lwip_http
    hardware_flash
    pico_stdlib
)

pico_enable_stdio_usb(${PROGRAM_NAME} TRUE)
pico_enable_stdio_uart(${PROGRAM_NAME} TRUE)
suppress_tinyusb_warnings()

pico_add_extra_outputs(${PROGRAM_NAME})

