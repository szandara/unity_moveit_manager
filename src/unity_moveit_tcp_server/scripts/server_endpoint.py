#!/usr/bin/env python

import rospy
import rosparam

from ros_tcp_endpoint import TcpServer, RosPublisher, RosSubscriber
from sensor_msgs.msg import JointState
from trajectory_msgs.msg import JointTrajectory

UNITY_CONTROLLER_BASE = '/move_group/unity_trajectory/'
UNITY_CONTROLLER_LIST_PARAM = 'controller_list'

ROS_JOINT_STATES_TOPIC = '/joint_states'

TCP_NODE_NAME_PARAM = '/TCP_NODE_NAME'
TCP_BUFFER_SIZE_PARAM = '/TCP_BUFFER_SIZE'
TCP_CONNECTIONS_PARAM = '/TCP_CONNECTIONS'

def setup_kinematic_server():
    """
    Setup a unity TCP server which proxy the messages from and to Unity.
    It automatically setup a list of proxies for kinematic relevant ROS topics.

    /joint_states is used as the default joint configuration publisher for all controllers

    For each controller which is configured in the unity_moveit_controller_manager
    it sets up a controller topic with the following path

    /move_group/unity_trajectory/<CONTROLLER_NAME>

    This script is intended to work with unity_moveit_manager.cpp
    """
    ros_node_name = rospy.get_param(TCP_NODE_NAME_PARAM, 'TCPServer')
    buffer_size = rospy.get_param(TCP_BUFFER_SIZE_PARAM, 1024)
    connections = rospy.get_param(TCP_CONNECTIONS_PARAM, 10)

    tcp_server = TcpServer(ros_node_name, buffer_size, connections)
    rospy.init_node(ros_node_name, anonymous=True, log_level=rospy.INFO)

    rospy.loginfo('Advertising /joint_states for the joint configuration')
    topics = {
        'joint_states': RosPublisher(ROS_JOINT_STATES_TOPIC, JointState),
    }

    for controller in rosparam.get_param(UNITY_CONTROLLER_LIST_PARAM):
        controller_name = controller['name']
        topics[controller_name] = RosSubscriber(UNITY_CONTROLLER_BASE + controller_name,
                                                JointTrajectory,
                                                tcp_server)
        rospy.loginfo(f'Listening to {UNITY_CONTROLLER_BASE + controller_name} '
                      f'for the trajectory execution')

    tcp_server.start(topics)

    rospy.spin()


if __name__ == "__main__":
    setup_kinematic_server()
