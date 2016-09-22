#!/usr/bin/lua

-- Copyright: (C) 2016 iCub Facility - Istituto Italiano di Tecnologia (IIT)
-- Authors: Ugo Pattacini <ugo.pattacini@iit.it>
-- Copy Policy: Released under the terms of the LGPLv2.1 or later, see LGPL.TXT

-- Dependencies
--
-- To install posix.signal do:
-- sudo apt-get install luarocks
-- sudo luarocks install luaposix

-- Command line parameters
-- --look-around to start in looking around mode
-- --track-blob to start in tracking blob mode
-- --track-face to start in tracking face mode
-- --flip to flip images left-right
-- --w <int> to specify image width (320 by default)
-- --h <int> to specify image height (240 by default)
-- --max-track-area to specify the max blob area to track (10000 by default)

-- Available commands to be sent to /onTheFlyRec/gaze
--
-- #1: look azi ele
-- #2: look-around
-- #3: look-around azi ele
-- #4: set-delta azi-delta ele-delta
-- #5: track-blob
-- #6: track-face
-- #7: stop
-- #8: idle
-- #9: quit

local signal = require("posix.signal")
require("yarp")

rf = yarp.ResourceFinder()
rf:setVerbose(false)
rf:configure(arg)

if rf:check("look-around") then
    state = "init"
elseif rf:check("track-blob") then
    state = "track-blob"
elseif rf:check("track-face") then
    state = "track-face"
else
    state = "idle"
end

interrupting = false
signal.signal(signal.SIGINT, function(signum)
    interrupting = true
end)

signal.signal(signal.SIGTERM, function(signum)
    interrupting = true
end)

yarp.Network()

port_cmd = yarp.BufferedPortBottle()
port_blob = yarp.BufferedPortBottle()
port_face = yarp.BufferedPortBottle()
port_gaze_tx = yarp.BufferedPortProperty()
port_gaze_rx = yarp.BufferedPortProperty()
port_gaze_rpc = yarp.RpcClient()
port_gaze_train_blob = yarp.BufferedPortBottle()
port_gaze_train_roi = yarp.BufferedPortBottle()

port_cmd:open("/onTheFlyRec/gaze")
port_blob:open("/onTheFlyRec/gaze/blob")
port_face:open("/onTheFlyRec/gaze/face")
port_gaze_tx:open("/onTheFlyRec/gaze/tx")
port_gaze_rx:open("/onTheFlyRec/gaze/rx")
port_gaze_rpc:open("/onTheFlyRec/gaze/rpc")
port_gaze_train_blob:open("/onTheFlyRec/gaze/train/blob")
port_gaze_train_roi:open("/onTheFlyRec/gaze/train/roi")

function train(tl_x,tl_y,br_x,br_y)
    local blob = port_gaze_train_blob:prepare()
    blob:clear()
    local val1 = blob:addList()
    val1:addDouble((tl_x+br_x)/2)
    val1:addDouble((tl_y+br_y)/2)
    val1:addDouble((br_x-tl_x)*(br_y-tl_y))
    port_gaze_train_blob:write()

    local roi = port_gaze_train_roi:prepare()
    roi:clear()
    local val2 = roi:addList()
    val2:addDouble(tl_x)
    val2:addDouble(tl_y)
    val2:addDouble(br_x)
    val2:addDouble(br_y)
    port_gaze_train_roi:write()
end

function look_at_angle(azi,ele)
    local tx = port_gaze_tx:prepare()
    tx:clear()
    tx:put("control-frame","gaze")
    tx:put("target-type","angular")

    local location = yarp.Bottle()
    local val = location:addList()
    val:addDouble(azi)
    val:addDouble(ele)
    tx:put("target-location",location:get(0))
    port_gaze_tx:write()

    print("look_at_angle:", tx:toString())
end

function look_at_pixel(mode,px,py)
    local tx = port_gaze_tx:prepare()
    tx:clear()
    tx:put("control-frame","depth")
    tx:put("target-type","image")
    tx:put("image","depth")

    local location = yarp.Bottle()
    local val = location:addList()
    val:addDouble(px)
    val:addDouble(py)
    tx:put("target-location",location:get(0))
    port_gaze_tx:write()

    print("look_at_pixel:", mode, tx:toString())
end


while not interrupting and port_gaze_rx:getInputCount() == 0 do
    print("checking yarp connection...")
    yarp.Time_delay(1.0)
end


