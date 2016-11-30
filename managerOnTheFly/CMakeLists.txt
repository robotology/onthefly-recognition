# Copyright: 2016 RobotCub Consortium
# Author: Giulia Pasquale, Sean Ryan Fanello, Carlo Ciliberto
# CopyPolicy: Released under the terms of the GNU GPL v2.0.

cmake_minimum_required(VERSION 2.8)

set(PROJECTNAME managerOnTheFly)
project(${PROJECTNAME})

find_package(OpenCV REQUIRED)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${YARP_INCLUDE_DIRS})
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/include)

set(folder_header ${folder_header}    include/ManagerThread.h
                                      include/ScorerThread.h
                                      include/definitions.h
                                      include/CropperThread.h )

set(folder_source ${folder_source}    src/ManagerThread.cpp
				      src/ScorerThread.cpp
                                      src/CropperThread.cpp
                                      src/main.cpp )

source_group("Source Files" FILES ${folder_source})
source_group("Header Files" FILES ${folder_header})

add_executable(${PROJECTNAME} ${folder_source} ${folder_header})

target_link_libraries(${PROJECTNAME} ${OpenCV_LIBRARIES} ${YARP_LIBRARIES})

install(TARGETS ${PROJECTNAME} DESTINATION bin)
