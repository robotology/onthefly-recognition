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
        std::cout << "Classifier busy for stopping to save scores: please make it stops somehow!" << std::endl;
        return false;
    }

    return true;
}

bool ManagerThread::store_robot(string class_name)
{
    // check if the robot is already holding an object
    Bottle command,reply;
    command.addString("get");
    command.addString("hold");
    port_rpc_are_get.write(command,reply);
    if (reply.size()==0 || reply.get(0).asVocab()!=ACK)
    {
        std::cout << "Cannot set ARE to get hold!" << std::endl;
        return false;
    }

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
            std::cout << "Classifier busy for stopping to save scores: please make it stops somehow!" << std::endl;
        }

        complete_robot();
        return false;
    }

    if (!send_cmd2rpc_classifier("stop", 10))
    {
        std::cout << "Classifier busy for stopping to save scores: please make it stops somehow!" << std::endl;
        complete_robot();
        return false;
    }

    return true;
}

bool ManagerThread::observe_robot(string &predicted_class)
{
    // check if the robot is already holding an object
    Bottle command,reply;
    command.addString("get");
    command.addString("hold");
    port_rpc_are_get.write(command,reply);
    if (reply.size()==0 || reply.get(0).asVocab()!=ACK)
    {
        std::cout << "Cannot set ARE to get hold!" << std::endl;
        return false;
    }

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

bool ManagerThread::complete_robot()
{
    // just drop the object
    Bottle command,reply;
    command.addString("give");
    port_rpc_are_cmd.write(command,reply);
    if (reply.size()==0 || reply.get(0).asVocab()!=ACK)
    {
        std::cout << "Cannot set ARE to give!" << std::endl;
        return false;
    }

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

    // out speech
    port_out_speech.open(("/"+name+"/speech:o").c_str());

    thr_cropper = new CropperThread(rf);
    thr_cropper->start();

    thr_scorer = new ScorerThread(rf);
    thr_scorer->start();

    mutex.wait();

    human_time_training = rf.check("human_time_training", Value(10.0)).asDouble();
    recognition_started = false;

    set_mode(MODE_HUMAN);
    set_state(STATE_CLASSIFYING);
    set_crop_mode(CROP_MODE_RADIUS);
    
    is_face = false;

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

    if (state==STATE_WHATISTHIS)
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

        if (mode==MODE_HUMAN)
        {
            thr_scorer->get_predicted_class(current_class);
            thr_cropper->set_displayed_class(current_class);
        }

        if (current_class!="?")
        {
            if (is_face)
                speak("I think this is " + current_class);
            else
                speak("I think this is a " + current_class);
        }
        else
        {
            if (is_face)
                speak("Sorry, I cannot recognize this person.");
            else
                speak("Sorry, I cannot recognize this object.");
        }
            
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

        set_state(STATE_CLASSIFYING);
    }

    if (state==STATE_TRAINING)
    {

        thr_cropper->get_displayed_class(current_class);

        if (is_face)
            speak("Ok, let me have a look at " + current_class);
        else
            speak("Ok, show me this wonderful " + current_class);

        bool ok = false;;
        switch (mode)
        {
        case MODE_ROBOT:
        {
            ok = store_robot(current_class.c_str());
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

        if (is_face)
           speak("Hello " + current_class + " nice to meet you ");
        else
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
        reply.addString("train <classname> <is_face>        : observes the class and retrain");
        reply.addString("forget all                         : forgets all classes");
        reply.addString("forget <classname>                 : forgets the specified class and retrain");
        reply.addString("what  <is_face>                    : in human mode provides current label,");
        reply.addString("                                     in robot mode takes the object and explores it");
        reply.addString(" ");
        reply.addString("robot                              : sets the robot mode");
        reply.addString("human                              : sets the human mode");
        reply.addString(" ");
        reply.addString("radius                             : active only in human mode, sets the square ROI");
        reply.addString("bbdisp                             : active only in human mode, sets the dispBlobber ROI");
        reply.addString(" ");
        reply.addString("set radius_human <value>           [ int>0  ]: sets the radius of the square ROI in human mode");
        reply.addString("set radius_robot <value>           [ int>0  ]: sets the radius of the square ROI in robot mode ");
        reply.addString("set buffer_size <value>            [ int>0  ]: sets the size of the buffer to average predictions");
        reply.addString("set skip_frames <value>            [ int>=0 ]: sets the number of frames to skip in training mode");
        reply.addString("set human_time_training <value>    [ int>0  ]: sets the time interval of training (tracking) in human mode");
        reply.addString(" ");
        reply.addString("get classes                        : provides the list of known classes");
        reply.addString("get skip_frames                    : provides the skipped frames in training");
        reply.addString("get human_time_training            : provides the time interval of training");

        ok = true;

    } break ;


    case CMD_TRAIN:
    {
        if (command.size()<2)
        {
            ok = false;
            reply.addString("You need to specify a class!");
            speak("Sorry, I missed the name of the object. Can you repeat?");
            break;
        }

        string class_name = command.get(1).asString().c_str();
        is_face = command.get(2).asBool();
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

        set_mode(MODE_ROBOT);

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
                        speak("Sorry, but I do not have classes to forget.");
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
                    speak("Sorry, but I do not know this class.");
                }
            }
            
            break;

        }
        else
        {
            ok = false;
            reply.addString("Syntax must be: forget <classname> or forget all");
            speak("Please, tell me what to forget.");
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

                }
                else
                {
                    is_face = command.get(1).asBool();
                    set_state(STATE_WHATISTHIS);
                    reply.addVocab(ACK);

                }
            }
        }

        if (done==false)
        {
            ok = false;
            reply.addString("Classifier busy for getting objList!");
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
            else if (property=="skip_frames")
            {
                    reply.addInt(thr_cropper->get_skip_frames());
                    ok = true;
                    break;
            }
            else if (property=="human_time_training")
            {
                    reply.addDouble(human_time_training);
                    ok =true;
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
