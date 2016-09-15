
event_table = {
    See         = "e_exit",
    Look        = "e_observe",
    Train       = "e_train",
    Recognize   = "e_recognize",
    Forget      = "e_forget",
    Mode        = "e_mode",
    I           = "e_i_will",
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

                result = Receive_Speech(speechRecog_port)
                local cmd
                if result:size() > 0 then
                    print("received REPLY: ", result:toString() )
                    --print("size is: ", result:get(0):asList():size() )
                    cmd = result:get(0):asString()
                    print("cmd is:", cmd)
                else
                    cmd =  "waiting" -- do nothing
                end

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
            speak(ispeak_port, "training ")
            local b = onTheFlyRec_train(onTheFlyRec_port)
        end
    },

    SUB_OBSERVE = rfsm.state{
        entry=function()
            local obj= result:get(1):asString()
            speak(ispeak_port, "Ok, I will examine the " .. obj)
            local b = onTheFlyRec_observe(onTheFlyRec_port, obj)
        end
    },

    SUB_MODE = rfsm.state{
        entry=function()
            local user = result:get(3):asString()
            local b = onTheFlyRec_mode(onTheFlyRec_port, user)
        end
    },

    ----------------------------------
    -- state SUB_LET             --
    ----------------------------------

    SUB_LET = rfsm.state{
        entry=function()
            print ("in recognize ")
            speak(ispeak_port, "Ok, I will try to recognize what you will show me" )
            local b = onTheFlyRec_recognize(onTheFlyRec_port)
        end
    },

    ----------------------------------
    -- state SUB_FORGET             --
    ----------------------------------

    SUB_FORGET = rfsm.state{
        entry=function()
            local obj
            obj= result:get(1):asString()
            print ("in forget ")
            speak(ispeak_port, "Forgetting")
            if string.len(obj) > 1 then
                print ("single object ", obj)
                obj= result:get(1):asString()
            else
                obj="all"
            end
            local b = onTheFlyRec_forget(onTheFlyRec_port, obj)
        end
    },

    ----------------------------------
    -- state SUB_TEACH             --
    ----------------------------------

    SUB_TEACH_OBJ = rfsm.state{
        entry=function()
            print("in SUB_TEACH_OBJ")
           local obj
            obj= result:get(1):asString()
            print("done with name ", obj)
            speak(ispeak_port, "Ok, I will learn about the" .. obj)
            local b = onTheFlyRec_train(onTheFlyRec_port, obj)
       end
   },

    ----------------------------------
    -- state transitions            --
    ----------------------------------

    rfsm.trans{ src='initial', tgt='SUB_MENU'},
    rfsm.transition { src='SUB_MENU', tgt='SUB_EXIT', events={ 'e_exit' } },

    rfsm.transition { src='SUB_MENU', tgt='SUB_OBSERVE', events={ 'e_observe' } },
    rfsm.transition { src='SUB_OBSERVE', tgt='SUB_MENU', events={ 'e_done' } },

    rfsm.transition { src='SUB_MENU', tgt='SUB_TRAIN', events={ 'e_train' } },
    rfsm.transition { src='SUB_TRAIN', tgt='SUB_MENU', events={ 'e_done' } },

    rfsm.transition { src='SUB_MENU', tgt='SUB_LET', events={ 'e_recognize' } },
    rfsm.transition { src='SUB_LET', tgt='SUB_MENU', events={ 'e_done' } },

    rfsm.transition { src='SUB_MENU', tgt='SUB_MODE', events={ 'e_mode' } },
    rfsm.transition { src='SUB_MODE', tgt='SUB_MENU', events={ 'e_done' } },

    rfsm.transition { src='SUB_MENU', tgt='SUB_FORGET', events={ 'e_forget' } },
    rfsm.transition { src='SUB_FORGET', tgt='SUB_MENU', events={ 'e_done' } },

    rfsm.transition { src='SUB_MENU', tgt='SUB_TEACH_OBJ', events={ 'e_i_will' } },
    rfsm.transition { src='SUB_TEACH_OBJ', tgt='SUB_MENU', events={ 'e_done' } },

}
