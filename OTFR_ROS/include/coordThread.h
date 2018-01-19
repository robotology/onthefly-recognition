/*
 * THREAD for transformation COORD FROM YARP TO ROS format
 * Copyright (C) 2017 iCub Facility - Istituto Italiano di Tecnologia
 * Author: Tanis Mar
 * email: tanis.mar@iit.it
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

#ifndef __OTFRCOORDTHRD_H__
#define __OTFRCOORDTHRD_H__

// Includes
#include <iostream>
#include <yarp/os/RateThread.h>
#include <yarp/os/Network.h>


// YARP - iCub libs
#include <yarp/os/all.h>
#include <yarp/sig/all.h>

// ROS related libs
#include <yarp/os/Subscriber.h>
#include <yarp/os/Publisher.h>
#include <std_msgs_Header.h>
#include <std_msgs_Int32MultiArray.h>

class CoordThread: public yarp::os::RateThread
{
protected:
    // EXTERNAL VARIABLES: set on call
    std::string name;


    std_msgs_Int32MultiArray                        coords_msg;
    yarp::os::Publisher<std_msgs_Int32MultiArray>   pub_coords;

    yarp::os::BufferedPort<yarp::os::Bottle>        coordsPortIn;

    bool gettingInput;


public:
    // CONSTRUCTOR
    CoordThread(int period, const std::string &_name);
    // INIT
    virtual bool threadInit();
    // RUN
    virtual void run();
    // RELEASE
    virtual void threadRelease();
   
};

#endif

