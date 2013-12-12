# Copyright: 2010 RobotCub Consortium
# Authors: Sean Ryan Fanello, Carlo Ciliberto
# CopyPolicy: Released under the terms of the GNU GPL v2.0.

set(PROJECTNAME onTheFlyRecognition)

file(GLOB scripts  ${CMAKE_CURRENT_SOURCE_DIR}/scripts/*.template ${CMAKE_CURRENT_SOURCE_DIR}/scripts/*.xml)
set(lua_fsm        ${CMAKE_CURRENT_SOURCE_DIR}/LUA/onTheFly_funcs.lua
                   ${CMAKE_CURRENT_SOURCE_DIR}/LUA/onTheFly_interact_fsm.lua
                   ${CMAKE_CURRENT_SOURCE_DIR}/LUA/onTheFly_root_fsm.lua)
set(lua_bin        ${CMAKE_CURRENT_SOURCE_DIR}/LUA/onTheFly_main.lua)

yarp_install(FILES     ${scripts} DESTINATION ${ICUBCONTRIB_APPLICATIONS_TEMPLATES_INSTALL_DIR})
yarp_install(PROGRAMS  ${lua_fsm} DESTINATION ${ICUBCONTRIB_CONTEXTS_INSTALL_DIR}/${PROJECTNAME}/LUA)

add_custom_target(copy_lua_bin ALL)
add_custom_command(TARGET copy_lua_bin POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${lua_bin} ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR})
install(PROGRAMS ${lua_bin} DESTINATION bin)
     