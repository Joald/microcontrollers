cmake_minimum_required(VERSION 3.20)
project(labtest C)


set(CMAKE_SYSTEM_PROCESSOR arm)
set(tools /opt/arm)
set(CMAKE_C_COMPILER ${tools}/bin/arm-eabi-gcc)

set(CMAKE_C_STANDARD 11)

add_executable(labtest main.c)
target_compile_options(labtest PUBLIC "-Wall" "-Wextra" "-Wshadow" "-mcpu=cortex-r4" "-mthumb")