/*
 * Copyright (C) 2016 iCub Facility - Istituto Italiano di Tecnologia
 * Authors: Giulia Pasquale, Carlo Ciliberto
 * emails:  giulia.pasquale@iit.it, carlo.ciliberto@iit.it
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU General Public License, version 2 or any
 * later version published by the Free Software Foundation.
 *
 * A copy of the license can be found at
 * http://www.robotcub.org/icub/license/gpl.txt
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
 */

#include <ManagerThread.h>
#include <definitions.h>

bool ManagerThread::set_state(int _state)
{
    state = _state;
    bool ok = thr_cropper->set_state(state);

    return ok;
}

bool ManagerThread::set_mode(int _mode)
{
    mode =_mode;
    bool ok = thr_cropper->set_mode(mode);

    return ok;
}

bool ManagerThread::set_crop_mode(int _crop_mode)
{
    crop_mode = _crop_mode;
    bool ok = thr_cropper->set_crop_mode(crop_mode);

    return ok;
}

bool ManagerThread::set_human_time_training(double _t)
{
    if (_t>0)
    {
        human_time_training = _t;
        return true;
    }
    else
        return false;
}

bool ManagerThread::speak(string speech)
{
    if (port_out_speech.getOutputCount()>0)
    {
        Bottle b;
        b.addString(speech.c_str());
        port_out_speech.write(b);
        return true;
    }
    return false;
}

bool ManagerThread::store_human(string class_name)
{

    if (!send_doublecmd2rpc_classifier("save", class_name.c_str(), 10))
    {
        std::cout << "Classifier busy for saving!" << std::endl;
        return false;
    }

    Time::delay(human_time_training);

    if (!send_cmd2rpc_classifier("stop", 10))
    {
        std::cout << "Classifier busy for stopping to save scores: please make it stop somehow!" << std::endl;
        return false;
    }

    return true;
}

bool ManagerThread::store_robot(string class_name)
{
    // check if the robot is already holding an object
    Bottle command,reply;
    command.addString("get");
    command.addString("holding");
    port_rpc_are_get.write(command,reply);

    // if the robot is not holding an object then ask the human to give one
    if(reply.get(0).asVocab()!=ACK)
    {
        reply.clear();
        command.clear();
        command.addString("expect");
        command.addString("near");
        command.addString("no_sacc");
        port_rpc_are_cmd.write(command,reply);
        if(reply.size()==0 || reply.get(0).asVocab()!=ACK)
        {
            std::cout << "Cannot set ARE to expect near no_sacc!" << std::endl;
            return false;
        }
    }

    if (!send_doublecmd2rpc_classifier("save", class_name.c_str(), 10))
    {
        std::cout << "Classifier busy for saving!" << std::endl;
        complete_robot();
        return false;
    }

    // perform the exploration of the hand
    reply.clear();
    command.clear();
    command.addString("explore");
    command.addString("hand");
    command.addString("no_sacc");
    port_rpc_are_cmd.write(command,reply);
    if (reply.size()==0 || reply.get(0).asVocab()!=ACK)
    {
        std::cout << "Cannot set are to explore hand no_sacc!" << std::endl;

        if (!send_cmd2rpc_classifier("stop", 10))
        {
            std::cout << "Classifier busy for stopping to save scores: please make it stop somehow!" << std::endl;
        }

        complete_robot();
        return false;
    }

    if (!send_cmd2rpc_classifier("stop", 10))
    {
        std::cout << "Classifier busy for stopping to save scores: please make it stop somehow!" << std::endl;
        complete_robot();
        return false;
    }

    return true;
}


