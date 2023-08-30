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

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, DeclareLaunchArgument
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node, SetRemap

def generate_launch_description():
    apriltag_ros_config_file_dir = os.path.join(get_package_share_directory('apriltag_ros'), 'cfg', 'tags_36h11.yaml')
    april_tag_pub_node = Node(
        package = 'apriltag_ros',
        executable = 'apriltag_node',
        parameters = [apriltag_ros_config_file_dir],
        #arguments = [
        #    '-configuration_directory', FindPackageShare('cartographer_ros').find('cartographer_ros') + '/configuration_files',
        #    '-configuration_basename', 'turtlebot_2d.lua',
        #    '-load_state_filename', load_state_filename,'-minloglevel','0'],
        remappings=[
            ('/image_rect', '/camera/image_raw'),
            ('/camera_info', '/camera/camera_info'),],
        output = 'screen',
        )
    ld = LaunchDescription()
    ld.add_action(april_tag_pub_node)
    return ld

