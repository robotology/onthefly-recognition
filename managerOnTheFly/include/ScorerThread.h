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

#ifndef SCORERTHREAD_H_
#define SCORERTHREAD_H_

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

	deque<cv::Scalar> 					histColorsCode;

public:

	ScorerThread(ResourceFinder &_rf) : RateThread(5),rf(_rf) { }

	virtual bool threadInit();

	virtual void run();

	bool clear_hist();

	void draw_hist(vector<int> bins);

	bool set_buffer_size(int _bsize);

	bool get_buffer_size(int &_bsize);

	bool get_predicted_class(string &_predicted_class);

	bool execReq(const Bottle &command, Bottle &reply);

	virtual void interrupt();

	virtual bool releaseThread();

};

#endif /* SCORERTHREAD_H_ */