bool ManagerThread::store_robot_tool(string class_name)
{
    // check if the robot is already holding a tool
    cout << "Checking that tool is in hand" << endl;
    Bottle command,reply;
    command.addString("get");
    command.addString("holding");
    port_rpc_are_get.write(command,reply);

    // if the robot is not holding a tool then ask the human to give one
    if(reply.get(0).asVocab()!=ACK)
    {
        cout << "Gimme the tool " << class_name << endl;

        reply.clear();      command.clear();
        command.addString("tato");
        command.addString("right");
        port_rpc_are_cmd.write(command,reply);

        reply.clear();      command.clear();
        command.addString("hand");
        command.addString("close_hand_tool");
        port_rpc_are_cmd.write(command,reply);

        cout << "Waiting until tool is grasped" << endl;
        Time::delay(5.0);
    } else  {
        cout << "Tool " << class_name << " is already in hand" << endl;
    }

    cout << "Moving hand and looking at tool" << endl;
    // perform the exploration of the hand
    reply.clear();
    command.clear();
    command.addString("turnHand");
    port_rpc_o3de.write(command,reply);

    cout << "Waiting till looking at tool" << endl;
    Time::delay(1.0);


    cout << "Saving image in classifier" << endl;
    if (!send_doublecmd2rpc_classifier("save", class_name.c_str(), 10))
    {
        std::cout << "Classifier busy for saving!" << std::endl;
        complete_robot();
        return false;
    }


    cout << "Looking around the tool" << endl;
    for (int i =1; i < 10 ; i++){
        reply.clear();
        command.clear();
        command.addString("lookAround");
        port_rpc_o3de.write(command,reply);        
        cout << "Getting perspective " << i << endl;
        Time::delay(1.0);
    }

    if (!send_cmd2rpc_classifier("stop", 10))
    {
        std::cout << "Classifier busy for stopping to save scores: please make it stop somehow!" << std::endl;
        complete_robot_tool(true);
        return false;
    }

    return true;
}


bool ManagerThread::observe_robot(string &predicted_class)
{
    // check if the robot is already holding an object
    Bottle command,reply;
    command.addString("get");
    command.addString("holding");
    port_rpc_are_get.write(command,reply);

    // if the robot is not holding an object then ask the human to give one
    if(reply.get(0).asVocab()!=ACK)
    {
        reply.clear();
        command.clear();
        command.addString("expect");
        command.addString("near");
        command.addString("no_sacc");
        port_rpc_are_cmd.write(command,reply);
        if(reply.size()==0 || reply.get(0).asVocab()!=ACK)
        {
            std::cout << "Cannot set ARE to expect near no_sacc!" << std::endl;
            return false;
        }
    }

    int old_buffer_size;
    thr_scorer->get_buffer_size(old_buffer_size);

    thr_scorer->set_buffer_size(MAX_BUFFER_SIZE);

    // perform the exploration of the hand
    reply.clear();
    command.clear();
    command.addString("explore");
    command.addString("hand");
    command.addString("no_sacc");
    port_rpc_are_cmd.write(command,reply);
    if (reply.size()==0 || reply.get(0).asVocab()!=ACK)
    {
        std::cout << "Cannot set are to explore hand no_sacc!" << std::endl;

        thr_scorer->set_buffer_size(old_buffer_size);

        complete_robot();

        return false;
    }

    thr_scorer->get_predicted_class(predicted_class);

    thr_scorer->set_buffer_size(old_buffer_size);

    return true;
}



bool ManagerThread::observe_robot_tool(string &predicted_class)
{
    // check if the robot is already holding an object
    Bottle command,reply;
    command.addString("get");
    command.addString("holding");
    port_rpc_are_get.write(command,reply);

    // if the robot is not holding an object then ask the human to give one
    if(reply.get(0).asVocab()!=ACK)
    {
        reply.clear();      command.clear();
        command.addString("tato");
        command.addString("right");
        port_rpc_are_cmd.write(command,reply);

        reply.clear();      command.clear();
        command.addString("hand");
        command.addString("close_hand_tool");
        port_rpc_are_cmd.write(command,reply);

        cout << "Waiting till tool is grasped" << endl;
        Time::delay(2.0);
    }

    int old_buffer_size;
    thr_scorer->get_buffer_size(old_buffer_size);

    thr_scorer->set_buffer_size(MAX_BUFFER_SIZE);

    // perform the exploration of the hand
    reply.clear();
    command.clear();
    command.addString("turnHand");
    port_rpc_o3de.write(command,reply);

    cout << "Waiting till looking at tool" << endl;
    for (int i = 1; i< 5 ; i++){
        Time::delay(1.0);
        cout << "Stabilizing recognition" << endl;
    }

    thr_scorer->get_predicted_class(predicted_class);

    thr_scorer->set_buffer_size(old_buffer_size);

    return true;
}

