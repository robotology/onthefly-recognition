#!/usr/bin/lua

require("rfsm")
require("yarp")

require("onTheFlyRec_funcs")

yarp.Network()

-------
shouldExit = false

-- initilization
ispeak_port = yarp.BufferedPortBottle()
speechRecog_port = yarp.Port()
onTheFlyRec_port = yarp.Port()

-- defining objects and actions vocabularies
objects = {"octopus", "box", "toy", "turtle"}
user = {"robot", "human"}

-- defining speech grammar for Menu

grammar = "Mode #User | Train #Object | Let's recognize | Forget all objects  | See you soon "

-- load state machine model and initalize it
fsm_model = rfsm.load("./onTheFlyRec_root_fsm.lua")
fsm = rfsm.init(fsm_model)
rfsm.run(fsm)

repeat
    rfsm.run(fsm)
    yarp.Time_delay(0.1)
until shouldExit ~= false

print("finishing")
-- Deinitialize yarp network
yarp.Network_fini()
