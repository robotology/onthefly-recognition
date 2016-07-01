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



#define                 ACK                 VOCAB3('a','c','k')
#define                 NACK                VOCAB4('n','a','c','k')

#define                 STATE_TRAINING      0
#define                 STATE_CLASSIFYING   1
#define					STATE_WHATISTHIS	2

#define                 MODE_ROBOT          0
#define                 MODE_HUMAN          1

#define                 CMD_ROBOT           VOCAB4('r','o','b','o')
#define                 CMD_HUMAN           VOCAB4('h','u','m','a')

#define					MAX_BUFFER_SIZE		900

#define                 CMD_TRAIN           VOCAB4('t','r','a','i')
#define                 CMD_FORGET          VOCAB4('f','o','r','g')
#define                 CMD_WHATISTHIS      VOCAB4('w','h','a','t')

#define                 CMD_SETPROP         VOCAB3('s','e','t')


class CropperThread: public RateThread
{

private:

    ResourceFinder                      &rf;
    Semaphore                           mutex;
    bool                                verbose;

    //input
    BufferedPort<Image>                 port_in_img;
    BufferedPort<Bottle>                port_in_blobs;

    //output
    Port                                port_out_show;
    Port                                port_out_crop;
    Port                                port_out_img;
    Port                                port_out_imginfo;

    //rpc
    RpcClient                           port_rpc_are_get_hand;

    int                                 radius_crop; 
    int                                 radius_crop_robot;
    int                                 radius_crop_human;

    ImageOf<PixelBgr>                   img_crop;

    string                              displayed_class;
    string                              true_class;

    int                                 mode;
    int                                 state;

public:

    CropperThread(ResourceFinder &_rf)
        :RateThread(5),rf(_rf)
    {
    }

    virtual bool threadInit()
    {

    	verbose = rf.check("verbose");

        string name = rf.find("name").asString().c_str();

        radius_crop_robot=rf.check("radius_crop_robot",Value(80)).asInt();
        radius_crop_human=rf.check("radius_crop_human",Value(40)).asInt();
        radius_crop = radius_crop_human;

        //input
        port_in_img.open(("/"+name+"/img:i").c_str());
        port_in_blobs.open(("/"+name+"/blobs:i").c_str());

        //output
        port_out_show.open(("/"+name+"/show:o").c_str());
        port_out_crop.open(("/"+name+"/crop:o").c_str());
        port_out_img.open(("/"+name+"/img:o").c_str());
        port_out_imginfo.open(("/"+name+"/imginfo:o").c_str());

        //rpc
        port_rpc_are_get_hand.open(("/"+name+"/are/hand:io").c_str());

        displayed_class="?";
        true_class="?";

        return true;
    }