bool ManagerThread::complete_robot()
{
    // just drop the object
    Bottle command,reply;
    command.addString("give");
    port_rpc_are_cmd.write(command,reply);

    cout << " Object given " << endl;


    command.clear();
    command.addString("home");
    port_rpc_are_cmd.write(command,reply);
    if (reply.size()==0 || reply.get(0).asVocab()!=ACK)
    {
        std::cout << "Cannot set ARE to home!" << std::endl;
        return false;
    }

    return true;
}

bool ManagerThread::complete_robot_tool(bool drop)
{
    // Go home keeping the tool in the hand or not
    Bottle command,reply;
    command.clear();  reply.clear();
    if (drop){
        command.addString("home");
    }else{
        command.addString("home");
        command.addString("head");
        command.addString("arms");
    }
    port_rpc_are_cmd.write(command,reply);

    if (reply.size()==0 || reply.get(0).asVocab()!=ACK)
    {
        std::cout << "Cannot set ARE to home!" << std::endl;
        return false;
    }

    return true;
}

bool ManagerThread::threadInit()
{

    verbose = rf.check("verbose");

    string name = rf.find("name").asString().c_str();

    // rpc ARE
    port_rpc_are.open(("/"+name+"/are/rpc").c_str());
    port_rpc_are_get.open(("/"+name+"/are/get:io").c_str());
    port_rpc_are_cmd.open(("/"+name+"/are/cmd:io").c_str());

    // rpc linearClassifier
    port_rpc_classifier.open(("/"+name+"/classifier:io").c_str());

    // rpc linearClassifier
    port_rpc_o3de.open(("/"+name+"/o3de:io").c_str());


    // out speech
    port_out_speech.open(("/"+name+"/speech:o").c_str());

    thr_cropper = new CropperThread(rf);
    thr_cropper->start();

    thr_scorer = new ScorerThread(rf);
    thr_scorer->start();

    mutex.wait();

    human_time_training = rf.check("human_time_training", Value(15.0)).asDouble();
    recognition_started = false;
    tool_mode = rf.check("tool", Value(false)).asBool();

    set_mode(MODE_HUMAN);
    set_state(STATE_CLASSIFYING);
    set_crop_mode(CROP_MODE_BBDISP);

    mutex.post();

    return true;
}

