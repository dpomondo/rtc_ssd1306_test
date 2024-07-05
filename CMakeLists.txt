# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH "/home/dpoyesac/Projects/pico/pico-sdk")

set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(rtc_ssd1306_test C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(rtc_ssd1306_test "")
add_subdirectory(src)
# add_subdirectory(include)
add_subdirectory(libraries/pico-ssd1306)
add_subdirectory(libraries/ntp_request)
add_subdirectory(libraries/bmp280_i2c)
add_subdirectory(libraries/eeprom)

pico_set_program_name(rtc_ssd1306_test "rtc_ssd1306_test")
pico_set_program_version(rtc_ssd1306_test "0.1")

pico_enable_stdio_uart(rtc_ssd1306_test 1)
pico_enable_stdio_usb(rtc_ssd1306_test 0)

# Add the standard library to the build
target_link_libraries(rtc_ssd1306_test
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(rtc_ssd1306_test PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/include
  ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts or any other standard includes, if required
)

# Add any user requested libraries
target_link_libraries(rtc_ssd1306_test 
        pico_time
        pico_cyw43_arch_lwip_threadsafe_background
        hardware_rtc
        hardware_i2c
        hardware_spi
        hardware_timer
        bmp280_i2c        
        ntp_request
        pico-ssd1306
        eeprom_25XX  
        )

pico_add_extra_outputs(rtc_ssd1306_test)