    virtual void run()
    {

        mutex.wait();

        Image *img=port_in_img.read(false);
        if(img==NULL)
        {
            mutex.post();
            return;
        }

        Stamp stamp;
        port_in_img.getEnvelope(stamp);

        bool found = false;
        int x,y;
        int pixelCount=0;

        if (mode==MODE_HUMAN)
        {
        	Bottle *blobs = port_in_blobs.read(false);

            if (blobs!=NULL)
            {

                Bottle *window = blobs->get(0).asList();

                x = window->get(0).asInt();
                y = window->get(1).asInt();
                pixelCount = window->get(2).asInt();

                radius_crop = radius_crop_human;

                found = true;
            }
        }
        
        if (mode==MODE_ROBOT)
        {
            Bottle cmd_are_hand,reply_are_hand;

            cmd_are_hand.addString("get");
            cmd_are_hand.addString("hand");
            cmd_are_hand.addString("image");
            
            port_rpc_are_get_hand.write(cmd_are_hand,reply_are_hand);
            
            if(reply_are_hand.size()>0 && reply_are_hand.get(0).asVocab()!=NACK)
            {
                x = reply_are_hand.get(2).asInt();
                y = reply_are_hand.get(3).asInt();

                radius_crop = radius_crop_robot;
                
                if (0<x && x<img->width() && 0<y && y<img->height())
                    found=true;
            }
        }
        
        if (found)
        {
            int radius = std::min(radius_crop,x);
            radius = std::min(radius,y);
            radius = std::min(radius,img->width()-x-1);
            radius = std::min(radius,img->height()-y-1);
            
            if (radius>10)
            {
                int radius2=radius<<1;

                img_crop.resize(radius2,radius2);

                cvSetImageROI((IplImage*)img->getIplImage(),cvRect(x-radius,y-radius,radius2,radius2));
                cvCopy((IplImage*)img->getIplImage(),(IplImage*)img_crop.getIplImage());
                cvResetImageROI((IplImage*)img->getIplImage());

                if (port_out_crop.getOutputCount()>0)
                {
                	port_out_crop.setEnvelope(stamp);
                	port_out_crop.write(img_crop);
                }

                if (port_out_img.getOutputCount()>0)
                {
                	port_out_img.setEnvelope(stamp);
                	port_out_img.write(*img);
                }

                if (port_out_imginfo.getOutputCount()>0)
                {
                	port_out_imginfo.setEnvelope(stamp);
                	Bottle imginfo;
                	imginfo.addInt(x);
                	imginfo.addInt(y);
                	imginfo.addInt(pixelCount);
                	imginfo.addString(true_class.c_str());
                	port_out_imginfo.write(imginfo);
                }

                CvFont font;
                cvInitFont(&font,CV_FONT_HERSHEY_SIMPLEX,0.8,0.8,0,3);

                int y_text = y-radius-10;
                if (y_text<5)
                    y_text = y + radius + 2;

                CvScalar text_color = cvScalar(0,0,255);
                string text_string = displayed_class;
                
                if (state==STATE_TRAINING)
                {
                    text_color = cvScalar(255,0,0);
                    text_string = "look: " + displayed_class;
                }

                if (state==STATE_CLASSIFYING)
                {
                    text_string = displayed_class;
                }

                cvRectangle(img->getIplImage(),cvPoint(x-radius,y-radius),cvPoint(x+radius,y+radius),cvScalar(0,255,0),2);
                cvPutText(img->getIplImage(),text_string.c_str(),cvPoint(x-radius,y_text),&font,text_color);
                
                //cvCircle(img->getIplImage(),cvPoint(x,y),3,cvScalar(255,0,0),3);
            }
        }

        if (port_out_show.getOutputCount()>0)
            port_out_show.write(*img);

        mutex.post();
    }

    bool set_displayed_class(string _displayed_class)
    {
        displayed_class = _displayed_class;
        return true;
    }

    bool set_radius_crop_human(int _radius_crop)
    {
        radius_crop_human = _radius_crop;
        return true;
    }

    bool set_radius_crop_robot(int _radius_crop)
    {
        radius_crop_robot = _radius_crop;
        return true;
    }
    
    bool set_mode(int _mode)
    {
        mutex.wait();
        mode = _mode;
        mutex.post();
            
        return true;
    }
    
    bool set_state(int _state)
    {
        mutex.wait();
        state = _state;
        mutex.post();
            
        return true;
    }

    bool get_displayed_class(string &_displayed_class)
    {
        _displayed_class = displayed_class;
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
        mutex.wait();

        port_in_img.interrupt();
        port_in_blobs.interrupt();

        port_out_show.interrupt();
        port_out_crop.interrupt();
        port_out_img.interrupt();
        port_out_imginfo.interrupt();

        port_rpc_are_get_hand.interrupt();

        mutex.post();
    }

    virtual bool releaseThread()
    {
        mutex.wait();

        port_in_img.close();
        port_in_blobs.close();

        port_out_show.close();
        port_out_crop.close();
        port_out_img.close();
        port_out_imginfo.close();

        port_rpc_are_get_hand.close();

        mutex.post();

        return true;
    }

};


class ScorerThread: public RateThread
{

private:

    ResourceFinder                      &rf;
    Semaphore                           mutex;
    bool                                verbose;

    //input
    BufferedPort<Bottle>                port_in_scores;

    //output
    Port                                port_out_confidence;

