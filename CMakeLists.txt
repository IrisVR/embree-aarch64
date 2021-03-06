## Copyright 2009-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

SET(EMBREE_VERSION_MAJOR 3)
SET(EMBREE_VERSION_MINOR 12)
SET(EMBREE_VERSION_PATCH 1)
SET(EMBREE_VERSION_NOTE "")

SET(EMBREE_VERSION ${EMBREE_VERSION_MAJOR}.${EMBREE_VERSION_MINOR}.${EMBREE_VERSION_PATCH})
MATH(EXPR EMBREE_VERSION_NUMBER "10000*${EMBREE_VERSION_MAJOR} + 100*${EMBREE_VERSION_MINOR} + ${EMBREE_VERSION_PATCH}")
SET(CPACK_RPM_PACKAGE_RELEASE 1)

# https://stackoverflow.com/questions/42716466/how-can-i-tell-within-a-cmakelists-txt-whether-its-used-with-add-subdirectory
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  # I am top-level project.
  SET(EMBREE_AS_SUBMODULE Off)
else()
  # I am called from other project with add_subdirectory().
  MESSAGE(STATUS "Build Embree as submodule") 
  SET(EMBREE_AS_SUBMODULE On)
endif()

PROJECT(embree${EMBREE_VERSION_MAJOR})

CMAKE_MINIMUM_REQUIRED(VERSION 3.1.0)

# We use our own strip tool on macOS to sign during install. This is required as CMake modifies RPATH of the binary during install.
IF (APPLE AND EMBREE_SIGN_FILE)
  SET(EMBREE_STRIP ${CMAKE_STRIP})
  SET(CMAKE_STRIP ${PROJECT_BINARY_DIR}/post_install_target.sh)
  CONFIGURE_FILE(scripts/post_install_target.sh.in ${PROJECT_BINARY_DIR}/post_install_target.sh @ONLY)
ENDIF()

MACRO (SIGN_TARGET target)
  IF (EMBREE_SIGN_FILE)
    IF (WIN32)
      # on Windows we sign and do not strip as debug symbols not included in binary
      ADD_CUSTOM_COMMAND(TARGET ${target} POST_BUILD
        COMMAND ${EMBREE_SIGN_FILE} $<TARGET_FILE:${target}>)
    ELSEIF (APPLE)
      # on MacOSX we strip and sign here for testing purposes but also during install, as CMake modifies binary during install
      ADD_CUSTOM_COMMAND(TARGET ${target} POST_BUILD
        COMMAND ${EMBREE_STRIP} -x $<TARGET_FILE:${target}>
        COMMAND ${EMBREE_SIGN_FILE} -o runtime -e ${CMAKE_SOURCE_DIR}/common/cmake/embree.entitlements $<TARGET_FILE:${target}>)
    ELSE()
      # on Linux signing of binaries is not supported and stripping is done during install
    ENDIF()
  ENDIF()
ENDMACRO()

# find git version
IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git)
  FIND_PACKAGE(Git)
  IF(GIT_FOUND)
    EXECUTE_PROCESS(
      COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      OUTPUT_VARIABLE "EMBREE_HASH"
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE)
  ELSE()
    SET(EMBREE_HASH 0)
  ENDIF()
ENDIF()

IF(COMMAND cmake_policy)
  if (POLICY CMP0003)
    cmake_policy(SET CMP0003 NEW)
  endif()
  if (POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)
  endif()
  if(POLICY CMP0072)
    cmake_policy(SET CMP0072 OLD)
  endif()
  if(POLICY CMP0022)
    cmake_policy(SET CMP0022 NEW)
  endif()
  if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
  endif()
ENDIF(COMMAND cmake_policy)

MARK_AS_ADVANCED(CMAKE_BACKWARDS_COMPATIBILITY)
MARK_AS_ADVANCED(EXECUTABLE_OUTPUT_PATH)
MARK_AS_ADVANCED(LIBRARY_OUTPUT_PATH)

