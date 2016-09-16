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

#include <CropperThread.h>
#include <definitions.h>

bool CropperThread::threadInit()
{

	verbose = rf.check("verbose");

	string name = rf.find("name").asString().c_str();

    mutex.wait();

	radius_robot = rf.check("radius_robot",Value(80)).asInt();
	radius_human = rf.check("radius_human",Value(40)).asInt();
	radius = radius_human;

	skip_frames = rf.check("skip_frames",Value(5)).asInt();
	frame_counter = 0;

    displayed_class="?";
    true_class="?";

    mutex.post();

	//input
	port_in_img.open(("/"+name+"/img:i").c_str());
	port_in_blobs.open(("/"+name+"/blobs:i").c_str());
	port_in_roi.open(("/"+name+"/roi:i").c_str());

	//output
	port_out_show.open(("/"+name+"/show:o").c_str());
	port_out_crop.open(("/"+name+"/crop:o").c_str());
	port_out_img.open(("/"+name+"/img:o").c_str());
	port_out_imginfo.open(("/"+name+"/imginfo:o").c_str());

	//rpc
	port_rpc_are_get_hand.open(("/"+name+"/are/hand:io").c_str());

	return true;
}


void CropperThread::run()
{

	mutex.wait();

	Image *img=port_in_img.read(false);
	if(img==NULL)
	{
		mutex.post();
		return;
	}
	cv::Mat img_mat = cv::cvarrToMat(img->getIplImage());

	Stamp stamp;
	port_in_img.getEnvelope(stamp);

	bool crop_found = false;
	bool crop_valid = false;
	int x=-1;
	int y=-1;
	int pixelCount=0;
	int tlx = -1;
	int tly = -1;
	int brx  = -1;
	int bry = -1;

	if (mode==MODE_HUMAN)
	{
		if (crop_mode==CROP_MODE_RADIUS)
		{
			Bottle *blobs = port_in_blobs.read(false);
			if (blobs!=NULL)
			{
				Bottle *window = blobs->get(0).asList();
				x = window->get(0).asInt();
				y = window->get(1).asInt();
				pixelCount = window->get(2).asInt();
				radius = radius_human;
				crop_found = true;
			}
		}
		else if (crop_mode==CROP_MODE_BBDISP)
		{
			Bottle *roi = port_in_roi.read(false);
			if (roi!=NULL)
			{
				Bottle *window = roi->get(0).asList();
				tlx = window->get(0).asInt();
				tly = window->get(1).asInt();
				brx = window->get(2).asInt();
				bry = window->get(3).asInt();
				crop_found = true;
			}
		}
	}
	else if (mode==MODE_ROBOT)
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

			radius = radius_robot;

			if (0<x && x<img_mat.cols && 0<y && y<img_mat.rows)
				crop_found=true;
		}
	}

	if (crop_found)
	{
		if (crop_mode==CROP_MODE_RADIUS)
		{
			int r = std::min(radius,x);
			r = std::min(r,y);
			r = std::min(r,img_mat.cols-x-1);
			r = std::min(r,img_mat.rows-y-1);
			if (r>10)
			{
				tlx = x-r;
				tly = y-r;
				brx = x+r;
				bry = y+r;
				crop_valid = true;
			}
		}
		else if (crop_mode==CROP_MODE_BBDISP)
		{
			tlx = std::max(tlx, 0);
			tly = std::max(tly, 0);
			brx = std::max(brx, 0);
			bry = std::max(bry, 0);
			tlx = std::min(tlx, img_mat.cols);
			tly = std::min(tly, img_mat.rows);
			brx = std::min(brx, img_mat.cols);
			bry = std::min(bry, img_mat.rows);
			if (brx-tlx>20 && bry-tly>20)
				crop_valid = true;

		}

		if (crop_valid)
		{
			cv::Rect img_ROI = cv::Rect(cv::Point( tlx, tly ), cv::Point( brx, bry ));
			ImageOf<PixelRgb> img_crop;
			img_crop.resize(img_ROI.width, img_ROI.height);
			cv::Mat img_crop_mat = cv::cvarrToMat((IplImage*)img_crop.getIplImage());
			img_mat(img_ROI).copyTo(img_crop_mat);

			if (port_out_crop.getOutputCount()>0)
			{
				if (state==STATE_TRAINING)
				{
					if (frame_counter<skip_frames)
					{
						frame_counter++;
						//cout << "skip" << endl;
					} else
					{
						port_out_crop.setEnvelope(stamp);
						port_out_crop.write(img_crop);
						frame_counter = 0;
					}

				} else if (state==STATE_CLASSIFYING || state==STATE_WHATISTHIS)
				{
					frame_counter = 0;
					port_out_crop.setEnvelope(stamp);
					port_out_crop.write(img_crop);
				}
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
				if (crop_mode==CROP_MODE_RADIUS)
				{
					imginfo.addInt(x);
					imginfo.addInt(y);
					imginfo.addInt(pixelCount);
				}
				else if (crop_mode==CROP_MODE_BBDISP)
				{
					imginfo.addInt(tlx);
					imginfo.addInt(tly);
					imginfo.addInt(brx);
					imginfo.addInt(bry);
				}
				imginfo.addString(true_class.c_str());
				port_out_imginfo.write(imginfo);
			}

			int y_text, x_text;
			y_text = tly-10;
			x_text = tlx;
			if (y_text<5)
				y_text = bry+2;

			cv::Scalar text_color;
			string text_string = displayed_class;

			if (state==STATE_TRAINING)
			{
				text_color = cv::Scalar(255,0,0);
				text_string = "look: " + displayed_class;
			}
			else if (state==STATE_CLASSIFYING || state==STATE_WHATISTHIS)
			{
				text_string = displayed_class;
				text_color = cv::Scalar(0,255,0);
			}

			cv::rectangle(img_mat,cv::Point(tlx,tly),cv::Point(brx,bry),cv::Scalar(0,255,0),2);
			cv::putText(img_mat,text_string.c_str(),cv::Point(x_text,y_text), cv::FONT_HERSHEY_SIMPLEX, 0.8, text_color, 2);
		}
	}

	if (port_out_show.getOutputCount()>0)
		port_out_show.write(*img);

	mutex.post();

}

