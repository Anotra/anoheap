cmake_minimum_required(VERSION 3.13)

enable_language(C)

include(CheckSymbolExists)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(BUILD_EXAMPLES "build examples" ON)
option(BUILD_TESTS "enable testing" OFF)
option(SHARED "build shared library(off for static library)" ON)

check_symbol_exists(timespec_get time.h HAVE_TIMESPEC_GET)
check_symbol_exists(clock_gettime time.h HAVE_CLOCK_GETTIME)
check_symbol_exists(gettimeofday sys/time.h HAVE_GETTIMEOFDAY)

project(anoheap VERSION 1.0.0 LANGUAGES C)
if (SHARED)
  add_library(${PROJECT_NAME} SHARED)
else()
  add_library(${PROJECT_NAME} STATIC)
endif()
target_sources(${PROJECT_NAME} PRIVATE
  anoheap.c
)
target_include_directories(${PROJECT_NAME} PUBLIC include)
set_target_properties(${PROJECT_NAME} PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON)

if(NOT WIN32)
  target_compile_options(${PROJECT_NAME} PRIVATE
    -pedantic -Wall -Werror -pipe -Wunreachable-code
    -O1
  )

  include(GNUInstallDirs)
  install(DIRECTORY include/ DESTINATION include)
  install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

if (BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if (BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()