MARK_AS_ADVANCED(CMAKE_OSX_ARCHITECTURES)
MARK_AS_ADVANCED(CMAKE_OSX_DEPLOYMENT_TARGET)
MARK_AS_ADVANCED(CMAKE_OSX_SYSROOT)
MARK_AS_ADVANCED(CLEAR CMAKE_CXX_COMPILER)

SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/common/cmake ${CMAKE_MODULE_PATH})

IF (BUILD_TESTING)
  INCLUDE(test)
ENDIF()

set(BUILD_DOC OFF CACHE INTERNAL "build documentation (internal only)")
IF (BUILD_DOC)
  ADD_SUBDIRECTORY(doc)
ENDIF()

OPTION(EMBREE_TUTORIALS    "Enable to build Embree tutorials" ON)

##############################################################
# Embree configuration
##############################################################

GET_PROPERTY(SHARED_LIBS_SUPPORTED GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS)
IF(SHARED_LIBS_SUPPORTED)
  OPTION(EMBREE_STATIC_LIB "Build Embree as a static library.")
  MARK_AS_ADVANCED(CLEAR EMBREE_STATIC_LIB)
ELSE()
  SET(EMBREE_STATIC_LIB ON CACHE BOOL "Build Embree as a static library." FORCE)
  MARK_AS_ADVANCED(EMBREE_STATIC_LIB)
ENDIF()
IF (EMBREE_STATIC_LIB)
  SET(EMBREE_LIB_TYPE STATIC)
ELSE()
  SET(EMBREE_LIB_TYPE SHARED)
ENDIF()

# embree-aarch64 specific.
OPTION(EMBREE_USE_PARENT_PROJECT_COMPILER_FLAGS "Use parent project's cxx compiler flags(Useful when you want to embed embree to your project)" OFF)

OPTION(EMBREE_ISPC_SUPPORT "Build Embree with support for ISPC applications." ON)
OPTION(EMBREE_TUTORIALS    "Enable to build Embree tutorials" ON)
OPTION(EMBREE_BUILD_VERIFY "Build verify program for testing" ON)
SET(EMBREE_API_NAMESPACE "" CACHE STRING "C++ namespace to put API symbols into.")
SET(EMBREE_LIBRARY_NAME  "embree${EMBREE_VERSION_MAJOR}" CACHE STRING "Name of the embree library file (default is embree${EMBREE_VERSION_MAJOR})")

IF (EMBREE_USE_PARENT_PROJECT_COMPILER_FLAGS)
  IF (UNIX)
    # We still need -fPIC or -fPIE
    SET(CMAKE_POSITION_INDEPENDENT_CODE On)
  ENDIF ()
ENDIF (EMBREE_USE_PARENT_PROJECT_COMPILER_FLAGS)



IF (EMBREE_API_NAMESPACE AND EMBREE_ISPC_SUPPORT)
   MESSAGE(FATAL_ERROR "You cannot enable ISPC when EMBREE_API_NAMESPACE is used.")
ENDIF()

##############################################################################
# Configurations (add configurations also to common/cmake/embree-config.cmake)

OPTION(EMBREE_ZIP_MODE "Create Embree ZIP package" OFF)
MARK_AS_ADVANCED(EMBREE_ZIP_MODE)

OPTION(EMBREE_INSTALL_DEPENDENCIES "Install Embree dependencies in binary packages and install" OFF)
MARK_AS_ADVANCED(EMBREE_INSTALL_DEPENDENCIES)

OPTION(EMBREE_STAT_COUNTERS "Enables statistic counters.")
OPTION(EMBREE_STACK_PROTECTOR "When enabled Embree compiles with stack protection against return address overrides." OFF)

OPTION(EMBREE_RAY_MASK "Enables ray mask support.")
OPTION(EMBREE_BACKFACE_CULLING "Enables backface culling.")
OPTION(EMBREE_FILTER_FUNCTION "Enables filter functions." ON)
OPTION(EMBREE_IGNORE_INVALID_RAYS "Ignores invalid rays." OFF) # FIXME: enable by default?
OPTION(EMBREE_COMPACT_POLYS "Enables double indexed poly layout." OFF)

