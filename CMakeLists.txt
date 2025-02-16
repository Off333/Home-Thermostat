# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NEVER EDIT THE NEXT LINES for Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.0.0)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(Thermostat C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/lcd.h ${CMAKE_CURRENT_SOURCE_DIR}/digital_tmp_sensor.h ${CMAKE_CURRENT_SOURCE_DIR}/relay_driver.h ${CMAKE_CURRENT_SOURCE_DIR}/wifi_driver.h ${CMAKE_CURRENT_SOURCE_DIR}/button_driver.h ${CMAKE_CURRENT_SOURCE_DIR}/debug.h ${CMAKE_CURRENT_SOURCE_DIR}/ntp.h ${CMAKE_CURRENT_SOURCE_DIR}/wifi_psswd.h)

set(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/Thermostat.c ${CMAKE_CURRENT_SOURCE_DIR}/lcd.c ${CMAKE_CURRENT_SOURCE_DIR}/digital_tmp_sensor.c ${CMAKE_CURRENT_SOURCE_DIR}/relay_driver.c ${CMAKE_CURRENT_SOURCE_DIR}/wifi_driver.c ${CMAKE_CURRENT_SOURCE_DIR}/button_driver.c ${CMAKE_CURRENT_SOURCE_DIR}/ntp.c)

add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCE})

pico_set_program_name(Thermostat "Thermostat")
pico_set_program_version(Thermostat "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(Thermostat 0)
pico_enable_stdio_usb(Thermostat 1)



# Add the standard library to the build
target_link_libraries(Thermostat
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(Thermostat PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts or any other standard includes, if required
)


# Add any user requested libraries
target_link_libraries(Thermostat 
        hardware_i2c
        hardware_adc
        hardware_rtc
        hardware_sync
        hardware_flash
        pico_sync
        pico_cyw43_arch_lwip_threadsafe_background
        pico_util
        pico_flash
        )


pico_add_extra_outputs(Thermostat)

