# Copyright: 2010 RobotCub Consortium
# Authors: Sean Ryan Fanello, Carlo Ciliberto
# CopyPolicy: Released under the terms of the GNU GPL v2.0.

set(PROJECTNAME onthefly-recognition)

file(GLOB scripts  ${CMAKE_CURRENT_SOURCE_DIR}/scripts/*.template ${CMAKE_CURRENT_SOURCE_DIR}/scripts/*.xml)
set(lua_fsm        ${CMAKE_CURRENT_SOURCE_DIR}/LUA/onTheFlyRec_funcs.lua
                   ${CMAKE_CURRENT_SOURCE_DIR}/LUA/onTheFlyRec_interact_fsm.lua
                   ${CMAKE_CURRENT_SOURCE_DIR}/LUA/onTheFlyRec_root_fsm.lua
                   ${CMAKE_CURRENT_SOURCE_DIR}/LUA/mobile/onTheFlyRec_funcs_mobile.lua
                   ${CMAKE_CURRENT_SOURCE_DIR}/LUA/mobile/onTheFlyRec_interact_fsm_mobile.lua
                   ${CMAKE_CURRENT_SOURCE_DIR}/LUA/mobile/onTheFlyRec_root_fsm_mobile.lua)
set(lua_gaze       ${CMAKE_CURRENT_SOURCE_DIR}/LUA/onTheFlyRec_gaze.lua)
set(lua_bin        ${CMAKE_CURRENT_SOURCE_DIR}/LUA/onTheFlyRec_main.lua)
set(lua_bin_mb     ${CMAKE_CURRENT_SOURCE_DIR}/LUA/mobile/onTheFlyRec_main_mobile.lua)

yarp_install(FILES    ${scripts} DESTINATION ${ICUBCONTRIB_APPLICATIONS_TEMPLATES_INSTALL_DIR})
yarp_install(PROGRAMS ${lua_fsm} DESTINATION ${ICUBCONTRIB_CONTEXTS_INSTALL_DIR}/${PROJECTNAME}/LUA)

# We cannot go for "yarp_install(PROGRAMS ${lua_bin} DESTINATION bin)", since on Windows, only
# at build time (i.e. not during installation), we have to handle the Debug|Release subfolders.
# Requirements: the cmake "-E copy" command does not change file permissions, hence the file needs
# to be stored in the versioning database with *executable* rights.
add_custom_target(copy_lua_gaze_in_build ALL)
add_custom_command(TARGET copy_lua_gaze_in_build POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy ${lua_gaze} ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}
                   COMMENT "Copying ${lua_gaze} to ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}/")
install(PROGRAMS ${lua_gaze} DESTINATION bin)
     
add_custom_target(copy_lua_bin_in_build ALL)
add_custom_command(TARGET copy_lua_bin_in_build POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy ${lua_bin} ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}
                   COMMENT "Copying ${lua_bin} to ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}/")
install(PROGRAMS ${lua_bin} DESTINATION bin)
     
add_custom_target(copy_lua_bin_mb_in_build ALL)
add_custom_command(TARGET copy_lua_bin_mb_in_build POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy ${lua_bin_mb} ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}
                   COMMENT "Copying ${lua_bin_mb} to ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}/")
install(PROGRAMS ${lua_bin_mb} DESTINATION bin)
     