flip = rf:check("flip")
w = rf:check("w",yarp.Value(320)):asInt()
h = rf:check("h",yarp.Value(240)):asInt()
max_track_area = rf:check("max-track-area",yarp.Value(10000)):asInt()

azi = 0.0
ele = 0.0
azi_delta = 5
ele_delta = 5
t0 = yarp.Time_now()


while state ~= "quit" and not interrupting do

    local cmd = port_cmd:read(false)
    if cmd ~= nil then
        local cmd_rx = cmd:get(0):asString()

        if cmd_rx == "look-around" or cmd_rx == "look" or
           cmd_rx == "track-blob" or cmd_rx == "track-face" or
           cmd_rx == "stop" or cmd_rx == "idle" or
           cmd_rx == "quit" then

            state = cmd_rx

            if state == "look" then
                azi = cmd:get(1):asDouble()
                ele = cmd:get(2):asDouble()
                print("received: look ", azi,ele)
            elseif state == "look-around" then
                if cmd:size()>1 then
                    azi = cmd:get(1):asDouble()
                    ele = cmd:get(2):asDouble()
                else
                    local fp = port_gaze_rx:read(true)
                    local ang = fp:find("angular"):asList()
                    azi = ang:get(0):asDouble()
                    ele = ang:get(1):asDouble()
                end
                print("received: look around ", azi,ele)
            end
        elseif cmd_rx == "set-delta" then
            azi_delta = cmd:get(1):asDouble()
            ele_delta = cmd:get(2):asDouble()
            print("received: set delta ", azi_delta,ele_delta)
        else
            print("warning: unrecognized command")
        end
    end

    if state == "init" then
        local fp = port_gaze_rx:read(true)
        local ang = fp:find("angular"):asList()
        azi = ang:get(0):asDouble()
        ele = ang:get(1):asDouble()
        state = "look-around"
    elseif state == "look" then
        look_at_angle(azi,ele)
        state = "idle"
    elseif state == "look-around" then
        local t1 = yarp.Time_now()
        if t1-t0 > math.random(2,4) then
            local azi_new = azi + math.random(-azi_delta,azi_delta)
            local ele_new = ele + math.random(-ele_delta,ele_delta)
            look_at_angle(azi_new,ele_new)
            t0 = t1
        end
    elseif state == "track-blob" then
        local t1 = yarp.Time_now()
        local blobs = port_blob:read(false)
        if blobs ~= nil then
           local blob = blobs:get(0):asList()
           local tl_x = blob:get(0):asInt()
           local tl_y = blob:get(1):asInt()
           local br_x = blob:get(2):asInt()
           local br_y = blob:get(3):asInt()
           local cx = (tl_x+br_x)/2
           local cy = (tl_y+br_y)/2
           local area = (br_x-tl_x)*(br_y-tl_y)

           train(tl_x,tl_y,br_x,br_y)

           if flip == true then
              cx = w-cx
           end

           if area < max_track_area then
              look_at_pixel(state,cx,cy)
              t0 = t1
           end
        end

        if t1-t0 > 5 then
           look_at_angle(math.random(-10,10),0)
           t0 = t1
        end

        yarp.Time_delay(0.01)
    elseif state == "track-face" then
        local t1 = yarp.Time_now()
        local faces = port_face:read(false)
        if faces ~= nil then
           local face = faces:get(0):asList()
           local tl_x = face:get(0):asInt()
           local tl_y = face:get(1):asInt()
           local br_x = face:get(2):asInt()
           local br_y = face:get(3):asInt()
           local cx = (tl_x+br_x)/2
           local cy = (tl_y+br_y)/2

           train(tl_x,tl_y,br_x,br_y)

           if flip == true then
               cx = w-cx
           end

           look_at_pixel(state,cx,cy)
           t0 = t1
        end

        if t1-t0 > 5 then
           look_at_angle(math.random(-10,10),0)
           t0 = t1
        end

        yarp.Time_delay(0.01)
    elseif state == "stop" then
        local cmd = yarp.Bottle()
        local rep = yarp.Bottle()

        cmd:addString("stop")
        port_gaze_rpc:write(cmd,rep)

        print("just stopped!")
        state = "idle"
    elseif state == "idle" then
        yarp.Time_delay(0.1)
    end
end

if port_gaze_rx:getInputCount() > 0 then
    look_at_angle(0,0)
end

port_cmd:close()
port_blob:close()
port_face:close()
port_gaze_tx:close()
port_gaze_rx:close()
port_gaze_rpc:close()
port_gaze_train_blob:close()
port_gaze_train_roi:close()

yarp.Network_fini()
