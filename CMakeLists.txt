cmake_minimum_required(VERSION 3.25)

project(AetherQ
  VERSION 0.1.0
  DESCRIPTION "Reproducible, hardware-aware runtime for quantitative workloads"
  HOMEPAGE_URL "https://github.com/aditya-munday/AetherQ"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(AETHERQ_BUILD_TESTS "Build tests" ON)
option(AETHERQ_BUILD_BENCHMARKS "Build benchmarks" ON)
option(AETHERQ_ENABLE_COVERAGE "Enable coverage instrumentation" OFF)
option(AETHERQ_ENABLE_LTO "Enable link time optimization" OFF)
option(AETHERQ_WARNINGS_AS_ERRORS "Treat warnings as errors" ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

find_program(AETHERQ_CCACHE ccache)
if(AETHERQ_CCACHE)
  set(CMAKE_CXX_COMPILER_LAUNCHER "${AETHERQ_CCACHE}")
endif()

find_package(Threads REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)

find_package(Git QUIET)
if(Git_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE AETHERQ_GIT_COMMIT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" status --porcelain
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE AETHERQ_GIT_STATUS
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)
  if(AETHERQ_GIT_STATUS)
    set(AETHERQ_GIT_DIRTY "dirty")
  else()
    set(AETHERQ_GIT_DIRTY "clean")
  endif()
endif()
if(NOT AETHERQ_GIT_COMMIT)
  set(AETHERQ_GIT_COMMIT "unknown")
endif()
if(NOT AETHERQ_GIT_DIRTY)
  set(AETHERQ_GIT_DIRTY "unknown")
endif()

add_library(aetherq_warnings INTERFACE)
target_compile_options(aetherq_warnings INTERFACE
  -Wall
  -Wextra
  -Wpedantic
  -Wconversion
  -Wshadow
  -Wnon-virtual-dtor
  -Wold-style-cast
  -Wcast-align
  -Woverloaded-virtual
  -Wnull-dereference
  -Wdouble-promotion
  -Wformat=2
  -Wundef
  -Wpointer-arith)

if(AETHERQ_WARNINGS_AS_ERRORS)
  target_compile_options(aetherq_warnings INTERFACE -Werror)
endif()

if(AETHERQ_ENABLE_COVERAGE)
  target_compile_options(aetherq_warnings INTERFACE --coverage -O0 -g)
  target_link_options(aetherq_warnings INTERFACE --coverage)
endif()

add_library(aetherq
  src/runtime/version.cpp
  src/runtime/result.cpp
  src/runtime/logging.cpp
  src/runtime/config.cpp
  src/runtime/runtime.cpp
  src/memory/aligned_alloc.cpp
  src/memory/allocation_metrics.cpp
  src/memory/monotonic_arena.cpp
  src/memory/fixed_pool.cpp
  src/memory/pmr_resources.cpp
  src/executor/executor.cpp
  src/executor/serial_executor.cpp
  src/executor/thread_pool_executor.cpp
  src/executor/parallel_for.cpp
  src/kernels/cpu_features.cpp
  src/kernels/scalar_kernels.cpp
  src/kernels/optimized_kernels.cpp
  src/kernels/dispatch.cpp
  src/kernels/rng.cpp
  src/measurement/clock.cpp
  src/measurement/stats.cpp
  src/measurement/environment.cpp
  src/measurement/perf_counters.cpp
  src/measurement/sampler.cpp
  src/measurement/report.cpp
  src/measurement/report_validate.cpp
  src/measurement/report_csv.cpp
  src/measurement/compare.cpp
  src/workloads/monte_carlo.cpp
  src/workloads/black_scholes.cpp
  src/workloads/factor_risk.cpp
  src/workloads/allocation_stress.cpp
  src/workloads/registry.cpp
  src/workloads/workload_support.cpp)

target_include_directories(aetherq
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(aetherq
  PUBLIC
    nlohmann_json::nlohmann_json
  PRIVATE
    $<BUILD_INTERFACE:aetherq_warnings>
    Threads::Threads)

set_target_properties(aetherq PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR})

if(AETHERQ_ENABLE_LTO)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT AETHERQ_IPO_SUPPORTED OUTPUT AETHERQ_IPO_ERROR)
  if(AETHERQ_IPO_SUPPORTED)
    set_property(TARGET aetherq PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  endif()
endif()

add_executable(aetherq_cli
  src/cli/main.cpp
  src/cli/cli_common.cpp
  src/cli/command_system_info.cpp
  src/cli/command_run.cpp
  src/cli/command_bench.cpp
  src/cli/command_compare.cpp
  src/cli/command_report.cpp
  src/cli/command_validate.cpp)

set_target_properties(aetherq_cli PROPERTIES OUTPUT_NAME aetherq)

target_link_libraries(aetherq_cli
  PRIVATE
    aetherq
    CLI11::CLI11
    aetherq_warnings)

target_compile_definitions(aetherq PRIVATE
  AETHERQ_GIT_COMMIT="${AETHERQ_GIT_COMMIT}"
  AETHERQ_GIT_DIRTY="${AETHERQ_GIT_DIRTY}"
  AETHERQ_BUILD_TYPE="${CMAKE_BUILD_TYPE}")

if(AETHERQ_BUILD_TESTS)
  enable_testing()
  find_package(GTest CONFIG REQUIRED)
  add_subdirectory(tests)
endif()

if(AETHERQ_BUILD_BENCHMARKS)
  find_package(benchmark CONFIG REQUIRED)
  find_package(Eigen3 CONFIG REQUIRED)
  add_subdirectory(benchmarks)
endif()

install(TARGETS aetherq aetherq_cli
  EXPORT AetherQTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(DIRECTORY include/aetherq DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT AetherQTargets
  FILE AetherQTargets.cmake
  NAMESPACE AetherQ::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AetherQ)

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/AetherQConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/AetherQConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AetherQ)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/AetherQConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/AetherQConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/AetherQConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AetherQ)
