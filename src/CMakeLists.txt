# Need to have unit_test_framework available here for ../test/ to link properly..
find_package (Boost COMPONENTS program_options unit_test_framework log REQUIRED)

################################################################################
# librtde
################################################################################

set (SRCS
  con.cpp
  header.cpp
  rtde_handler.cpp
  rtde_recipe.cpp
  rtde_recipe_token.cpp
  urx_script.cpp
  urx_handler.cpp
  robot.cpp
  )

if (USE_TSN)
  set (SRCS
    ${SRCS}
    tsn_con.cpp
    rtde_tsn_handler.cpp
    )
endif ()
set (LIBS
  ${Boost_LIBRARIES}		#Required for test. Need to figure out why that is
)

if (USE_TSNLIB)
  if (ALT_LIBS)
    add_library(avtp SHARED IMPORTED)
    set_target_properties(avtp PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/${ALT_LIBS}/lib/libavtp.so.0.1.0)
    set_property(TARGET avtp APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/${ALT_LIBS}/include>)

    add_library(tsn SHARED IMPORTED)
    set_target_properties(tsn PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/${ALT_LIBS}/lib/libtsn.so.1.0)
    set_property(TARGET avtp APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/${ALT_LIBS}/include>)
  else ()
    message (WARNING "ALT_LIBS is not set, this means that libavtp should be installed system-wide. ")
    message (WARNING "This has not been tested. You're on your own.")
  endif ()
  set (LIBS
    ${LIBS}
    avtp
    tsn
    )
endif ()



add_library (urx SHARED ${SRCS})

target_link_libraries (urx ${LIBS})

set_target_properties (
  urx PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION})

install (TARGETS urx DESTINATION lib)
