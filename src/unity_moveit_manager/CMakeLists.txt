cmake_minimum_required(VERSION 2.8.3)
project(unity_moveit_manager)

include_directories(include)
include_directories(
  SYSTEM
  ${catkin_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
)

find_package(catkin COMPONENTS
  moveit_core
  pluginlib
  roscpp
  REQUIRED)

catkin_package(
  LIBRARIES
  INCLUDE_DIRS
  DEPENDS moveit_core)

include_directories(${catkin_INCLUDE_DIRS})

# Unity controller
add_library(unity_moveit_manager
  src/unity_moveit_manager.cpp)
set_target_properties(unity_moveit_manager PROPERTIES VERSION "${${PROJECT_NAME}_VERSION}")
target_link_libraries(unity_moveit_manager ${catkin_LIBRARIES} ${Boost_LIBRARIES})

#############
## Install ##
#############

# Mark executables and/or libraries for installation
install(
  TARGETS
    unity_moveit_manager
  ARCHIVE DESTINATION
    ${CATKIN_PACKAGE_LIB_DESTINATION}
  LIBRARY DESTINATION
    ${CATKIN_PACKAGE_LIB_DESTINATION}
  RUNTIME DESTINATION
    ${CATKIN_PACKAGE_BIN_DESTINATION}
)

install(FILES unity_moveit_plugin_description.xml
        DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION})