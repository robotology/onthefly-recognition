#!/usr/bin/lua

require("rfsm")
require("yarp")

yarp.Network()

-------
shouldExit = false

-- initialization
ispeak_port = yarp.BufferedPortBottle()
speechRecog_port = yarp.BufferedPortBottle()
onTheFlyRec_port = yarp.Port()

-- defining objects and actions vocabularies
objects = {"octopus", "box", "toy", "turtle"}
user = {"robot", "human"}

-- defining speech grammar for Menu

grammar = "Mode #User | Train #Object | Recognize | Forget all objects  | See you soon | I will teach you a new object"

-- load state machine model and initalize it
rf = yarp.ResourceFinder()
rf:setVerbose()
rf:setDefaultContext("onTheFlyRecognition/LUA")
rf:configure(arg)
fsm_file = rf:findFile("onTheFlyRec_root_fsm_mobile.lua")
fsm_model = rfsm.load(fsm_file)
fsm = rfsm.init(fsm_model)

repeat
    rfsm.run(fsm)
    yarp.Time_delay(0.1)
until shouldExit ~= false

print("finishing")
-- Deinitialize yarp network
yarp.Network_fini()
