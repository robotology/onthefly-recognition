#!/usr/bin/lua

require("rfsm")
require("yarp")

yarp.Network()

-------
shouldExit = false

-- initialization
ispeak_port = yarp.BufferedPortBottle()
speechRecog_port = yarp.Port()
onTheFlyRec_port = yarp.Port()
onTheFlyRec_track = yarp.Port()

-- defining objects and actions vocabularies
objects = {"phone", "toy", "wallet", "bottle", "badge", "book", "keys", "hand", "face", "cup", "glasses", "bag", "male", "female", "child", "scarf", "shoe", "person", "pen", "tissue", "object", "tablet", "charger", "ticket"}
user = {"robot", "human"}

-- defining speech grammar for Menu

--grammar = "Mode #User | Train #Object | Let recognize | Forget all objects  | See you soon | I will teach you a new object"

grammar = "This is a #Object | What is this | See you soon | Forget all objects | Forget the #Object | Who is this | Let me introduce you to #Object"

-- load state machine model and initalize it
rf = yarp.ResourceFinder()
rf:setVerbose()
rf:setDefaultContext("onthefly-recognition/LUA")
rf:configure(arg)
fsm_file = rf:findFile("onTheFlyRec_root_fsm_laptop.lua")
fsm_model = rfsm.load(fsm_file)
fsm = rfsm.init(fsm_model)

repeat
    rfsm.run(fsm)
    yarp.Time_delay(0.1)
until shouldExit ~= false

print("finishing")
-- Deinitialize yarp network
yarp.Network_fini()