void ManagerThread::run()
{

    mutex.wait();

    string current_class;

    if (port_rpc_classifier.getOutputCount()==0)
    {
        std::cout << "Please connect a classifier to start!" << std::endl;
        thr_scorer->clear_hist();
        mutex.post();
        return;
    }

    if (recognition_started==false)
    {
        Bottle cmd_classifier,reply_classifier;
        cmd_classifier.addString("recognize");
        port_rpc_classifier.write(cmd_classifier,reply_classifier);

        if (reply_classifier.size()>0)
        {
            recognition_started = true;

            if (reply_classifier.get(0).asVocab()!=ACK)
            {
                thr_scorer->clear_hist();
            }

        } else
        {
            std::cout << "Cannot get response to recognize command." << std::endl;
            thr_scorer->clear_hist();
            mutex.post();
            return;
        }
    }

    if (state==STATE_CLASSIFYING)
    {
        thr_scorer->get_predicted_class(current_class);
        thr_cropper->set_displayed_class(current_class);
    }

  /*  if (state==STATE_WHATISTHIS)
    {

        if (mode==MODE_ROBOT)
        {
            bool ok = observe_robot(current_class);
            if (!ok)
            {
                std::cout << "observe_robot() failed!" << std::endl;
                set_state(STATE_CLASSIFYING);
                mutex.post();
                return;
            }
        }
        if  (mode==MODE_ROBOT_TOOL)
        {
            bool ok = observe_robot_tool(current_class);
            if (!ok)
            {
                std::cout << "observe_robot_tool() failed!" << std::endl;
                set_state(STATE_CLASSIFYING);
                mutex.post();
                return;
            }
        }
        if (mode==MODE_HUMAN)
        {
            thr_scorer->get_predicted_class(current_class);
            thr_cropper->set_displayed_class(current_class);
        }

        if (current_class!="?")
            speak("I think this is a " + current_class);
        else
            speak("Sorry, I cannot recognize this object.");

        if (mode==MODE_ROBOT)
        {

            bool ok = complete_robot();
            if (!ok)
            {
                std::cout << "complete_robot() failed!" << std::endl;
                set_state(STATE_CLASSIFYING);
                mutex.post();
                return;
            }
        }
        if  (mode==MODE_ROBOT_TOOL)
        {

            bool ok = complete_robot_tool(false);
            if (!ok)
            {
                std::cout << "complete_robot_tool() failed!" << std::endl;
                set_state(STATE_CLASSIFYING);
                mutex.post();
                return;
            }
        }


        set_state(STATE_CLASSIFYING);

    }
*/
    if (state==STATE_TRAINING)
    {

        thr_cropper->get_displayed_class(current_class);

        speak("Ok, show me this wonderful " + current_class);
        std::cout << "Ok, show me this wonderful " << current_class << std::endl;

        bool ok = false;;
        switch (mode)
        {
        case MODE_ROBOT:            // To avoid code repetition
        case MODE_ROBOT_TOOL:
        {
            if (tool_mode){
                cout << "Training in state MODE_ROBOT_TOOL" << endl;
                ok = store_robot_tool(current_class.c_str());
            }else{
                cout << "Training in state MODE_ROBOT" << endl;
                ok = store_robot(current_class.c_str());
            }
            if (!ok)
            {
                std::cout << "observe_robot() failed!" << std::endl;
                set_state(STATE_CLASSIFYING);
                break;
            }
            ok = complete_robot();
            if (!ok)
            {
                std::cout << "complete_robot() failed!" << std::endl;
                set_state(STATE_CLASSIFYING);
                break;
            }
        } break;

        case MODE_HUMAN:
        {
            cout << "Training in state MODE_HUMAN" << endl;
            ok = store_human(current_class.c_str());
            if (!ok)
            {
                std::cout << "observe_human() failed!" << std::endl;
                set_state(STATE_CLASSIFYING);
                break;
            }
        } break;

        }

        if (!ok)
        {
            std::cout << "observe failed!" << std::endl;
            set_state(STATE_CLASSIFYING);
            mutex.post();
            return;
        }

        if (!send_cmd2rpc_classifier("train", 10))
        {
            std::cout << "Classifier busy for training!" << std::endl;
            set_state(STATE_CLASSIFYING);
            mutex.post();
            return;
        }

        speak("Ok, now I know the " + current_class);

        if (!send_cmd2rpc_classifier("recognize", 10))
        {
            std::cout << "Classifier busy for recognition!" << std::endl;
            set_state(STATE_CLASSIFYING);
            mutex.post();
            return;
        }

        set_state(STATE_CLASSIFYING);

    }

    mutex.post();

}

bool ManagerThread::execReq(const Bottle &command, Bottle &reply)
{
    switch(command.get(0).asVocab())
    {
    default:
        return false;
    }
}

bool ManagerThread::send_cmd2rpc_classifier(string cmdstring, int Ntrials)
{
    bool done = false;
    for (int i=0; !done && i<Ntrials; i++)
    {
        Bottle cmd_classifier,reply_classifier;
        cmd_classifier.addString(cmdstring.c_str());
        port_rpc_classifier.write(cmd_classifier,reply_classifier);

        if (reply_classifier.size()>0 && (reply_classifier.get(0).asVocab()==ACK || reply_classifier.get(0).asString() =="ok"))
            done = true;
    }

    return done;
}