OPTION(EMBREE_GEOMETRY_TRIANGLE "Enables support for triangle geometries." ON)
OPTION(EMBREE_GEOMETRY_QUAD "Enables support for quad geometries." ON)
OPTION(EMBREE_GEOMETRY_CURVE "Enables support for curve geometries." ON)
OPTION(EMBREE_GEOMETRY_SUBDIVISION "Enables support for subdiv geometries." ON)
OPTION(EMBREE_GEOMETRY_USER "Enables support for user geometries." ON)
OPTION(EMBREE_GEOMETRY_INSTANCE "Enables support for instances." ON)
OPTION(EMBREE_GEOMETRY_GRID "Enables support for grid geometries." ON)
OPTION(EMBREE_GEOMETRY_POINT "Enables support for point geometries." ON)
OPTION(EMBREE_RAY_PACKETS "Enabled support for ray packets." ON)

OPTION(EMBREE_USE_CCACHE "Use ccache(for developer)." OFF)
OPTION(EMBREE_USE_LLD "Use lld(for clang only. required when cross-compiling embree for aarch64 target) You can set path to lld through LLD_PATH environment variable." OFF)

SET(EMBREE_MAX_INSTANCE_LEVEL_COUNT 1 CACHE STRING "Maximum number of instance levels.")
SET(EMBREE_CURVE_SELF_INTERSECTION_AVOIDANCE_FACTOR 2.0 CACHE STRING "Self intersection avoidance factor for flat curves. Specify floating point value in range 0 to inf.")
OPTION(EMBREE_MIN_WIDTH "Enables min-width feature to enlarge curve and point thickness to pixel width." OFF)

# CCache
if(EMBREE_USE_CCACHE)
  if(MSVC)
    # No ccache support
  else()
    find_program(CCACHE_EXE ccache)
    if(CCACHE_EXE)

      message(STATUS "Use ccache : " ${CCACHE_EXE})
      # CMAKE_C_COMPILER_LAUNCHER = available from cmake 3.4+)
      if(CMAKE_C_COMPILER_LAUNCHER)
        set(CMAKE_C_COMPILER_LAUNCHER "${CMAKE_C_COMPILER_LAUNCHER}"
                                      "${CCACHE_EXE}")
      else()
        set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_EXE}")
      endif()

      if(CMAKE_CXX_COMPILER_LAUNCHER)
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CMAKE_CXX_COMPILER_LAUNCHER}"
                                        "${CCACHE_EXE}")
      else()
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_EXE}")
      endif()

    endif(CCACHE_EXE)
  endif()
endif(EMBREE_USE_CCACHE)

##############################################################
# Mobile Support (support arm64 / aarch64 on Android and iOS)
##############################################################

OPTION(EMBREE_USE_EMBEDDED_TBB "Build TBB from source(please download and extract it to third_party/tbb." OFF)

# We forward-assign these variables for sanity and ease of debugging the build system.
# They are not directly being used.
SET(EMBREE_ANDROID ${ANDROID})
SET(EMBREE_IOS ${IOS})

IF (IOS OR AS_MAC)
    ADD_DEFINITIONS(-DBUILD_IOS)
ENDIF()

# Identify whether we are compiling for ARM architecture
STRING(FIND "${DIST_ARCH}" "arm64" pos1)
IF(NOT pos1 MATCHES -1)
  SET(EMBREE_ARM ON)
  MESSAGE(STATUS "Detected ARM Architecture")
ENDIF()

IF(EMBREE_ARM AND AS_MAC)
  MESSAGE(STATUS "building for Apple Silicon Mac" )
  SET(EMBREE_NEON_AVX2_EMULATION ON)
  SET(EMBREE_IOS 1)
ENDIF() 

IF(EMBREE_NEON_AVX2_EMULATION)
  MESSAGE(STATUS "Emulation of AVX2 by 2xNEON" )
  SET(EMBREE_MAX_ISA AVX2)
  ADD_DEFINITIONS(-DNEON_AVX2_EMULATION)
