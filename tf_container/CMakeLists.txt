# cspell:disable
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

project(cx_tf_container)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(APPLE)
  set(CMAKE_C_COMPILER clang)
  set(CMAKE_MACOSX_RPATH 1)
  set(CMAKE_CXX_COMPILER clang++)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED True)
endif()

find_package(PkgConfig)
pkg_check_modules(Erfa REQUIRED erfa>=1.7)
pkg_check_modules(Healpix_tx REQUIRED healpix_cxx>=3.60)
pkg_check_modules(Wcslib REQUIRED wcslib>=7.1)

find_package(
  ROOT REQUIRED
  COMPONENTS Core
             RIO
             Net
             Hist
             Graf
             Graf3d
             Gpad
             Tree
             Rint
             Postscript
             Matrix
             Physics
             MathCore
             Thread
             dl
             Minuit
             TreePlayer
             FITSIO)
if(NOT ROOT_FOUND)
  message(STATUS "ROOT cern not found ont this COMPUTER")
endif()
include(${ROOT_USE_FILE})
add_definitions(${ROOT_CXX_FLAGS})

set(CMAKE_INSTALL_PREFIX $ENV{HOME}/software/install3/)

include_directories(${PROJECT_SOURCE_DIR}/ ${ROOT_INCLUDE_DIRS}/ /usr/include/
                    /usr/local/include/ /opt/homebrew/include/)
link_directories(/usr/lib/ /usr/local/lib/ /opt/homebrew/lib/)

# TODO:
root_generate_dictionary(
  TFdict
  TFTable.h
  TFGroup.h
  TFRowIterator.h
  TFNameConvert.h
  TFError.h
  TFHeader.h
  TFIOElement.h
  TFRootIO.h
  TFFitsIO.h
  TFAsroIO.h
  TFAsroFile.h
  LINKDEF
  TFLinkDef.h)
root_generate_dictionary(TFcoldict TFColumn.h TFGroup.h LINKDEF TFColLinkDef.h)
root_generate_dictionary(TFimgdict TFImage.h LINKDEF TFImgLinkDef.h)
root_generate_dictionary(TFstrdict LINKDEF TFTStringLinkDef.h)

file(GLOB sources ${PROJECT_SOURCE_DIR}/*.cxx)
file(GLOB headers ${PROJECT_SOURCE_DIR}/*.h)

set(LINK_LIBS ${ROOT_LIBRARIES} wcs CCfits cfitsio)

add_library(cx_tf_container SHARED TFdict.cxx TFcoldict.cxx TFimgdict.cxx
                                   TFstrdict.cxx ${sources} ${headers})
target_link_libraries(cx_tf_container ${LINK_LIBS})
