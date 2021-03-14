/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2021, Simone Zandara
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Ioan A. Sucan nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/

/* Author: Simone Zandara */

#include <ros/ros.h>
#include <ros/console.h>
#include <moveit/controller_manager/controller_manager.h>
#include <sensor_msgs/JointState.h>
#include <pluginlib/class_list_macros.hpp>
#include <map>

namespace unity_moveit_manager
{

/**
 * Class which communicate with Unity the RobotTrajectory message
 * generated by a moveit_commander
 */
class UnityMoveitManagerHandler : public moveit_controller_manager::MoveItControllerHandle
{
private:
  ros::Publisher unity_pub_;
  static constexpr const char* UNITY_TRAJECTORY_BASE = "unity_trajectory/";
  std::vector<std::string> joints_;

public:
  typedef std::shared_ptr<UnityMoveitManagerHandler> Ptr;

  UnityMoveitManagerHandler(const std::string& name,
                            ros::NodeHandle& node_handle,
                            const std::vector<std::string> & joints) :
    moveit_controller_manager::MoveItControllerHandle(name), joints_(joints)
  {
    unity_pub_ = node_handle.
        advertise<moveit_msgs::RobotTrajectory>(UNITY_TRAJECTORY_BASE + name, 1000);
  }

  /**
   * Send the trajectory to Unity using a ROS topic.
   *
   * @param msg the Robot Trajectory calculated by moveit
   * @return bool if the message was sent successfully
   */
  bool sendTrajectory(const moveit_msgs::RobotTrajectory& msg) override
  {
    // Send the trajectory to Unity
    unity_pub_.publish(msg);
    return true;
  }

  /**
   * Not supported
   */
  bool cancelExecution() override
  {
    ROS_WARN_STREAM("Cancelling execution is not supported on Unity." <<
                    "Called cancelExecution() on controller " << name_);
    return true;
  }

  /**
   * Not supported
   */
  bool waitForExecution(const ros::Duration& /*timeout*/) override
  {
    ROS_ERROR_STREAM("waitForExecution execution is not supported on Unity." <<
                    "Called waitForExecution() on controller " << name_);
    return true;
  }

  /**
   * @return moveit_controller_manager::ExecutionStatus with the status of the last execution
   * Currently it's always success as Unity does not communicate back
   */
  moveit_controller_manager::ExecutionStatus getLastExecutionStatus() override
  {
    return moveit_controller_manager::ExecutionStatus(moveit_controller_manager::ExecutionStatus::SUCCEEDED);
  }

  /**
   * @return std::vector with the list of joints handled by this controller
   * Currently it's always success as Unity does not communicate back
   */
  std::vector<std::string> getJoints()
  {
    return joints_;
  }
};

/**
 * Class which manages the list of controllers for the current configured URDF.
 * It creates one UnityController instance per controller using the joint
 * names reported in the config file.
 */
class UnityMoveitManager : public moveit_controller_manager::MoveItControllerManager
{
public:
  UnityMoveitManager() : node_handle_("~")
  {
    if (!node_handle_.hasParam(CONFIG_CONTROLLER_LIST_PARAM))
    {
      ROS_ERROR_STREAM_NAMED(CONTROLLER_NAME, "No controller_list specified.");
      return;
    }

    XmlRpc::XmlRpcValue controller_list;
    node_handle_.getParam(CONFIG_CONTROLLER_LIST_PARAM, controller_list);
    if (controller_list.getType() != XmlRpc::XmlRpcValue::TypeArray)
    {
      ROS_ERROR_NAMED(CONTROLLER_NAME, "controller_list should be specified as an array");
      return;
    }

    bool latch = true;

    // Create each controller
    for (int i = 0; i < controller_list.size(); ++i)  // NOLINT(modernize-loop-convert)
    {
      if (!controller_list[i].hasMember(CONFIG_NAME_PARAM) ||
          !controller_list[i].hasMember(CONFIG_JOINTS_PARAM))
      {
        ROS_ERROR_NAMED(CONTROLLER_NAME, "Name and joints must be specified for each controller");
        continue;
      }
      const auto name = std::string(controller_list[i]["name"]);
      auto joint_list = controller_list[i][CONFIG_JOINTS_PARAM];

      if (joint_list.getType() != XmlRpc::XmlRpcValue::TypeArray)
      {
        ROS_ERROR_STREAM_NAMED(CONTROLLER_NAME,
                               "The list of joints for controller " << name << " is not specified as an array");
        continue;
      }

      std::vector<std::string> joint_names;
      joint_names.reserve(joint_list.size());

      for (int j = 0; j < joint_list.size(); ++j)
      {
        joint_names.emplace_back(std::string(joint_list[j]));
      }

      controllers_[name].reset(new UnityMoveitManagerHandler(name, node_handle_, joint_names));
    }
  }

  ~UnityMoveitManager() override = default;

  moveit_controller_manager::MoveItControllerHandlePtr getControllerHandle(const std::string& name) override
  {
    std::map<std::string, UnityMoveitManagerHandler::Ptr>::const_iterator it = controllers_.find(name);
    if (it != controllers_.end())
    {
      return it->second;
    }
    else
    {
      ROS_FATAL_STREAM("No such controller: " << name);
    }

    return moveit_controller_manager::MoveItControllerHandlePtr();
  }

  /*
   * Get the list of controller names.
   */
  void getControllersList(std::vector<std::string>& names) override
  {
    for (auto it = controllers_.begin(); it != controllers_.end(); ++it)
    {
      names.push_back(it->first);
    }

    ROS_INFO_STREAM("Returned " << names.size() << " controllers in list");
  }

  /*
   * This plugin assumes that all controllers are already active -- and if they are not, well, it has no way to deal
   * with it anyways!
   */
  void getActiveControllers(std::vector<std::string>& names) override
  {
    getControllersList(names);
  }

  /*
   * Controller must be loaded to be active, see comment above about active controllers...
   */
  virtual void getLoadedControllers(std::vector<std::string>& names)
  {
    getControllersList(names);
  }

  /**
    * Get the list of joints that a controller can control.
    *
    * @param name Name of the controller
    */
  void getControllerJoints(const std::string& name, std::vector<std::string>& joints) override
  {
    auto controller_handle = getControllerHandle(name);
    joints = std::static_pointer_cast<UnityMoveitManagerHandler>(controller_handle)->getJoints();
  }

  /*
   * Controllers are all active and default.
   */
  moveit_controller_manager::MoveItControllerManager::ControllerState
  getControllerState(const std::string& /*name*/) override
  {
    moveit_controller_manager::MoveItControllerManager::ControllerState state;
    state.active_ = true;
    state.default_ = true;
    return state; // Always active
  }

  /* Cannot switch our controllers */
  bool switchControllers(const std::vector<std::string>& /*activate*/,
                         const std::vector<std::string>& /*deactivate*/) override
  {
    ROS_ERROR_STREAM("Cannot switch Unity controller");
    return false;
  }

protected:
  ros::NodeHandle node_handle_;
  std::map<std::string, UnityMoveitManagerHandler::Ptr> controllers_;

  // Configuration parameter names
  static constexpr const char* CONFIG_JOINTS_PARAM = "joints";
  static constexpr const char* CONFIG_CONTROLLER_LIST_PARAM = "controller_list";
  static constexpr const char* CONFIG_NAME_PARAM = "name";

  static constexpr const char* CONTROLLER_NAME = "UnityMoveItManager";
};

}  // end namespace unity_moveit_manager

PLUGINLIB_EXPORT_CLASS(unity_moveit_manager::UnityMoveitManager,
                       moveit_controller_manager::MoveItControllerManager);