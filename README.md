## Intro

This is a moveit! plugin which sends the trajectory to Unity using https://github.com/Unity-Technologies/Unity-Robotics-Hub which is a library developed to support accurate robotics simulation for Unity. The package should be configured as a ```moveit_controller_manager```. The package simply publishes the calculated trajectory to a custom topic which is read by the Unity TCP server designed (also part of this package)

### Installation
Simply checkout this package

```git clone git@github.com:szandara/unity_moveit_manager.git``` in your catkin workspace and build it.

### Example
A working example of the working solution can be found in https://github.com/szandara/unity_reachy_tutorial which is part of the tutorial https://dev.to/szandara/robotics-on-wsl2-using-ros-docker-and-unity-3d-part-i-3752