bool CropperThread::set_displayed_class(string _displayed_class)
{
	mutex.wait();
	displayed_class = _displayed_class;
	mutex.post();
	return true;
}

bool CropperThread::set_radius_human(int _radius)
{
	if (_radius>0)
	{
		mutex.wait();
		radius_human = _radius;
		mutex.post();
		return true;
	}
	else
		return false;
}

bool CropperThread::set_radius_robot(int _radius)
{
	if (_radius>0)
	{
		mutex.wait();
		radius_robot = _radius;
		mutex.post();
		return true;
	}
	else
		return false;
}

bool CropperThread::set_skip_frames(int _frames)
{
	if (_frames>=0)
	{
		mutex.wait();
		skip_frames = _frames;
		mutex.post();
		return true;
	}
	else
		return false;
}

int CropperThread::get_radius_human()
{
	mutex.wait();
	int r = radius_human;
	mutex.post();
	return r;
}

int CropperThread::get_radius_robot()
{
	mutex.wait();
	int r = radius_robot;
	mutex.post();
	return r;
}

int CropperThread::get_skip_frames()
{
	mutex.wait();
	int s = skip_frames;
	mutex.post();
	return s;
}

bool CropperThread::set_mode(int _mode)
{
	if (_mode!=MODE_HUMAN && _mode!=MODE_ROBOT)
		return false;

	mutex.wait();
	mode = _mode;
	mutex.post();

	return true;
}

bool CropperThread::set_state(int _state)
{
	if (_state!=STATE_TRAINING && _state!=STATE_CLASSIFYING && _state!=STATE_WHATISTHIS)
		return false;

	mutex.wait();
	state = _state;
	mutex.post();

	return true;
}

bool CropperThread::set_crop_mode(int _crop_mode)
{
	if (_crop_mode!=CROP_MODE_RADIUS && _crop_mode!=CROP_MODE_BBDISP)
		return false;

	mutex.wait();
	crop_mode = _crop_mode;
	mutex.post();

	return true;
}

bool CropperThread::get_displayed_class(string &_displayed_class)
{
	mutex.wait();
	_displayed_class = displayed_class;
	mutex.post();

	return true;
}

bool CropperThread::execReq(const Bottle &command, Bottle &reply)
{
	switch(command.get(0).asVocab())
	{
	default:
		return false;
	}
}

void CropperThread::interrupt()
{
//	mutex.wait();

	port_in_img.interrupt();
	port_in_blobs.interrupt();
	port_in_roi.interrupt();

	port_out_show.interrupt();
	port_out_crop.interrupt();
	port_out_img.interrupt();
	port_out_imginfo.interrupt();

	port_rpc_are_get_hand.interrupt();

//	mutex.post();
}

bool CropperThread::releaseThread()
{
//	mutex.wait();

	port_in_img.close();
	port_in_blobs.close();
	port_in_roi.close();

	port_out_show.close();
	port_out_crop.close();
	port_out_img.close();
	port_out_imginfo.close();

	port_rpc_are_get_hand.close();

//	mutex.post();

	return true;
}
