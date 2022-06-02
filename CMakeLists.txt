cmake_minimum_required(VERSION 2.8)

project (urx)

set (PROJECT_AUTHOR "SINTEF Digital")
set (PROJECT_URL "http://localhost/")
set (PROJECT_LICENSE "MPLv2")
set (PROJECT_VERSION "1.0")

option(BUILD_TESTS    "Build tests and emulators" ON)
option(ALT_LIBS  "Alternative location for libraries" OFF)
option(USE_TSNLIB "Build with tsnlib (tsn-proxy)" OFF)
enable_testing ()

include (CheckCXXCompilerFlag REQUIRED)

function(set_cxx_flag FLAG)
  check_cxx_compiler_flag (${FLAG} CXX_SUPPORT_FLAG)
  if (${CXX_SUPPORT_FLAG})
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAG}" PARENT_SCOPE)
  else ()
    message(FATAL_ERROR "Failed setting ${FLAG}, please update CMakeLists")
  endif ()
endfunction(set_cxx_flag)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
set_cxx_flag("-Wall")
set_cxx_flag("-pedantic")
set_cxx_flag("-Wextra")
set_cxx_flag("-Werror")
set_cxx_flag("-Wfatal-errors")
set_cxx_flag("-std=c++20")

# FIXME: want to enable this, but this will hit you *hard*
# set_cxx_flag("-fsanitize=address")

check_cxx_compiler_flag ("-fpic" CXX_SUPPORT_FPIC_1)
check_cxx_compiler_flag ("-fPIC" CXX_SUPPORT_FPIC_2)

if (${CXX_SUPPORT_FPIC_1})
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpic")
elseif (${CXX_SUPPORT_FPIC_2})
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
else ()
  message (WARNING "${CMAKE_CXX_COMPILER} does not support -fpic or -fPIC flags")
endif ()

include (CheckCCompilerFlag REQUIRED)


set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -pedantic -Wextra")
if (USE_TSNLIB)
  add_definitions(-DUSE_TSN)
endif ()

check_c_compiler_flag ("-fpic" C_SUPPORT_FPIC_1)
check_c_compiler_flag ("-fPIC" C_SUPPORT_FPIC_2)
if (${C_SUPPORT_FPIC_1})
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpic")
elseif (${C_SUPPORT_FPIC_2})
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
else ()
  message (WARNING "${CMAKE_C_COMPILER} does not support -fpic or -fPIC flags")
endif ()

include_directories (BEFORE "include/")

add_subdirectory (src)
if (BUILD_TESTS)
  add_subdirectory (test)
endif (BUILD_TESTS)

add_subdirectory (tools)

# Doxygen documentation
# Will add doxygen and graphviz to the list of requirements
option(BUILD_DOC "Build Documentation" ON)
find_package(Doxygen)
if (DOXYGEN_FOUND)

  set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
  set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
  configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

  add_custom_target(doc
    ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating liburx API documentation."
    VERBATIM)
else (DOXYGEN_FOUND)
  message (WARNING "Doxygen must be installed in order to make documentation")
endif (DOXYGEN_FOUND)

install (DIRECTORY include/ DESTINATION include FILES_MATCHING PATTERN "*.h*")