ENDIf()


SET(EMBREE_TASKING_SYSTEM "TBB" CACHE STRING "Selects tasking system")
IF (WIN32)
  SET_PROPERTY(CACHE EMBREE_TASKING_SYSTEM PROPERTY STRINGS TBB INTERNAL PPL)
ELSE()
  SET_PROPERTY(CACHE EMBREE_TASKING_SYSTEM PROPERTY STRINGS TBB INTERNAL)
ENDIF()

IF (EMBREE_TASKING_SYSTEM STREQUAL "TBB")
  SET(TASKING_GCD      OFF )
  SET(TASKING_TBB      ON )
  SET(TASKING_INTERNAL OFF)
  SET(TASKING_PPL      OFF )
  ADD_DEFINITIONS(-DTASKING_TBB)
  LIST(APPEND ISPC_DEFINITIONS -DTASKING_TBB)
ELSEIF (EMBREE_TASKING_SYSTEM STREQUAL "PPL")
  SET(TASKING_GCD      OFF )
  SET(TASKING_PPL      ON )
  SET(TASKING_TBB      OFF )
  SET(TASKING_INTERNAL OFF)
  ADD_DEFINITIONS(-DTASKING_PPL)
  LIST(APPEND ISPC_DEFINITIONS -DTASKING_PPL)
ELSEIF (EMBREE_TASKING_SYSTEM STREQUAL "GCD" AND IOS)
  SET(TASKING_GCD      ON )
  SET(TASKING_PPL      OFF )
  SET(TASKING_TBB      OFF )
  SET(TASKING_INTERNAL OFF)
  ADD_DEFINITIONS(-DTASKING_GCD)
  LIST(APPEND ISPC_DEFINITIONS -DTASKING_GCD)
ELSE()
  SET(TASKING_GCD      OFF )
  SET(TASKING_INTERNAL ON )
  SET(TASKING_TBB      OFF)
  SET(TASKING_PPL      OFF )
  ADD_DEFINITIONS(-DTASKING_INTERNAL)
  LIST(APPEND ISPC_DEFINITIONS -DTASKING_INTERNAL)
ENDIF()

##############################################################
# Compiler
##############################################################

SET(CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo")

IF (WIN32)
  IF (NOT DEFAULT_CMAKE_CONFIGURATION_TYPES_SET)
    SET(CMAKE_CONFIGURATION_TYPES "${CONFIGURATION_TYPES}" CACHE STRING "List of generated configurations." FORCE)
    SET(DEFAULT_CMAKE_CONFIGURATION_TYPES_SET ON CACHE INTERNAL "Default CMake configuration types set.")
  ENDIF()
  SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

  IF (${CMAKE_GENERATOR_TOOLSET} MATCHES "^LLVM" )
    MESSAGE("CLANG detected")
    INCLUDE(clang)
  ELSEIF(${CMAKE_GENERATOR_TOOLSET} MATCHES "^Intel")
    MESSAGE("Intel Compiler detected")
    INCLUDE (intel)
  ELSE()
    IF (MINGW)
      IF (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # Assume llvm-mingw
        INCLUDE (clang)
      ELSE ()
        INCLUDE (gnu)
      ENDIF ()
    ELSE (MINGW)
      IF (EMBREE_ISA_AVX512KNL OR EMBREE_ISA_AVX512SKX)
        MESSAGE(FATAL_ERROR "Microsoft Visual C++ Compiler does not support AVX512. Please use Intel Compiler or Clang.")
      ENDIF()
      INCLUDE (msvc)
    ENDIF (MINGW)
  ENDIF()

ELSE (WIN32)
  IF(CMAKE_CXX_COMPILER_WRAPPER STREQUAL "CrayPrgEnv")
    INCLUDE (crayprgenv)
  ELSE()
    STRING(TOLOWER "${CMAKE_CXX_COMPILER_ID}" _LOWER_CXX_COMPILER_ID)
    STRING(REPLACE "appleclang" "clang" _LOWER_CXX_COMPILER_ID ${_LOWER_CXX_COMPILER_ID})
    INCLUDE(${_LOWER_CXX_COMPILER_ID} OPTIONAL RESULT_VARIABLE COMPILER_FOUND)
    IF (NOT COMPILER_FOUND)
      MESSAGE(FATAL_ERROR "Unsupported compiler: " ${CMAKE_CXX_COMPILER_ID})
    ENDIF ()
  ENDIF ()

  IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE "Release" CACHE STRING "Specifies the build type." FORCE)
    SET_PROPERTY(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CONFIGURATION_TYPES})
  ENDIF(NOT CMAKE_BUILD_TYPE)

  OPTION(CMAKE_VERBOSE_MAKEFILE "Enables verbose mode.")
  MARK_AS_ADVANCED(CLEAR CMAKE_VERBOSE_MAKEFILE)

