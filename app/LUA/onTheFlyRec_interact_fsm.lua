
event_table = {
	See			= "e_exit",
	Train       = "e_train",
	Recognize   = "e_recognize",
	Forget    	= "e_forget",
	Mode    	= "e_mode",
	}
	
interact_fsm = rfsm.state{

	----------------------------------
	-- state SUB_MENU               --
	----------------------------------
	SUB_MENU = rfsm.state{
		entry=function()
			print("in substate MENU : waiting for speech command!")
		end,

		doo = function()
            while true do
				speak(ispeak_port, "What should I do?")
				result = SM_Reco_Grammar(speechRecog_port, grammar)
				print("received REPLY: ", result:toString() )
				local cmd =  result:get(1):asString():c_str()
				rfsm.send_events(fsm, event_table[cmd])
                rfsm.yield(true)
            end
		end
	},

	----------------------------------
	-- states                       --
	----------------------------------

	SUB_EXIT = rfsm.state{
		entry=function()
			speak(ispeak_port, "Ok, bye bye")
			rfsm.send_events(fsm, 'e_menu_done')
		end
	},

	SUB_TRAIN = rfsm.state{
		entry=function()
			local obj = result:get(2):asString():c_str()
			local b = onTheFlyRec_train(onTheFlyRec_port, obj)
		end
	},

	SUB_MODE = rfsm.state{
		entry=function()
			local user = result:get(2):asString():c_str()
			local b = onTheFlyRec_mode(onTheFlyRec_port, user)
		end
	},
	
	SUB_RECOGNIZE = rfsm.state{
		entry=function()
			local b = onTheFlyRec_recognize(onTheFlyRec_port)
		end
	},

	SUB_FORGET = rfsm.state{
		entry=function()
			--local obj = result:get(3):asString():c_str()
			--local b = onTheFlyRec_forget(onTheFlyRec_port, obj)
			--not sure how onTheFlyRec deals with individual objects. All will get deleted
			local b = onTheFlyRec_forget(onTheFlyRec_port)
		end
	},

	----------------------------------
	-- state transitions            --
	----------------------------------
	

	rfsm.trans{ src='initial', tgt='SUB_MENU'},
	rfsm.transition { src='SUB_MENU', tgt='SUB_EXIT', events={ 'e_exit' } },

	rfsm.transition { src='SUB_MENU', tgt='SUB_TRAIN', events={ 'e_train' } },
	rfsm.transition { src='SUB_TRAIN', tgt='SUB_MENU', events={ 'e_done' } },

	rfsm.transition { src='SUB_MENU', tgt='SUB_RECOGNIZE', events={ 'e_recognize' } },
	rfsm.transition { src='SUB_RECOGNIZE', tgt='SUB_MENU', events={ 'e_done' } },

	rfsm.transition { src='SUB_MENU', tgt='SUB_MODE', events={ 'e_mode' } },
	rfsm.transition { src='SUB_MODE', tgt='SUB_MENU', events={ 'e_done' } },

	rfsm.transition { src='SUB_MENU', tgt='SUB_FORGET', events={ 'e_forget' } },
	rfsm.transition { src='SUB_FORGET', tgt='SUB_MENU', events={ 'e_done' } },

}
