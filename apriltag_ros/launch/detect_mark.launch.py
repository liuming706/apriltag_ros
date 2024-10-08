#!/usr/bin/env python3
#
# Copyright 2019 ROBOTIS CO., LTD.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Authors: Joep Tool

import os
from launch_ros.actions import Node, SetRemap
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, DeclareLaunchArgument
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration


def generate_launch_description():

    config = os.path.join(
        get_package_share_directory("apriltag_ros"), "cfg", "tags_36h11.yaml"
    )
    apriltag_node = Node(
        package="apriltag_ros",
        executable="apriltag_node",
        parameters=[config],
        remappings=[
            ("/image", "/sensor/camera/stereo_left/image/raw"),
            (
                "camera_info",
                "/sensor/camera/stereo_left/image/info",
            ),
        ],
        output="screen",
    )

    ld = LaunchDescription()
    ld.add_action(apriltag_node)
    return ld
