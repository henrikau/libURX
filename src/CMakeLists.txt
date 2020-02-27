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

set (LIBS
  ${Boost_LIBRARIES}		#Required for test. Need to figure out why that is
  )

add_library (urx SHARED ${SRCS})

target_link_libraries (urx ${LIBS})

set_target_properties (
  urx PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION})

install (TARGETS urx DESTINATION lib)
install (DIRECTORY include/ DESTINATION include FILES_MATCHING PATTERN "*.hpp")