    int                                 buffer_size;
    list<Bottle>                        scores_buffer;
    string                              predicted_class;

    int                                 confidence_width;
    int                                 confidence_height;


public:
    ScorerThread(ResourceFinder &_rf)
        :RateThread(5),rf(_rf)
    {
    }

    virtual bool threadInit()
    {
        verbose = rf.check("verbose");

        string name = rf.find("name").asString().c_str();

        buffer_size = rf.check("buffer_size", Value(30)).asInt();
        confidence_width = rf.check("confidence_width",Value(800)).asInt();
        confidence_height = rf.check("confidence_height",Value(500)).asInt();

        //Ports
        //-----------------------------------------------------------
        //input
        port_in_scores.open(("/"+name+"/scores:i").c_str());

        //output
        port_out_confidence.open(("/"+name+"/confidence:o").c_str());
        //------------------------------------------------------------

        predicted_class = "?";

        return true;
    }


    virtual void run()
    {

        mutex.wait();

        Bottle *bot=port_in_scores.read(false);
        
        if (bot==NULL || bot->size()<1)
        {
        	mutex.post();
        	return;
        }

        int n_classes = bot->size();

        int n_classes_old = 0;
        if (scores_buffer.size()>0)
        	n_classes_old = scores_buffer.front().size();

        if (n_classes_old>0 && n_classes!=n_classes_old)
        	scores_buffer.clear();

        scores_buffer.push_back(*bot);

        while (scores_buffer.size()>buffer_size)
        	scores_buffer.pop_front();

        if (scores_buffer.size()<1)
        {
        	std::cout << "There is a problem with buffer_size in streaming mode." << std::endl;
        	mutex.post();
        	return;
        }

        vector<double> class_avg(n_classes,0.0);
        vector<int> class_votes(n_classes,0);

        for (list<Bottle>::iterator score_itr=scores_buffer.begin(); score_itr!=scores_buffer.end(); score_itr++)
        {
            double max_score=-1000.0;
            int max_idx;
            for (int class_idx=0; class_idx<n_classes; class_idx++)
            {
                double s=score_itr->get(class_idx).asList()->get(1).asDouble();
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
        int max_votes_sum=0;

        for(int class_idx=0; class_idx<n_classes; class_idx++)
        {
            class_avg[class_idx]=class_avg[class_idx]/n_classes;
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
            
            max_votes_sum+=class_votes[class_idx];
        }

        predicted_class = scores_buffer.front().get(max_avg_idx).asList()->get(0).asString().c_str();
        if (max_votes/scores_buffer.size()<0.2)
            predicted_class = "?";

        cout << "Scores: " << endl;
        for (int i=0; i<n_classes; i++)
            cout << "[" << scores_buffer.front().get(i).asList()->get(0).asString().c_str() << "]: " << class_avg[i] << " "<< class_votes[i] << endl;
        cout << endl << endl;

        if (port_out_confidence.getOutputCount()>0)
        {
            ImageOf<PixelRgb> img_conf;
            img_conf.resize(confidence_width,confidence_height);
            cvZero(img_conf.getIplImage());

            int max_height = (int)img_conf.height()*0.8;
            int min_height = img_conf.height()-20;

            int width_step = (int)img_conf.width()/n_classes;

            for (int class_idx=0; class_idx<n_classes; class_idx++)
            {
                int class_height=img_conf.height()-((int)max_height*class_votes[class_idx]/max_votes_sum);
                if(class_height>min_height)
                    class_height=min_height;

                cvRectangle(img_conf.getIplImage(),cvPoint(class_idx*width_step,class_height),cvPoint((class_idx+1)*width_step,min_height),cvScalar(155,155,255),CV_FILLED);
                cvRectangle(img_conf.getIplImage(),cvPoint(class_idx*width_step,class_height),cvPoint((class_idx+1)*width_step,min_height),cvScalar(0,0,255),3);
                
                CvFont font;
                cvInitFont(&font,CV_FONT_HERSHEY_SIMPLEX,0.6,0.6,0,2);
                
                cvPutText(img_conf.getIplImage(),scores_buffer.front().get(class_idx).asList()->get(0).asString().c_str(),cvPoint(class_idx*width_step,img_conf.height()-5),&font,cvScalar(255,255,255));
            }

            port_out_confidence.write(img_conf);
        }


        mutex.post();
    }

    bool clear_hist()
    {
    	ImageOf<PixelRgb> img_conf;
    	img_conf.resize(confidence_width,confidence_height);
    	cvZero(img_conf.getIplImage());

    	port_out_confidence.write(img_conf);

    	set_predicted_class("?");

    	reset_scores();

    	return true;
    }

    bool set_buffer_size(int _bsize)
    {
    	mutex.wait();
    	buffer_size = _bsize;
    	mutex.post();

    	return true;
    }

    bool get_buffer_size(int &_bsize)
    {
    	mutex.wait();
    	_bsize = buffer_size;
    	mutex.post();

    	return true;
    }

    bool set_predicted_class(string _predicted_class)
    {
        mutex.wait();
        predicted_class = _predicted_class;
        mutex.post();

        return true;
    }
    
    bool get_predicted_class(string &_predicted_class)
    {
        mutex.wait();
        _predicted_class = predicted_class;
        mutex.post();

        return true;
    }

    bool reset_scores()
    {
        mutex.wait();
        scores_buffer.clear();
        mutex.post();

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
        mutex.wait();
        port_in_scores.interrupt();
        port_out_confidence.interrupt();
        mutex.post();
    }

    virtual bool releaseThread()
    {
        mutex.wait();
        port_in_scores.close();
        port_out_confidence.close();
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
    CropperThread                   	*thr_cropper;
    ScorerThread                        *thr_scorer;

    // rpc ARE
    RpcClient                           port_rpc_are;
    RpcClient                           port_rpc_are_get;
    RpcClient                           port_rpc_are_cmd;

    // rpc linearClassifier
    RpcClient                           port_rpc_classifier;

    bool								recognition_started;

    // rpc human commands
    RpcClient                           port_rpc_human;

    // output
    Port                                port_out_speech;

    double                              observe_human_time_training;

    int                                 mode;
    int                                 state;

private:

    bool set_state(int _state)
    {
        state = _state;
        thr_cropper->set_state(state);
        
        return true;
    }
    
    bool set_mode(int _mode)
    {
        mode=_mode;
        thr_cropper->set_mode(mode);
        
        return true;
    }

    bool speak(string speech)
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

    bool store_human(string class_name)
    {

    	if (!send_doublecmd2rpc_classifier("save", class_name.c_str(), 10))
    	{
    		std::cout << "Classifier busy for saving!" << std::endl;
    		return false;
    	}

    	Time::delay(observe_human_time_training);

    	if (!send_cmd2rpc_classifier("stop", 10))
    	{
    		std::cout << "Classifier busy for stopping to save scores: please make it stops somehow!" << std::endl;
    		return false;
    	}

    	return true;
    }

    bool store_robot(string class_name)
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

    bool observe_robot(string &predicted_class)
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

    	thr_scorer->reset_scores();
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

    		thr_scorer->reset_scores();
    		thr_scorer->set_buffer_size(old_buffer_size);

    		complete_robot();

    		return false;
    	}

    	thr_scorer->get_predicted_class(predicted_class);

    	thr_scorer->reset_scores();
    	thr_scorer->set_buffer_size(old_buffer_size);

    	return true;
    }

    bool complete_robot()
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

public:

    ManagerThread(ResourceFinder &_rf)
        :RateThread(10),rf(_rf)
    {
    }

    virtual bool threadInit()
    {

    	verbose = rf.check("verbose");

        string name = rf.find("name").asString().c_str();

        observe_human_time_training = rf.check("observe_human_time_training", Value(20.0)).asDouble();

        // rpc ARE
        port_rpc_are.open(("/"+name+"/are/rpc").c_str());
        port_rpc_are_get.open(("/"+name+"/are/get:io").c_str());
        port_rpc_are_cmd.open(("/"+name+"/are/cmd:io").c_str());

        // rpc linearClassifier
        port_rpc_classifier.open(("/"+name+"/classifier:io").c_str());

        // out speech
        port_out_speech.open(("/"+name+"/speech:o").c_str());

        mutex.wait();

        recognition_started = false;

        mutex.post();

        thr_cropper = new CropperThread(rf);
        thr_cropper->start();

        thr_scorer = new ScorerThread(rf);
        thr_scorer->start();

        set_mode(MODE_HUMAN);

        set_state(STATE_CLASSIFYING);

        return true;
    }

    virtual void run()
    {

        mutex.wait();

        string current_class;

        if (port_rpc_classifier.getOutputCount()==0)
        {
        	std::cout << "Please connect a classifier to start!" << std::endl;
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

        	set_state(STATE_CLASSIFYING);

        }

        if (state==STATE_TRAINING)
        {

            thr_cropper->get_displayed_class(current_class);

        	speak("Ok, show me this wonderful " + current_class);

        	bool ok;
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
        		break;
        	}

        	case MODE_HUMAN:
        	{

        		ok = store_human(current_class.c_str());
        		if (!ok)
        		{
        			std::cout << "observe_human() failed!" << std::endl;
        			set_state(STATE_CLASSIFYING);
        			break;
        		}
        		break;
        	}

        	}

        	if (!ok)
        	{
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

    bool execReq(const Bottle &command, Bottle &reply)
    {
        switch(command.get(0).asVocab())
        {
            default:
                return false;
        }
    }

    bool send_cmd2rpc_classifier(string cmdstring, int Ntrials)
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

    bool send_doublecmd2rpc_classifier(string cmdstring1, string cmdstring2, int Ntrials)
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

    bool execHumanCmd(Bottle &command, Bottle &reply)
    {
        bool ok;

        mutex.wait();

    	switch(command.get(0).asVocab())
        {

            case CMD_TRAIN:
            {
                if (command.size()<1)
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
                	if (!send_doublecmd2rpc_classifier("forget", class_forget.c_str(), 10))
                	{
                		ok = false;
                		reply.addString("Classifier busy for forgetting one object!");
                		break;
                	}

                	speak("I have forgotten " + class_forget);

                	recognition_started = false;

                	ok = true;
                	reply.addVocab(ACK);
                	break;

                }
                else
                {
                    
                	string class_forget = command.get(1).asString().c_str();
                	if (!send_doublecmd2rpc_classifier("forget", "all", 10))
                	{
                		ok = false;
                		reply.addString("Classifier busy for forgetting all objects!");
                		break;
                	}

                	speak("I have forgotten " + class_forget);

                	recognition_started = false;

                	ok = true;
                	reply.addVocab(ACK);
                	break;

                }
            }

            case CMD_WHATISTHIS:
            {
            	set_state(STATE_WHATISTHIS);

            	ok = true;
            	reply.addVocab(ACK);
            	break;

            }

        }

    	mutex.post();

    	if (ok)
        return true;
    	else
    		return false;

    }

    virtual void interrupt()
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

    virtual bool releaseThread()
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
        string name = rf.find("name").asString().c_str();

        Time::turboBoost();

        manager_thr = new ManagerThread(rf);
        manager_thr->start();

        port_rpc_human.open(("/"+name+"/human:io").c_str());
        port_rpc.open(("/"+name+"/rpc").c_str());
        attach(port_rpc);
        return true;
    }

    virtual bool interruptModule()
    {
        manager_thr->interrupt();
        port_rpc_human.interrupt();
        port_rpc.interrupt();
        return true;
    }

    virtual bool close()
    {
        manager_thr->stop();
        delete manager_thr;

        port_rpc_human.close();
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
        {
            manager_thr->execHumanCmd(human_cmd,reply);
            port_rpc_human.reply(reply);
        }

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
   rf.setDefaultContext("onthefly-recognition");
   rf.setDefaultConfigFile("config.ini");
   rf.configure(argc,argv);
   rf.setDefault("name","onTheFlyRecognition");
   ManagerModule mod;

   return mod.runModule(rf);
}

