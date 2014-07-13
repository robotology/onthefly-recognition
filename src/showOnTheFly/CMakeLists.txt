# Copyright: (C) 2011 Department of Robotics Brain and Cognitive Sciences - Istituto Italiano di Tecnologia
# Authors: Carlo Ciliberto
# CopyPolicy: Released under the terms of the GNU GPL v2.0.

cmake_minimum_required(VERSION 2.6)
set(PROJECTNAME showOnTheFly)
project(${PROJECTNAME})

find_package(OpenCV)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
  
include_directories(${YARP_INCLUDE_DIRS})
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR})

set(folder_source main.cpp)
source_group("Source Files" FILES ${folder_source})
add_executable(${PROJECTNAME} ${folder_source})
target_link_libraries(${PROJECTNAME} ${OpenCV_LIBRARIES} ${YARP_LIBRARIES})
install(TARGETS ${PROJECTNAME} DESTINATION bin)

