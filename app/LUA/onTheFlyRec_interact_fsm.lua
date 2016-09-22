
event_table = {
    See         = "e_exit",
    This        = "e_train",
    What        = "e_recog",
    Forget      = "e_forget",
    Let         = "e_introduce",
    Who         = "e_who",
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
                -- speak(ispeak_port, "What should I do?")
                result = SM_Reco_Grammar(speechRecog_port, grammar)
                print("received REPLY: ", result:toString() )
                local cmd = result:get(1):asString()
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
            local is_face = false
            local obj = result:get(7):asString()
            print ("object is ", obj)
            print ("bool is ", is_face)
            local b = onTheFlyRec_train(onTheFlyRec_port, obj, is_face)
        end
    },

    SUB_RECOG = rfsm.state{
        entry=function()
            print ("in recognition mode ")
            local is_face = false
            print ("bool is ", is_face)
            local b = onTheFlyRec_recognize(onTheFlyRec_port, is_face)
        end
    },

    SUB_LET = rfsm.state{
        entry=function()
            onTheFlyRec_gazeLook(onTheFlyRec_track)
            yarp.Time_delay(1)
            onTheFlyRec_gazeTrackFace(onTheFlyRec_track)
            local obj = result:get(11):asString()
            print ("in introduction mode ")
            yarp.Time_delay(2)
            print ("person is ", obj)
            local  is_face = true
            print ("bool is ", is_face)
            local b = onTheFlyRec_train(onTheFlyRec_port, obj, is_face)
            print ("done onTheFlyRec_train ",  b)
            print ("delaying to continue tracking faces.... ")
            yarp.Time_delay(15)
            print ("returning to blob tracking.... ")
            onTheFlyRec_gazeTrackBlob(onTheFlyRec_track)
        end
    },
    
    SUB_WHO = rfsm.state{
        entry=function()
            onTheFlyRec_gazeLook(onTheFlyRec_track)
            yarp.Time_delay(1)
            onTheFlyRec_gazeTrackFace(onTheFlyRec_track)
            print ("in person request mode ")
            yarp.Time_delay(2)
            local  is_face = true
            print ("bool is ", is_face)
            local b = onTheFlyRec_recognize(onTheFlyRec_port, is_face)
            print ("done onTheFlyRec_recognize ",  b)
            print ("delaying to continue tracking faces.... ")
            yarp.Time_delay(4)
            print ("returning to blob tracking.... ")
            onTheFlyRec_gazeTrackBlob(onTheFlyRec_track)
        end
    },

    SUB_FORGET = rfsm.state{
        entry=function()
            local obj = result:get(5):asString()
            if  obj == "objects" then
                print ("forgetting all objects")
                obj="all"
            else
                print ("forgetting single object", obj)
            end
            local b = onTheFlyRec_forget(onTheFlyRec_port, obj)
            
        end
    },

    ----------------------------------
    -- state transitions            --
    ----------------------------------

    rfsm.trans{ src='initial', tgt='SUB_MENU'},
    rfsm.transition { src='SUB_MENU', tgt='SUB_EXIT', events={ 'e_exit' } },

    rfsm.transition { src='SUB_MENU', tgt='SUB_TRAIN', events={ 'e_train' } },
    rfsm.transition { src='SUB_TRAIN', tgt='SUB_MENU', events={ 'e_done' } },

    rfsm.transition { src='SUB_MENU', tgt='SUB_RECOG', events={ 'e_recog' } },
    rfsm.transition { src='SUB_RECOG', tgt='SUB_MENU', events={ 'e_done' } },

    rfsm.transition { src='SUB_MENU', tgt='SUB_FORGET', events={ 'e_forget' } },
    rfsm.transition { src='SUB_FORGET', tgt='SUB_MENU', events={ 'e_done' } },

    rfsm.transition { src='SUB_MENU', tgt='SUB_LET', events={ 'e_introduce' } },
    rfsm.transition { src='SUB_LET', tgt='SUB_MENU', events={ 'e_done' } },

    rfsm.transition { src='SUB_MENU', tgt='SUB_WHO', events={ 'e_who' } },
    rfsm.transition { src='SUB_WHO', tgt='SUB_MENU', events={ 'e_done' } },
}
