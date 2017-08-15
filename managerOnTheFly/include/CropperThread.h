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

#ifndef CROPPERTHREAD_H_
#define CROPPERTHREAD_H_

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
#include <opencv2/imgproc.hpp>
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

#include <definitions.h>

class CropperThread: public RateThread
{

private:

	ResourceFinder                      &rf;
	Semaphore                           mutex;
	bool                                verbose;

	//input
	BufferedPort<Image>                 port_in_img;
	BufferedPort<Bottle>                port_in_blobs;
	BufferedPort<Bottle>                port_in_roi;

	//output
	Port                                port_out_show;
	Port                                port_out_crop;
	Port                                port_out_img;
	Port                                port_out_imginfo;

	//rpc
	RpcClient                           port_rpc_are_get_hand;

	int                                 radius;
	int                                 radius_robot;
	int                                 radius_human;

	int									skip_frames;
	int									frame_counter;

	string                              displayed_class;
	string                              true_class;

	int                                 mode;
	int                                 state;
	int									crop_mode;

public:

	CropperThread(ResourceFinder &_rf) : RateThread(5), rf(_rf) { }

	virtual bool threadInit();

	virtual void run();

	bool set_displayed_class(string _displayed_class);

	bool set_radius_human(int _radius);

	bool set_radius_robot(int _radius);

	bool set_skip_frames(int _skip_frames);

	int get_radius_human();

	int get_radius_robot();

	int get_skip_frames();

	bool set_mode(int _mode);

	bool set_state(int _state);

	bool set_crop_mode(int _crop_mode);

	bool get_displayed_class(string &_displayed_class);

	bool execReq(const Bottle &command, Bottle &reply);

	virtual void interrupt();

	virtual bool releaseThread();

};

#endif /* CROPPERTHREAD_H_ */
