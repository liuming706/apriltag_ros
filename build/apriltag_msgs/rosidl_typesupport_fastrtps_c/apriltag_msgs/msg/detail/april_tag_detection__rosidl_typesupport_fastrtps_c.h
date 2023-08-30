// generated from rosidl_typesupport_fastrtps_c/resource/idl__rosidl_typesupport_fastrtps_c.h.em
// with input from apriltag_msgs:msg/AprilTagDetection.idl
// generated code does not contain a copyright notice
#ifndef APRILTAG_MSGS__MSG__DETAIL__APRIL_TAG_DETECTION__ROSIDL_TYPESUPPORT_FASTRTPS_C_H_
#define APRILTAG_MSGS__MSG__DETAIL__APRIL_TAG_DETECTION__ROSIDL_TYPESUPPORT_FASTRTPS_C_H_

#include <stddef.h>
#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_typesupport_interface/macros.h"
#include "apriltag_msgs/msg/rosidl_typesupport_fastrtps_c__visibility_control.h"
#include "apriltag_msgs/msg/detail/april_tag_detection__struct.h"
#include "fastcdr/Cdr.h"

#ifdef __cplusplus
extern "C" {
#endif

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_apriltag_msgs bool cdr_serialize_apriltag_msgs__msg__AprilTagDetection(
    const apriltag_msgs__msg__AprilTagDetection *ros_message, eprosima::fastcdr::Cdr &cdr);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_apriltag_msgs bool cdr_deserialize_apriltag_msgs__msg__AprilTagDetection(
    eprosima::fastcdr::Cdr &, apriltag_msgs__msg__AprilTagDetection *ros_message);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_apriltag_msgs size_t
get_serialized_size_apriltag_msgs__msg__AprilTagDetection(const void *untyped_ros_message, size_t current_alignment);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_apriltag_msgs size_t
max_serialized_size_apriltag_msgs__msg__AprilTagDetection(bool &full_bounded, bool &is_plain, size_t current_alignment);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_apriltag_msgs bool cdr_serialize_key_apriltag_msgs__msg__AprilTagDetection(
    const apriltag_msgs__msg__AprilTagDetection *ros_message, eprosima::fastcdr::Cdr &cdr);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_apriltag_msgs size_t
get_serialized_size_key_apriltag_msgs__msg__AprilTagDetection(const void *untyped_ros_message, size_t current_alignment);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_apriltag_msgs size_t max_serialized_size_key_apriltag_msgs__msg__AprilTagDetection(
    bool &full_bounded, bool &is_plain, size_t current_alignment);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_apriltag_msgs const rosidl_message_type_support_t
    *ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_fastrtps_c, apriltag_msgs, msg,
                                                       AprilTagDetection)();

#ifdef __cplusplus
}
#endif

#endif  // APRILTAG_MSGS__MSG__DETAIL__APRIL_TAG_DETECTION__ROSIDL_TYPESUPPORT_FASTRTPS_C_H_
