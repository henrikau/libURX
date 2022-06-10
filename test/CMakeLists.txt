find_package (Boost COMPONENTS program_options unit_test_framework log REQUIRED)
set(SRCS
  test_server.cpp
  )

add_library(testlib SHARED ${SRCS})
set (LIBS
  urx
  pthread
  )
set(TESTS
  header-test
  helper_test
  unit_converter_test
  # con_test
  rtde_handler_test
  rtde_recipe_test
  rtde_recipe_token_test
  urx_script_test
  urx_handler_test
  robot_test
  )

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../scripts/ur_script_movej.script
  ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../scripts/simple_movej.script
  ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

find_program (BASH_PROGRAM bash)
if (BASH_PROGRAM)
  add_test (set_test_file_capabilities ${BASH_PROGRAM} ${CMAKE_SOURCE_DIR}/scripts/set_props.sh)
endif (BASH_PROGRAM)

foreach(t ${TESTS})
  add_executable (${t} ${t}.cpp)
  target_link_libraries (${t} ${LIBS})
  add_test(${t} ${CMAKE_CURRENT_BINARY_DIR}/${t})
endforeach(t)
