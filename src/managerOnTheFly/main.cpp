/* 
 * Copyright (C) 2011 Department of Robotics Brain and Cognitive Sciences - Istituto Italiano di Tecnologia
 * Author: Carlo Ciliberto
 * email:  carlo.ciliberto@iit.it
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

#include <yarp/os/Network.h>
#include <yarp/os/RFModule.h>
#include <yarp/os/Time.h>
#include <yarp/os/BufferedPort.h>
#include <yarp/os/RateThread.h>
#include <yarp/os/Semaphore.h>
#include <yarp/os/RpcClient.h>
#include <yarp/os/PortReport.h>
#include <yarp/os/Stamp.h>

#include <yarp/sig/Vector.h>
#include <yarp/sig/Image.h>

#include <yarp/math/Math.h>
#include <yarp/math/Rand.h>

#include <highgui.h>
#include <cv.h>

#include <stdio.h>
#include <string>
#include <deque>
#include <algorithm>
#include <vector>
#include <iostream>
#include <fstream>
#include <list>

using namespace std;
using namespace yarp;
using namespace yarp::os;
using namespace yarp::sig;
using namespace yarp::math;



#define                 STATE_IDLE          0
#define                 STATE_TRAINING      1
#define                 STATE_CLASSIFY      2

#define                 MODE_ROBOT          0
#define                 MODE_HUMAN          1

#define                 CMD_TRAIN           VOCAB4('t','r','a','i')
#define                 CMD_CLASSIFY        VOCAB4('c','l','a','s')
#define                 CMD_ROBOT           VOCAB4('r','o','b','o')
#define                 CMD_HUMAN           VOCAB4('h','u','m','a')



class TransformerThread: public RateThread
{
private:
    ResourceFinder                      &rf;
    bool                                verbose;

    //input
    BufferedPort<Image>                 port_in_img;
    BufferedPort<Bottle>                port_in_blobs;

    //output
    Port                                port_out_show;
    Port                                port_out_crop;


    int                                 radius_crop;

    ImageOf<PixelBgr>                   img_crop;


    string                              current_class;

    bool                                coding_interrupted;




public:
    TransformerThread(ResourceFinder &_rf)
        :RateThread(5),rf(_rf)
    {
    }





    virtual bool threadInit()
    {
        verbose=rf.check("verbose");

        string name=rf.find("name").asString().c_str();

        radius_crop=rf.check("radius_crop",Value(40)).asInt();

        //Ports
        //-----------------------------------------------------------
        //input
        port_in_img.open(("/"+name+"/img:i").c_str());
        port_in_blobs.open(("/"+name+"/blobs:i").c_str());

        //output
        port_out_show.open(("/"+name+"/show:o").c_str());
        port_out_crop.open(("/"+name+"/crop:o").c_str());
        //------------------------------------------------------------

        current_class="?";
        coding_interrupted=true;

        return true;
    }


    virtual void run()
    {
        Image *img=port_in_img.read(false);
        if(img==NULL)
            return;

        Stamp stamp;
        port_in_img.getEnvelope(stamp);

        Bottle *blobs=port_in_blobs.read(false);
        if(blobs!=NULL)
        {
            Bottle *window=blobs->get(0).asList();
            if(window->get(2).asInt()>10)
            {
                int x=window->get(0).asInt();
                int y=window->get(1).asInt();

                int radius=std::min(radius_crop,x);
                radius=std::min(radius,y);
                radius=std::min(radius,img->width()-x-1);
                radius=std::min(radius,img->height()-y-1);
                int radius2=radius<<1;

                img_crop.resize(radius2,radius2);

                cvSetImageROI((IplImage*)img->getIplImage(),cvRect(x-radius,y-radius,radius2,radius2));
                cvCopy((IplImage*)img->getIplImage(),(IplImage*)img_crop.getIplImage());
                cvResetImageROI((IplImage*)img->getIplImage());


                //send the cropped image out and wait for response
                if(!coding_interrupted)
                {
                    port_out_crop.setEnvelope(stamp);
                    port_out_crop.write(img_crop);
                }

                CvFont font;
                cvInitFont(&font,CV_FONT_HERSHEY_SIMPLEX,0.8,0.8,0,3);

                int y_text=y-radius-10;
                if(y_text<5) y_text=y+radius+2;

                cvRectangle(img->getIplImage(),cvPoint(x-radius,y-radius),cvPoint(x+radius,y+radius),cvScalar(0,255,0),2);
                cvPutText(img->getIplImage(),current_class.c_str(),cvPoint(x-radius,y_text),&font,cvScalar(0,0,255));
            }
        }

        if(port_out_show.getOutputCount()>0)
            port_out_show.write(*img);
    }


    bool set_current_class(string _current_class)
    {
        current_class=_current_class;
        return true;
    }

    bool execReq(const Bottle &command, Bottle &reply)
    {
        switch(command.get(0).asVocab())
        {
            default:
                return false;
        }
    }


    virtual void interrupt()
    {
        port_in_img.interrupt();
        port_in_blobs.interrupt();
        port_out_show.interrupt();
        port_out_crop.interrupt();
    }

    virtual bool releaseThread()
    {
        port_in_img.close();
        port_in_blobs.close();
        port_out_show.close();
        port_out_crop.close();

        return true;
    }

    bool interruptCoding()
    {
        coding_interrupted=true;
        return true;
    }

    bool resumeCoding()
    {
        coding_interrupted=false;
        return true;
    }



};




class StorerPort: public BufferedPort<Bottle>
{
private:
    Semaphore                       mutex;
    list<Bottle>                    *scores_current;
    list<Bottle>                    scores_main;
    list<Bottle>                    scores_back;

    virtual void onRead(Bottle &bot)
    {
        mutex.wait();
        scores_current->push_back(bot);

        //if the scores exceed a certain threshold clear its head
        while(scores_current->size()>200)
            scores_current->pop_front();
        mutex.post();
    }

public:
    StorerPort()
    {
        scores_current=&scores_main;
    }


    bool get_scores(list<Bottle> &_scores)
    {
        mutex.wait();
        scores_current=&scores_main;
        mutex.post();

        for(list<Bottle>::iterator itr=scores_main.begin(); itr!=scores_main.end(); itr++)
            _scores.push_back(*itr);

        mutex.wait();
        scores_main.splice(scores_main.end(),scores_back);
        scores_current=&scores_main;
        mutex.post();

        return true;
    }

    bool reset_scores()
    {
        mutex.wait();
        scores_current->clear();
        mutex.post();

        return true;
    }
};





class ManagerThread: public RateThread
{
private:
    ResourceFinder                      &rf;
    bool                                verbose;

    Semaphore                           mutex;

    //threads
    TransformerThread                   *thr_transformer;

    //input
    StorerPort                          port_in_scores;

    //rpc
    RpcClient                           port_rpc_are;
    RpcClient                           port_rpc_are_get;
    RpcClient                           port_rpc_are_cmd;
    RpcClient                           port_rpc_human;
    RpcClient                           port_rpc_classifier;

    list<Bottle>                        scores_buffer;

    double                              observe_human_time_training;
    double                              observe_human_time_classify;

    int                                 mode;
    int                                 state;

    deque<string>                       known_objects;

private:

    bool observe_robot()
    {
        //check if the robot is already holding an object
        Bottle command,reply;
        command.addString("holding");
        port_rpc_are_get.write(command,reply);

        if(reply.size()==0)
            return false;

        //if the robot is not holding an object then ask the human to give one
        if(reply.get(0).asString()!="true")
        {
            reply.clear();
            command.clear();
            command.addString("beg");
            port_rpc_are_cmd.write(command,reply);

            if(reply.size()==0 || reply.get(0).asString()!="ack")
                return false;
        }

        //resume the storer
        thr_transformer->resumeCoding();

        //perform the exploration of the hand
        reply.clear();
        command.clear();
        command.addString("explore");
        command.addString("hand");
        port_rpc_are_cmd.write(command,reply);

        //interrupt the storer
        thr_transformer->interruptCoding();

        return true;
    }


    bool observe_human()
    {
        thr_transformer->resumeCoding();
        if(state==STATE_TRAINING)
            Time::delay(observe_human_time_training);
        else
            Time::delay(observe_human_time_classify);

        thr_transformer->interruptCoding();
        return true;
    }

    bool observe()
    {
        bool ok;
        switch(mode)
        {
            case MODE_ROBOT:
            {
                ok=observe_robot();
                break;
            }

            case MODE_HUMAN:
            {
                ok=observe_human();
                break;
            }
        }

        return ok;
    }



    bool complete_robot()
    {
        //just drop the object
        Bottle command,reply;
        command.addString("drop");
        port_rpc_are_cmd.write(command,reply);

        //wait for new commands
        state=STATE_IDLE;

        return true;
    }


    bool complete_human()
    {
        //do nothing
        return true;
    }

    bool complete()
    {
        bool ok;
        switch(mode)
        {
            case MODE_ROBOT:
            {
                ok=complete_robot();
                break;
            }

            case MODE_HUMAN:
            {
                ok=complete_human();
                break;
            }
        }

        //clear the buffer
        thr_transformer->interruptCoding();
        port_in_scores.reset_scores();
        scores_buffer.clear();

        return ok;
    }



    bool classified()
    {
        string label="?";
        //do the mumbo jumbo for classification
        //get the buffer from the score store
        port_in_scores.get_scores(scores_buffer);

        if(scores_buffer.size()==0)
            return false;

        int n_classes=scores_buffer.front().size();

        vector<double> class_avg(n_classes,0.0);
        vector<int> class_votes(n_classes,0);

        for(list<Bottle>::iterator score_itr=scores_buffer.begin(); score_itr!=scores_buffer.end(); score_itr++)
        {
            double max_score=-1000.0;
            int max_idx;
            for(int class_idx=0; class_idx<n_classes; class_idx++)
            {
                double s=score_itr->get(class_idx).asDouble();
                class_avg[class_idx]+=s;
                if(s>max_score)
                {
                    max_score=s;
                    max_idx=class_idx;
                }
            }

            class_votes[max_idx]++;
        }

        double max_avg=-10000.0;
        double max_votes=-10000.0;
        int max_avg_idx;
        int max_votes_idx;

        for(int class_idx=0; class_idx<n_classes; class_idx++)
        {
            if(class_avg[class_idx]>max_avg)
            {
                max_avg=class_avg[class_idx];
                max_avg_idx=class_idx;
            }

            if(class_votes[class_idx]>max_votes)
            {
                max_votes=class_votes[class_idx];
                max_votes_idx=class_idx;
            }
        }

        label=known_objects[max_avg_idx];
        if(max_votes/scores_buffer.size()<0.75)
            label="?";

        thr_transformer->set_current_class(label);

        return label!="?";
    }

    void decide()
    {
        switch(state)
        {
            case STATE_TRAINING:
            {
                train();
                complete();
                break;
            }

            case STATE_CLASSIFY:
            {
                if(classified())
                    complete();
                break;
            }
        }

    }

    bool train()
    {
        bool done=false;
        for(int i=0; !done && i<10; i++)
        {
            Bottle cmd_classifier,reply_classifier;
            cmd_classifier.addString("train");
            port_rpc_classifier.write(cmd_classifier,reply_classifier);

            if(reply_classifier.size()>0 && reply_classifier.get(0)=="ack")
                done=true;
        }

        return done;
    }


public:
    ManagerThread(ResourceFinder &_rf)
        :RateThread(10),rf(_rf)
    {
    }

    virtual bool threadInit()
    {
        verbose=rf.check("verbose");

        string name=rf.find("name").asString().c_str();

        observe_human_time_training=rf.check("observe_human_time_training",Value(10.0)).asDouble();
        observe_human_time_classify=rf.check("observe_human_time_classify",Value(2.0)).asDouble();

        thr_transformer=new TransformerThread(rf);
        thr_transformer->start();

        //Ports
        //-----------------------------------------------------------
        //input
        port_in_scores.open(("/"+name+"/scores:i").c_str());

        //rpc
        port_rpc_are.open(("/"+name+"/are/rpc").c_str());
        port_rpc_are_get.open(("/"+name+"/are/get:io").c_str());
        port_rpc_are_cmd.open(("/"+name+"/are/cmd:io").c_str());
        port_rpc_classifier.open(("/"+name+"/classifier:io").c_str());
        //------------------------------------------------------------

        thr_transformer->interruptCoding();

        state=STATE_IDLE;
        mode=MODE_HUMAN;

        return true;
    }


    virtual void run()
    {
        if(state==STATE_IDLE)
            return;

        mutex.wait();
        observe();
        decide();
        mutex.post();
    }





    bool execReq(const Bottle &command, Bottle &reply)
    {
        switch(command.get(0).asVocab())
        {
            default:
                return false;
        }
    }


    bool execHumanCmd(Bottle &command, Bottle &reply)
    {
        switch(command.get(0).asVocab())
        {
            case CMD_TRAIN:
            {
                mutex.wait();

                if(command.size()>1)
                {
                    string class_name=command.get(1).asString().c_str();
                    Bottle cmd_classifier,reply_classifier;
                    cmd_classifier.addString("save");
                    cmd_classifier.addString(class_name.c_str());
                    port_rpc_classifier.write(cmd_classifier,reply_classifier);

                    if(reply_classifier.size()>0 && reply_classifier.get(0)=="ack")
                    {
                        mutex.wait();
                        thr_transformer->set_current_class(class_name);
                        state=STATE_TRAINING;

                        bool found=false;
                        for(unsigned int i=0; i<known_objects.size(); i++)
                            if(known_objects[i]==class_name)
                                found=true;

                        if(!found)
                        {
                            known_objects.push_back(class_name);
                            sort(known_objects.begin(),known_objects.end());
                        }
                        mutex.post();

                        reply.addString(("learning "+class_name).c_str());
                    }
                    else
                        reply.addString("classifier busy!");
                }
                else
                    reply.addString("Error! Need to specify a class!");

                mutex.post();
                break;
            }

            case CMD_CLASSIFY:
            {
                mutex.wait();

                Bottle cmd_classifier,reply_classifer;
                cmd_classifier.addString("recognize");
                port_rpc_classifier.write(cmd_classifier,reply_classifer);

                if(reply_classifer.size()>0 && reply_classifer.get(0)=="ack")
                {
                    thr_transformer->set_current_class("?");
                    state=STATE_CLASSIFY;


                    reply.addString("classifing");
                }
                else
                    reply.addString("classifier busy!");

                mutex.post();
                break;
            }

            case CMD_ROBOT:
            {
                mutex.wait();

                Bottle cmd_are,reply_are;
                cmd_are.addString("idle");
                port_rpc_are_cmd.write(cmd_are,reply_are);

                if(reply_are.size()>0 && reply_are.get(0).asString()=="ack")
                {
                    reply_are.clear();
                    cmd_are.clear();
                    cmd_are.addString("home");
                    port_rpc_are_cmd.write(cmd_are,reply_are);

                    if(reply_are.size()>0 && reply_are.get(0).asString()=="ack")
                        mode=MODE_ROBOT;
                    else
                        reply.addString("Error!");
                }
                else
                    reply.addString("Error!");

                mutex.post();
                break;
            }

            case CMD_HUMAN:
            {
                mutex.wait();

                Bottle cmd_are,reply_are;
                cmd_are.addString("idle");
                port_rpc_are_cmd.write(cmd_are,reply_are);

                if(reply_are.size()>0 && reply_are.get(0).asString()=="ack")
                {
                    reply_are.clear();
                    cmd_are.clear();
                    cmd_are.addString("track");
                    cmd_are.addString("motion");
                    port_rpc_are_cmd.write(cmd_are,reply_are);

                    if(reply_are.size()>0 && reply_are.get(0).asString()=="ack")
                        mode=MODE_HUMAN;
                    else
                        reply.addString("Error!");
                }
                else
                    reply.addString("Error!");

                mutex.post();
                break;
            }


        }

        return true;
    }

    virtual void interrupt()
    {
        port_in_scores.interrupt();
        port_rpc_are.interrupt();
        port_rpc_are_cmd.interrupt();
        port_rpc_are_cmd.interrupt();
        port_rpc_classifier.interrupt();

        thr_transformer->interrupt();
    }

    virtual bool releaseThread()
    {
        port_in_scores.close();
        port_rpc_are.close();
        port_rpc_are_cmd.close();
        port_rpc_are_cmd.close();
        port_rpc_classifier.close();

        thr_transformer->stop();
        delete thr_transformer;

        return true;
    }



};






class ManagerModule: public RFModule
{
protected:
    ManagerThread       *manager_thr;
    RpcServer           port_rpc_human;
    Port                port_rpc;

public:
    ManagerModule()
    {}

    virtual bool configure(ResourceFinder &rf)
    {
        string name=rf.find("name").asString().c_str();

        Time::turboBoost();

        manager_thr=new ManagerThread(rf);
        manager_thr->start();

        port_rpc_human.open(("/"+name+"/human:io").c_str());
        port_rpc.open(("/"+name+"/rpc").c_str());
        attach(port_rpc);

        return true;
    }

    virtual bool close()
    {
        manager_thr->interrupt();
        manager_thr->stop();
        delete manager_thr;

        port_rpc_human.interrupt();
        port_rpc_human.close();

        port_rpc.interrupt();
        port_rpc.close();

        return true;
    }

    virtual bool respond(const Bottle &command, Bottle &reply)
    {
        if(manager_thr->execReq(command,reply))
            return true;
        else
            return RFModule::respond(command,reply);
    }

    virtual double getPeriod()    { return 1.0;  }
    virtual bool   updateModule()
    {
        Bottle human_cmd,reply;
        port_rpc_human.read(human_cmd,true);
        if(human_cmd.size()>0)
            manager_thr->execHumanCmd(human_cmd,reply);
        port_rpc_human.reply(reply);

        return true;
    }

};




int main(int argc, char *argv[])
{
   Network yarp;

   if (!yarp.checkNetwork())
       return -1;

   ResourceFinder rf;
   rf.setVerbose(true);
   rf.setDefaultContext("onTheFlyRecognition/conf");
   rf.setDefaultConfigFile("config.ini");
   rf.configure("ICUB_ROOT",argc,argv);
   rf.setDefault("name","onTheFlyRecognition");
   ManagerModule mod;

   return mod.runModule(rf);
}