ENDIF (WIN32)

##############################################################
# ISA configuration
##############################################################

IF(CMAKE_CXX_COMPILER_WRAPPER STREQUAL "CrayPrgEnv")
  SET(EMBREE_MAX_ISA "DEFAULT" CACHE STRING "Selects highest ISA to support.")
ELSE()
  SET(EMBREE_MAX_ISA "NONE" CACHE STRING "Selects highest ISA to support.")
ENDIF()
SET_PROPERTY(CACHE EMBREE_MAX_ISA PROPERTY STRINGS NONE SSE2 SSE4.2 AVX AVX2 AVX512KNL AVX512SKX DEFAULT)

IF (EMBREE_MAX_ISA STREQUAL "NONE")
  TRY_COMPILE(COMPILER_SUPPORTS_AVX       ${CMAKE_BINARY_DIR} ${PROJECT_SOURCE_DIR}/common/cmake/check_isa.cpp COMPILE_DEFINITIONS ${FLAGS_AVX})
  TRY_COMPILE(COMPILER_SUPPORTS_AVX2      ${CMAKE_BINARY_DIR} ${PROJECT_SOURCE_DIR}/common/cmake/check_isa.cpp COMPILE_DEFINITIONS ${FLAGS_AVX2})
  TRY_COMPILE(COMPILER_SUPPORTS_AVX512KNL ${CMAKE_BINARY_DIR} ${PROJECT_SOURCE_DIR}/common/cmake/check_isa.cpp COMPILE_DEFINITIONS ${FLAGS_AVX512KNL})
  TRY_COMPILE(COMPILER_SUPPORTS_AVX512SKX ${CMAKE_BINARY_DIR} ${PROJECT_SOURCE_DIR}/common/cmake/check_isa.cpp COMPILE_DEFINITIONS ${FLAGS_AVX512SKX})

  OPTION(EMBREE_ISA_SSE2 "Enables SSE2 ISA." ON)
  OPTION(EMBREE_ISA_SSE42 "Enables SSE4.2 ISA." ON)
  OPTION(EMBREE_ISA_AVX "Enables AVX ISA." ${COMPILER_SUPPORTS_AVX})
  OPTION(EMBREE_ISA_AVX2 "Enables AVX2 ISA." ${COMPILER_SUPPORTS_AVX2})
  IF (WIN32 OR APPLE)
    OPTION(EMBREE_ISA_AVX512KNL "Enables AVX512 ISA for Knights Landing." OFF)
    OPTION(EMBREE_ISA_AVX512SKX "Enables AVX512 ISA for Skylake." OFF)
  ELSE()
    OPTION(EMBREE_ISA_AVX512KNL "Enables AVX512 ISA for Knights Landing." OFF) # compilation on GCC 8 broken
    OPTION(EMBREE_ISA_AVX512SKX "Enables AVX512 ISA for Skylake." ${COMPILER_SUPPORTS_AVX512SKX})
  ENDIF()
