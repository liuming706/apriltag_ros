cmake_minimum_required(VERSION 3.20)
project(rosa_msgs)

find_package(ament_cmake REQUIRED)
find_package(std_msgs REQUIRED)

if("$ENV{ROSA_VERSION}" STREQUAL "2")

  find_package(rosa_generate_tools REQUIRED)
  generate_msgs_library(${PROJECT_NAME} DEPENDS std_msgs)

else()

  find_package(rosidl_default_generators REQUIRED)

  rosidl_generate_interfaces(${PROJECT_NAME} "msg/NodeState.msg" DEPENDENCIES
                             std_msgs)

  ament_export_dependencies(rosidl_default_runtime)

endif()

ament_package()
