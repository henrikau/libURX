set(LIBS
  urx
  )

set (APPS
  get_remote
  send_script
  joint_speed
  robot_speed
  measure_roundtrip
  tcp_pose
  )

foreach(a ${APPS})
  add_executable (${a} ${a}.cpp)
  target_link_libraries (${a} ${LIBS})
  target_compile_definitions(${a} PRIVATE )
  install (TARGETS ${a} DESTINATION bin)
endforeach(a)

# Simple movej-script for send_script.cpp
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../scripts/simple_movej.script
  ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
# script to use in tandem with robot_speed
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../scripts/ur_speedj.script
  ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
# script to use in tandem with tcp_poset
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../scripts/tcp_pose.script
  ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)


# install sample UR-scripts
install (FILES ../scripts/simple_movej.script DESTINATION share/liburx)
install (FILES ../scripts/ur_speedj.script DESTINATION share/liburx)
install (FILES ../scripts/tcp_pose.script DESTINATION share/liburx)
