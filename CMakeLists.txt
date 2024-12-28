# cspell:disable
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

file(READ "VERSION.md" ver)
string(REGEX MATCH "MAJ ([0-9]*)" _ ${ver})
set(CX_VER_MAJ ${CMAKE_MATCH_1}) # major
string(REGEX MATCH "MIN ([0-9]*)" _ ${ver})
set(CX_VER_MIN ${CMAKE_MATCH_1}) # minor
string(REGEX MATCH "PAT ([0-9]*)" _ ${ver})
set(CX_VER_PAT ${CMAKE_MATCH_1}) # patch
set(CX_VER_SS ${CX_VER_MAJ}.${CX_VER_MIN}.${CX_VER_PAT})
message("Version: ${CX_VER_SS}")

project(
  astroROOT6
  VERSION ${CX_VER_SS}
  DESCRIPTION "A fork of astro-ROOT"
  LANGUAGES CXX)

set(CMAKE_BUILD_TYPE Release)
if(APPLE)
  set(CMAKE_CXX_STANDARD 20)
  message("cxapple")
  set(CMAKE_BUILD_TYPE Debug)
  set(CMAKE_VERBOSE_MAKEFILE OFF)
  set(CMAKE_MACOSX_RPATH 1)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

  # cmake-lint: disable=C0103,C0307
  if(CMAKE_C_COMPILER_ID MATCHES "Clang\$")
    set(OpenMP_C_FLAGS "-Xclang -fopenmp")
    set(OpenMP_C_LIB_NAMES "omp")
    set(OpenMP_omp_LIBRARY omp)
  endif()

  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang\$")
    set(OpenMP_CXX_FLAGS "-Xclang -fopenmp")
    set(OpenMP_CXX_LIB_NAMES "omp")
    set(OpenMP_omp_LIBRARY omp)
  endif()
else()
  set(CMAKE_CXX_STANDARD 17)
endif()

# test for CPM
file(
  DOWNLOAD
  https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.38.3/CPM.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
  EXPECTED_HASH
    SHA256=cc155ce02e7945e7b8967ddfaff0b050e958a723ef7aad3766d368940cb15494)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

find_package(spdlog REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(yaml-cpp REQUIRED)

list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
# cmake-format: off
find_package(ROOT REQUIRED
  Core Imt RIO Net Hist Graf
  Graf3d Gpad ROOTVecOps Tree
  TreePlayer Rint Postscript Matrix
  Physics MathCore Thread
  MultiProc ROOTDataFrame FITSIO)
# cmake-format: on
if(NOT ROOT_FOUND)
  message(STATUS "ROOT Cern not found")
endif()

find_package(PkgConfig)
pkg_check_modules(Healpix_cxx REQUIRED healpix_cxx>=3.60)

set(CMAKE_INSTALL_PREFIX $ENV{HOME}/software/install/)

include_directories(
  ${PROJECT_SOURCE_DIR}/
  ${ROOT_INCLUDE_DIRS}/
  ${YAML_CPP_INCLUDE_DIR}
  # ${CPR_INCLUDE_DIRS}/
  /opt/homebrew/include
  /usr/include/
  /usr/local/include/
  $ENV{HOME}/software/install/include)
link_directories(/usr/lib/ /usr/local/lib /opt/homebrew/lib
                 $ENV{HOME}/software/install/lib)

add_subdirectory(./tf_container)

file(GLOB sources ${PROJECT_SOURCE_DIR}/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/*.hh)

set(LINK_LIBS
    ${ROOT_LIBRARIES}
    CCfits
    # ${CPR_LIBRARY}} cpr
    spdlog::spdlog
    # ${YAML_CPP_LIBRARIES} # not worked yet
    yaml-cpp)
