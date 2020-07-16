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

#ifndef MANAGERTHREAD_H_
#define MANAGERTHREAD_H_

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

#include <CropperThread.h>
#include <ScorerThread.h>
#include <definitions.h>

class ManagerThread: public RateThread
{

private:

	ResourceFinder                      &rf;
	bool                                verbose;

	Semaphore                           mutex;

	//threads
	CropperThread                       *thr_cropper;
	ScorerThread                        *thr_scorer;

	// rpc ARE
	RpcClient                           port_rpc_are;
	RpcClient                           port_rpc_are_get;
	RpcClient                           port_rpc_are_cmd;

	// rpc linearClassifier
	RpcClient                           port_rpc_classifier;

	bool			                    recognition_started;

	// rpc human commands
	RpcClient                           port_rpc_human;

	// output
	Port                                port_out_speech;

	double                              human_time_training;

	int                                 mode;
	int                                 state;
	int				                    crop_mode;
    bool                                is_face;

private:

	bool set_state(int _state);

	bool set_mode(int _mode);

	bool set_human_time_training(double _t);

	bool set_crop_mode(int _crop_mode);

	bool speak(string speech);

	bool store_human(string class_name);

	bool store_robot(string class_name);

	bool observe_robot(string &predicted_class);

	bool complete_robot();

public:

	ManagerThread(ResourceFinder &_rf) : RateThread(10), rf(_rf) { }

	virtual bool threadInit();

	virtual void run();

	bool execReq(const Bottle &command, Bottle &reply);

	bool send_cmd2rpc_classifier(string cmdstring, int Ntrials);

	bool send_doublecmd2rpc_classifier(string cmdstring1, string cmdstring2, int Ntrials);

	bool execHumanCmd(Bottle &command, Bottle &reply);

	virtual void interrupt();

	virtual bool releaseThread();

};

#endif /* MANAGERTHREAD_H_ */
