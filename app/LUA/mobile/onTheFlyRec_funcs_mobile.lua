
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

function onTheFlyRec_observe(port, objName)
	local wb = yarp.Bottle()
	local reply = yarp.Bottle()
	wb:clear()
    print("in observe ")
    wb:addString("observe")
    wb:addString(objName)
    port:write(wb,reply)
    print("received observe REPLY: ", reply:get(0):asString() )
    
    --[[wb:clear()
    wb:addString("train")	
    port:write(wb,reply)
    print("received train REPLY: ", reply:get(0):asString() )]]--
	return reply:get(0):asString()
end

function onTheFlyRec_train(port)
    local wb = yarp.Bottle()
	local reply = yarp.Bottle()    
    wb:clear()
    wb:addString("train")	
    port:write(wb,reply)
    print("received train REPLY: ", reply:get(0):asString() )
	return reply:get(0):asString()
end

function onTheFlyRec_recognize(port)
	local wb = yarp.Bottle()
	local reply = yarp.Bottle()
	wb:clear()
    wb:addString("classify")
    port:write(wb,reply)
	return reply:get(0):asString()
end

function onTheFlyRec_mode(port, userName)
	local wb = yarp.Bottle()
	local reply = yarp.Bottle()
	wb:clear()
	wb:addString(userName)
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

function Receive_Speech(port)
    local str = yarp.Bottle()
    local reply = yarp.Bottle()
    str = port:read(false)
    
    if str == nill or str:size() < 1 then
        --print ("null ... Receive_Speech got nothing")
    else

        print ("Initial is: ", str:toString())
        print ("size is: ", str:get(0):asList():size())
        
        reply:clear()
        for i=0,str:get(0):asList():size()-1,1
        do 
            inString = str:get(0):asList():get(i):asList():get(0):asString()
            print ("initial word is : ", inString)
            if (inString == "verb") then
                verb = str:get(0):asList():get(i):asList():get(1):asString()
                reply:addString(verb)
            end
            
            if (inString == "object") then
                verb = str:get(0):asList():get(i):asList():get(1):asString()
                reply:addString(verb)
            end
            
        end

        print ("reply is: ", reply:toString())

    end

    return reply
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