ELSEIF (EMBREE_MAX_ISA STREQUAL "DEFAULT")
  UNSET(EMBREE_ISA_SSE2 CACHE)
  UNSET(EMBREE_ISA_SSE42 CACHE)
  UNSET(EMBREE_ISA_AVX CACHE)
  UNSET(EMBREE_ISA_AVX2 CACHE)
  UNSET(EMBREE_ISA_AVX512KNL CACHE)
  UNSET(EMBREE_ISA_AVX512SKX CACHE)
  SET(EMBREE_ISA_SSE2 OFF)
  SET(EMBREE_ISA_SSE42 OFF)
  SET(EMBREE_ISA_AVX OFF)
  SET(EMBREE_ISA_AVX2 OFF)
  SET(EMBREE_ISA_AVX512KNL OFF)
  SET(EMBREE_ISA_AVX512SKX OFF)
  MESSAGE(STATUS "Detecting default ISA...")
  INCLUDE(check_isa_default)
  CHECK_ISA_DEFAULT(EMBREE_ISA_DEFAULT)
  MESSAGE(STATUS "Detected default ISA: ${EMBREE_ISA_DEFAULT}")
  SET(EMBREE_ISA_${EMBREE_ISA_DEFAULT} ON)
ELSE()
  UNSET(EMBREE_ISA_SSE2 CACHE)
  UNSET(EMBREE_ISA_SSE42 CACHE)
  UNSET(EMBREE_ISA_AVX CACHE)
  UNSET(EMBREE_ISA_AVX2 CACHE)
  UNSET(EMBREE_ISA_AVX512KNL CACHE)
  UNSET(EMBREE_ISA_AVX512SKX CACHE)

  IF(EMBREE_MAX_ISA STREQUAL "SSE2")
    SET(ISA  1)
  ELSEIF(EMBREE_MAX_ISA STREQUAL "SSE4.2")
    SET(ISA  2)
  ELSEIF(EMBREE_MAX_ISA STREQUAL "AVX")
    SET(ISA  3)
  ELSEIF(EMBREE_MAX_ISA STREQUAL "AVX2")
    SET(ISA  4)
  ELSEIF(EMBREE_MAX_ISA STREQUAL "AVX512KNL")
    SET(ISA  5)
  ELSEIF(EMBREE_MAX_ISA STREQUAL "AVX512SKX")
    SET(ISA 6)
  ELSE()
    MESSAGE(FATAL_ERROR "Unsupported ISA specified: " ${EMBREE_MAX_ISA})
  ENDIF()

  SET(EMBREE_ISA_SSE2 OFF)
  SET(EMBREE_ISA_SSE42 OFF)
  SET(EMBREE_ISA_AVX OFF)
  SET(EMBREE_ISA_AVX2 OFF)
  SET(EMBREE_ISA_AVX512KNL OFF)
  SET(EMBREE_ISA_AVX512SKX OFF)

  IF (ISA GREATER 0)
    SET(EMBREE_ISA_SSE2  ON)
  ENDIF ()
  IF (ISA GREATER 1)
    SET(EMBREE_ISA_SSE42  ON)
  ENDIF ()
  IF (ISA GREATER 2)
    SET(EMBREE_ISA_AVX  ON)
  ENDIF ()
  IF (ISA GREATER 3)
    SET(EMBREE_ISA_AVX2  ON)
  ENDIF ()
  IF (ISA GREATER 4)
    SET(EMBREE_ISA_AVX512KNL  ON)
  ENDIF ()
  IF (ISA GREATER 5)
    SET(EMBREE_ISA_AVX512SKX  ON)
  ENDIF ()
ENDIF()

IF(CMAKE_CXX_COMPILER_WRAPPER STREQUAL "CrayPrgEnv")
  SET(EMBREE_ISA_SSE42 OFF)
ENDIF()


IF(EMBREE_NEON_AVX2_EMULATION)
  SET(EMBREE_ISA_SSE2 ON)
  SET(EMBREE_ISA_SSE42 ON)
  SET(EMBREE_ISA_AVX ON)
  SET(FLAGS_SSE2 "") 
  SET(FLAGS_SSE42 "-D__SSE4_2__  -D__SSE4_1__  ")
  SET(FLAGS_AVX "-D__AVX__ -D__SSE4_2__  -D__SSE4_1__  -D__BMI__ -D__BMI2__ -D__LZCNT__")
  SET(FLAGS_AVX2 "-D__AVX2__ -D__AVX__ -D__SSE4_2__  -D__SSE4_1__  -D__BMI__ -D__BMI2__ -D__LZCNT__")
