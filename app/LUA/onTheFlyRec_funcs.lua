
function speak(port, str)
   local wb = port:prepare()
    wb:clear()
    wb:addString(str)
    port:write()
    yarp.delay(1.0)
end

----------------------------------
-- functions - onTheFlyRec      --
----------------------------------
function onTheFlyRec_gazeLook(port)
    local wb = yarp.Bottle()
    wb:clear()
    wb:addString("look")
    wb:addString("0.0")
    wb:addString("10.0")
    port:write(wb)
end

function onTheFlyRec_gazeTrackBlob(port)
	local wb = yarp.Bottle()
	wb:clear()
    wb:addString("track-blob")
    port:write(wb)
end

function onTheFlyRec_gazeTrackFace(port)
    local wb = yarp.Bottle()
    wb:clear()
    wb:addString("track-face")
    port:write(wb)
end

function onTheFlyRec_train(port, objName, is_face)
	local wb = yarp.Bottle()
	local reply = yarp.Bottle()

    print ("bool is ", is_face)

	wb:clear()
    wb:addString("train")
	wb:addString(objName)

    if is_face==true then
        wb:addInt(1)
    else
        wb:addInt(0)
    end

    port:write(wb,reply)
	return reply:get(0):asString()
end

function onTheFlyRec_recognize(port, is_face)
	local wb = yarp.Bottle()
	local reply = yarp.Bottle()
	wb:clear()
    wb:addString("what")

    if is_face==true then
        wb:addInt(1)
    else
        wb:addInt(0)
    end
    port:write(wb,reply)
	return reply:get(0):asString()
end

function onTheFlyRec_forget(port, objName)
	local wb = yarp.Bottle()
	local reply = yarp.Bottle()
	wb:clear()
    wb:addString("forget")
	wb:addString(objName)
    port:write(wb,reply)
	return reply:get(0):asString()
end


----------------------------------
-- functions SPEECH             --
----------------------------------

function SM_RGM_Expand(port, vocab, word)
    local wb = yarp.Bottle()
	local reply = yarp.Bottle()
    wb:clear()
    wb:addString("RGM")
	wb:addString("vocabulory")
	wb:addString("add")
	wb:addString(vocab)
	wb:addString(word)
    port:write(wb,reply)
	--print(reply:get(1):asString()
	return reply:get(1):asString()
end

function SM_Expand_asyncrecog(port, gram)
	local wb = yarp.Bottle()
	local reply = yarp.Bottle()
	wb:clear()
    wb:addString("asyncrecog")
	wb:addString("addGrammar")
	wb:addString(gram)
    port:write(wb,reply)
end

function SM_Reco_Grammar(port, gram)
	local wb = yarp.Bottle()
	local reply = yarp.Bottle()
	wb:clear()
    wb:addString("recog")
	wb:addString("grammarSimple")
	wb:addString(gram)
    port:write(wb,reply)
	return reply
end

function SM_RGM_Expand_Auto(port, vocab)
	local wb = yarp.Bottle()
	local reply = yarp.Bottle()
	wb:clear()
    wb:addString("RGM")
	wb:addString("vocabulory")
	wb:addString("addAuto")
	wb:addString(vocab)
    port:write(wb,reply)
	return reply:get(1):asString()
end

--[[
proc SM_Reco_Grammar { gram } {

	bottle clear
	bottle addString "recog"
	bottle addString "grammarSimple"
	bottle addString $gram
	SpeechManagerPort write bottle reply
	puts "Received from SpeechManager : [reply toString] "
	set wordsList ""
	for { set i 1 } { $i< [reply size] } {incr i 2} {
		set wordsList [lappend wordsList [ [reply get $i] toString] ]
	}
	return $wordsList
}
]]--