bool ManagerThread::send_doublecmd2rpc_classifier(string cmdstring1, string cmdstring2, int Ntrials)
{
    bool done = false;
    for (int i=0; !done && i<Ntrials; i++)
    {
        Bottle cmd_classifier,reply_classifier;
        cmd_classifier.addString(cmdstring1.c_str());
        cmd_classifier.addString(cmdstring2.c_str());
        port_rpc_classifier.write(cmd_classifier,reply_classifier);

        if (reply_classifier.size()>0 && (reply_classifier.get(0).asVocab()==ACK || reply_classifier.get(0).asString() =="ok"))
            done = true;
    }

    return done;
}

bool ManagerThread::execHumanCmd(Bottle &command, Bottle &reply)
{
    bool ok = false;

    mutex.wait();

    switch(command.get(0).asVocab())
    {

    case CMD_HELP:
    {
        reply.addVocab(Vocab::encode("many"));
        reply.addString(" ");
        reply.addString("train <classname>                  : observes the class and retrain");
        reply.addString("forget all                         : forgets all classes");
        reply.addString("forget <classname>                 : forgets the specified class and retrain");
        reply.addString("what                               : in human mode provides current label,");
        reply.addString("                                     in robot mode takes the object and explores it");
        reply.addString(" ");
        reply.addString("robot <bool>                       : sets the robot mode. Bool sets tool mode (default false)");
        reply.addString("human                              : sets the human mode");
        reply.addString(" ");
        reply.addString("radius                             : sets the square ROI");
        reply.addString("bbdisp                             : sets the dispBlobber ROI");
        reply.addString(" ");
        reply.addString("set radius_human <value>           [ int>0  ]: sets the radius of the square ROI in human mode");
        reply.addString("set radius_robot <value>           [ int>0  ]: sets the radius of the square ROI in robot mode ");
        reply.addString("set buffer_size <value>            [ int>0  ]: sets the size of the buffer to average predictions");
        reply.addString("set skip_frames <value>            [ int>=0 ]: sets the number of frames to skip in training mode");
        reply.addString("set human_time_training <value>    [ int>0  ]: sets the time interval of training (tracking) in human mode");
        reply.addString(" ");
        reply.addString("get classes                        : provides the list of known classes");

        ok = true;

    } break ;


    case CMD_TRAIN:
    {
        if (command.size()<2)
        {
            ok = false;
            reply.addString("You need to specify a class!");
            break;
        }

        string class_name = command.get(1).asString().c_str();
        thr_cropper->set_displayed_class(class_name);

        set_state(STATE_TRAINING);

        ok = true;
        reply.addVocab(ACK);
        break;
    }

    case CMD_ROBOT:
    {
        Bottle cmd_are,reply_are;
        cmd_are.addString("idle");
        port_rpc_are_cmd.write(cmd_are,reply_are);
        if (reply_are.size()==0 || reply_are.get(0).asVocab()!=ACK)
        {
            ok = false;
            reply.addString("Cannot set ARE to idle!");
            break;
        }

        reply_are.clear();
        cmd_are.clear();
        cmd_are.addString("home");
        port_rpc_are_cmd.write(cmd_are,reply_are);
        if(reply_are.size()>0 && reply_are.get(0).asVocab()!=ACK)
        {
            ok = false;
            reply.addString("Cannot set ARE to home!");
            break;
        }

        tool_mode = false;
        if (command.size()==2){
            tool_mode = command.get(1).asBool();
            cout << " Mode set to ROBOT TOOL" << endl;
        }

        if (tool_mode){
            set_mode(MODE_ROBOT_TOOL);
            set_crop_mode(CROP_MODE_BBDISP);
        }else{
            set_mode(MODE_ROBOT);
        }

        ok = true;
        reply.addVocab(ACK);
        break;
    }

    case CMD_HUMAN:
    {

        Bottle cmd_are,reply_are;
        cmd_are.addString("idle");
        port_rpc_are_cmd.write(cmd_are,reply_are);
        if (reply_are.size()>0 && reply_are.get(0).asVocab()!=ACK)
        {
            ok = false;
            reply.addString("Cannot set ARE to idle!");
            break;
        }

        reply_are.clear();
        cmd_are.clear();
        cmd_are.addString("track");
        cmd_are.addString("motion");
        cmd_are.addString("no_sacc");
        port_rpc_are_cmd.write(cmd_are,reply_are);
        if (reply_are.size()>0 && reply_are.get(0).asVocab()!=ACK)
        {
            ok = false;
            reply.addString("Cannot set ARE to track motion no_sacc!");
            break;
        }

        set_mode(MODE_HUMAN);

        ok = true;
        reply.addVocab(ACK);
        break;
    }

    case CMD_FORGET:
    {
        if (command.size()>1)
        {
            string class_forget = command.get(1).asString().c_str();

            // check that class_forget is either a known class name or "all"
            bool done = false, class_known = false, empty_classes = false;
            for (int i=0; !done && i<10; i++)
            {
                Bottle cmd_classifier,reply_classifier;
                cmd_classifier.addString("objList");
                port_rpc_classifier.write(cmd_classifier,reply_classifier);

                if (reply_classifier.size()>0 && reply_classifier.get(0).asVocab()==ACK)
                {
                    done = true;
                    Bottle *class_list = reply_classifier.get(1).asList();
                    if (class_list->size()==0)
                    {
                        ok = false;
                        empty_classes = true;
                        reply.addString("There are no known classes.");
                    }
                    else
                    {
                        for (int idx_class=0; idx_class<class_list->size(); idx_class++)
                        {
                            if (class_forget==class_list->get(idx_class).asString().c_str()) {
                                class_known = true;
                                break;
                            }
                        }
                    }
                }
            }

            if (empty_classes==false)
            {
                if (class_known || class_forget=="all")
                {
                    if (!send_doublecmd2rpc_classifier("forget", class_forget.c_str(), 10))
                    {
                        ok = false;
                        reply.addString("Classifier busy for forgetting one object!");
                        break;
                    }

                    speak(class_forget + " forgotten!");

                    recognition_started = false;

                    ok = true;
                    reply.addVocab(ACK);
                }
                else
                {
                    ok = false;
                    reply.addString("Class is unknown.");
                }
            }
            
            break;

        }
        else
        {
            ok = false;
            reply.addString("Syntax must be: forget <classname> or forget all");
            break;
        }
    }

    case CMD_WHATISTHIS:
    {
        // first check that some classes are known
        bool done = false;
        for (int i=0; !done && i<10; i++)
        {
            Bottle cmd_classifier,reply_classifier;
            cmd_classifier.addString("objList");
            port_rpc_classifier.write(cmd_classifier,reply_classifier);

            if (reply_classifier.size()>0 && reply_classifier.get(0).asVocab()==ACK)
            {
                done = true;
                ok = true;
                Bottle *class_list = reply_classifier.get(1).asList();
                if (class_list->size()==0)
                {
                    reply.addString("No classes in database.");
                    speak("Sorry, I don't know anything.");
                    ok = false;
                }
            }
        }

        if (done==false)
        {
            ok = false;
            reply.addString("Classifier busy for getting objList!");
        }

        if (done == true && ok == true)
        {
            cout << "Ready to predict!!" << endl;

            //mutex.wait();
            string pred_clas;
            if (mode==MODE_ROBOT)
            {
                cout << "Predicting object in hand!!" << endl;
                bool ok = observe_robot(pred_clas);
                if (!ok)
                {
                    std::cout << "observe_robot() failed!" << std::endl;
                    reply.addString("Couldn't look at object");
                    mutex.post();
                }
                reply.addVocab(ACK);
                reply.addString(pred_clas);
                complete_robot();
            }
            if  (mode==MODE_ROBOT_TOOL)
            {
                cout << "Predicting tool in hand!!" << endl;
                bool ok = observe_robot_tool(pred_clas);
                if (!ok)
                {
                    std::cout << "observe_robot_tool() failed!" << std::endl;
                    reply.addString("Couldn't look at tool");
                    mutex.post();
                }
                reply.addVocab(ACK);
                reply.addString(pred_clas);
                complete_robot_tool(false);
            }
            if (mode==MODE_HUMAN)
            {
                //set_state(STATE_WHATISTHIS);
                cout << "Predicting object in users hand!!" << endl;
                reply.addVocab(ACK);
                thr_scorer->get_predicted_class(pred_clas);
                reply.addString(pred_clas);
            }

            if (pred_clas != "?")
                speak("I think this is a " + pred_clas);
            else
                speak("Sorry, I cannot recognize this object.");

            //mutex.post();
        }

    } break;

    case CMD_SET:
    {
        if (command.size()>2)
        {
            string property = command.get(1).asString().c_str();
            if (property == "radius_human")
            {
                int r = command.get(2).asInt();
                ok = thr_cropper->set_radius_human(r);
            }
            else if (property == "radius_robot")
            {
                int r = command.get(2).asInt();
                ok = thr_cropper->set_radius_robot(r);
            }
            else if (property == "buffer_size")
            {
                int r = command.get(2).asInt();
                ok = thr_scorer->set_buffer_size(r);
            }
            else if (property == "skip_frames")
            {
                int s = command.get(2).asInt();
                ok = thr_cropper->set_skip_frames(s);
            }
            else if (property == "human_time_training")
            {
                double t = command.get(2).asDouble();
                ok = set_human_time_training(t);
            }
            else
            {
                ok = false;
                reply.addString("Unknown property.");
                break;
            }

        } else
        {
            ok = false;
            reply.addString("Syntax must be: set <prop> <value>");
            break;
        }

        if (ok)
            reply.addVocab(ACK);
        else
            reply.addString("Cannot set property (check e.g. property range)");

        break;

//  } break;
    }

    case CMD_GET:
    {
        if (command.size()>1)
        {
            string property = command.get(1).asString().c_str();
            if (property == "classes")
            {

                bool done = false;
                for (int i=0; !done && i<10; i++)
                {
                    Bottle cmd_classifier,reply_classifier;
                    cmd_classifier.addString("objList");
                    port_rpc_classifier.write(cmd_classifier,reply_classifier);

                    if (reply_classifier.size()>0 && reply_classifier.get(0).asVocab()==ACK)
                    {
                        done = true;
                        ok = true;
                        Bottle *class_list = reply_classifier.get(1).asList();
                        if (class_list->size()==0)
                        {
                            reply.addString("There are no known classes.");
                        }
                        else
                        {
                            reply.addList() = *class_list;
                        }
                    }
                }

                if (done==false)
                {
                    ok = false;
                    reply.addString("Classifier busy for getting objList!");
                }

                break;

            }
            else
            {
                ok = false;
                reply.addString("Unknown property.");
                break;
            }
        }
        else
        {
            ok = false;
            reply.addString("Syntax must be: get <property>");
            break;
        }

//  } break;
    }

    case CMD_RADIUS:
    {
        ok = set_crop_mode(CROP_MODE_RADIUS);
        if (ok)
            reply.addVocab(ACK);
        else
            reply.addString("CropperThread cannot set crop_mode.");
    } break;

    case CMD_BBDISP:
    {
        ok = set_crop_mode(CROP_MODE_BBDISP);
        if (ok)
            reply.addVocab(ACK);
        else
            reply.addString("CropperThread cannot set crop_mode.");
    } break;

    default:
        reply.addString("Unknown command!");

    }

    mutex.post();

    return ok;

}

void ManagerThread::interrupt()
{
    mutex.wait();

    port_rpc_are.interrupt();
    port_rpc_are_cmd.interrupt();
    port_rpc_are_cmd.interrupt();
    port_rpc_classifier.interrupt();
    port_out_speech.interrupt();

    thr_cropper->interrupt();
    thr_scorer->interrupt();

    mutex.post();
}

bool ManagerThread::releaseThread()
{
    mutex.wait();

    port_rpc_are.close();
    port_rpc_are_cmd.close();
    port_rpc_are_cmd.close();
    port_rpc_classifier.close();
    port_out_speech.close();

    thr_cropper->stop();
    delete thr_cropper;
    thr_scorer->stop();
    delete thr_scorer;

    mutex.post();

    return true;
}
