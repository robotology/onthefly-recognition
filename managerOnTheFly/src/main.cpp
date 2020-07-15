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

#include <opencv2/highgui/highgui.hpp>

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

#include <ManagerThread.h>

class ManagerModule: public RFModule
{
protected:

	ManagerThread       *manager_thr;
	RpcServer           port_rpc_human;
	Port                port_rpc;

public:

	ManagerModule() {}

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

