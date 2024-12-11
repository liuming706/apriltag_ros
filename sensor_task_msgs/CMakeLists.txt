cmake_minimum_required(VERSION 3.20)
project(sensor_task_msgs)

find_package(ament_cmake REQUIRED)

find_package(rosa_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)

if("$ENV{ROSA_VERSION}"
   STREQUAL
   "2"
)

  find_package(rosa_generate_tools REQUIRED)

  generate_msgs_library(
    ${PROJECT_NAME}
    DEPENDS
    # 在下面添加依赖的 msg
    rosa_msgs
    geometry_msgs
  )

else()

  find_package(rosidl_default_generators REQUIRED)

  macro(
    get_files_with_suffix
    result_var
    suffix
  )
    set(MSG_DIR "${CMAKE_SOURCE_DIR}/${suffix}")
    if(EXISTS "${MSG_DIR}")
      file(
        GLOB_RECURSE files
        RELATIVE "${CMAKE_SOURCE_DIR}"
        "${MSG_DIR}/*.${suffix}"
      )
      set(${result_var} ${files})
    endif()
  endmacro()

  get_files_with_suffix(MSG_FILES "msg")
  get_files_with_suffix(SRV_FILES "srv")
  get_files_with_suffix(ACTION_FILES "action")

  rosidl_generate_interfaces(
    ${PROJECT_NAME}
    ${MSG_FILES}
    ${SRV_FILES}
    ${ACTION_FILES}
    DEPENDENCIES
    rosa_msgs
    geometry_msgs
  )

  ament_export_dependencies(rosidl_default_runtime)

endif()

ament_package()