ENDIf()




IF (APPLE AND EMBREE_STATIC_LIB)

  # count number of set ISAs
  SET(NUMISA 0)
  IF (EMBREE_ISA_SSE2)
    MATH(EXPR NUMISA "${NUMISA}+1")
  ENDIF()
  IF (EMBREE_ISA_SSE42)
    MATH(EXPR NUMISA "${NUMISA}+1")
  ENDIF()
  IF (EMBREE_ISA_AVX)
    MATH(EXPR NUMISA "${NUMISA}+1")
  ENDIF()
  IF (EMBREE_ISA_AVX2)
    MATH(EXPR NUMISA "${NUMISA}+1")
  ENDIF()
  IF (EMBREE_ISA_AVX512KNL)
    MATH(EXPR NUMISA "${NUMISA}+1")
  ENDIF()
  IF (EMBREE_ISA_AVX512SKX)
    MATH(EXPR NUMISA "${NUMISA}+1")
  ENDIF()

  IF (NUMISA GREATER 1)
    IF (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
      IF (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER "9.0.0" OR ${CMAKE_CXX_COMPILER_VERSION} VERSION_EQUAL "9.0.0")
        MESSAGE(FATAL_ERROR "Using Embree as static library is not supported with AppleClang >= 9.0 when multiple ISAs are selected. Please either build a shared library or enable only one ISA.")
      ENDIF()
    ENDIF()
  ENDIF()
ENDIF()

##############################################################
# ISA configuration continued
##############################################################

SET(SSE2 0)
SET(SSE42 1)
SET(AVX 2)
SET(AVX2 3)
SET(AVX512KNL 4)
SET(AVX512SKX 5)

UNSET(FLAGS_LOWEST)
SET(ISA_LOWEST_AVX 2)

IF (EMBREE_ISA_SSE2)
  ADD_DEFINITIONS(-DEMBREE_TARGET_SSE2)
  LIST(APPEND ISPC_TARGETS "sse2")
  IF(NOT FLAGS_LOWEST)
    SET(ISA_LOWEST ${SSE2})
    SET(FLAGS_LOWEST ${FLAGS_SSE2})
  ENDIF()
ENDIF()

IF (EMBREE_ISA_SSE42)
  ADD_DEFINITIONS(-DEMBREE_TARGET_SSE42)
  LIST(APPEND ISPC_TARGETS "sse4")
  IF(NOT FLAGS_LOWEST)
    SET(ISA_LOWEST ${SSE42})
    SET(FLAGS_LOWEST ${FLAGS_SSE42})
  ENDIF()
ENDIF ()

IF (EMBREE_ISA_AVX)
  ADD_DEFINITIONS(-DEMBREE_TARGET_AVX)
  LIST(APPEND ISPC_TARGETS "avx")
  IF(NOT FLAGS_LOWEST)
    SET(ISA_LOWEST ${AVX})
    SET(ISA_LOWEST_AVX ${AVX})
    SET(FLAGS_LOWEST ${FLAGS_AVX})
  ENDIF()
ENDIF ()

IF (EMBREE_ISA_AVX2)
  ADD_DEFINITIONS(-DEMBREE_TARGET_AVX2)
  LIST(APPEND ISPC_TARGETS "avx2")
  IF(NOT FLAGS_LOWEST)
    SET(ISA_LOWEST ${AVX2})
    SET(ISA_LOWEST_AVX ${AVX2})
    SET(FLAGS_LOWEST ${FLAGS_AVX2})
  ENDIF()
ENDIF ()

IF (EMBREE_ISA_AVX512KNL)
  ADD_DEFINITIONS(-DEMBREE_TARGET_AVX512KNL)
  LIST(APPEND ISPC_TARGETS "avx512knl-i32x16")
  IF(NOT FLAGS_LOWEST)
    SET(ISA_LOWEST ${AVX512KNL})
    SET(ISA_LOWEST_AVX ${AVX512KNL})
    SET(FLAGS_LOWEST ${FLAGS_AVX512KNL})
  ENDIF()
ENDIF ()

IF (EMBREE_ISA_AVX512SKX)
  ADD_DEFINITIONS(-DEMBREE_TARGET_AVX512SKX)
  LIST(APPEND ISPC_TARGETS "avx512skx-i32x16")
  IF(NOT FLAGS_LOWEST)
    SET(ISA_LOWEST ${AVX512SKX})
    SET(ISA_LOWEST_AVX ${AVX512SKX})
    SET(FLAGS_LOWEST ${FLAGS_AVX512SKX})
  ENDIF()
ENDIF ()

INCLUDE (ispc)

##############################################################
# Find TBB
##############################################################
IF (TASKING_TBB)
  if (EMBREE_USE_EMBEDDED_TBB)
    # Build TBB using `add_subdirectory`
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/deps/tbb)
  else (EMBREE_USE_EMBEDDED_TBB)
    FIND_PACKAGE(TBB REQUIRED)
  endif (EMBREE_USE_EMBEDDED_TBB)
  INCLUDE_DIRECTORIES(${TBB_INCLUDE_DIRS})
  message(STATUS "TBB inc: " ${TBB_INCLUDE_DIRS})
  message(STATUS "TBB lib: " ${TBB_LIBRARIES})
ENDIF()

##############################################################
# Create Binary Packages (uses above config options)
##############################################################
include(package)

##############################################################
# Create Config files
##############################################################

CONFIGURE_FILE(
  "${PROJECT_SOURCE_DIR}/kernels/config.h.in"
  "${PROJECT_SOURCE_DIR}/kernels/config.h"
)

CONFIGURE_FILE(
  "${PROJECT_SOURCE_DIR}/kernels/rtcore_config.h.in"
  "${PROJECT_SOURCE_DIR}/include/embree3/rtcore_config.h"
)

CONFIGURE_FILE(
  "${PROJECT_SOURCE_DIR}/kernels/hash.h.in"
  "${PROJECT_SOURCE_DIR}/kernels/hash.h"
)
CONFIGURE_FILE(
  "${PROJECT_SOURCE_DIR}/kernels/export.linux.map.in"
  "${PROJECT_SOURCE_DIR}/kernels/export.linux.map"
)
CONFIGURE_FILE(
  "${PROJECT_SOURCE_DIR}/kernels/export.macosx.map.in"
  "${PROJECT_SOURCE_DIR}/kernels/export.macosx.map"
)


##############################################################
# Output paths
##############################################################
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

##############################################################
# Directories to compile
##############################################################

ADD_SUBDIRECTORY(common)
ADD_SUBDIRECTORY(kernels)
IF (EMBREE_TUTORIALS)
  ADD_SUBDIRECTORY(tutorials)
ENDIF()
IF (EMBREE_BUILD_VERIFY)
  IF (NOT EMBREE_TUTORIALS)
    ADD_SUBDIRECTORY(tutorials/common/image)
    ADD_SUBDIRECTORY(tutorials/common)
    ADD_SUBDIRECTORY(tutorials/verify)
  ENDIF()
ENDIF()

##############################################################
# Uninstall
##############################################################

IF (NOT EMBREE_AS_SUBMODULE)

  configure_file(
      "${CMAKE_CURRENT_SOURCE_DIR}/common/cmake/uninstall.cmake.in"
      "${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake"
      IMMEDIATE @ONLY)
  
  add_custom_target(uninstall
      COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake)
  
  SET_PROPERTY(TARGET uninstall PROPERTY FOLDER CMakePredefinedTargets)

ENDIF ()

##############################################################
# Has to be last
##############################################################

IF (NOT EMBREE_AS_SUBMODULE)

  INCLUDE(CPack)

ENDIF ()
