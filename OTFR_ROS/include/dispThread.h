/*
 * THREAD for ROS IMAGE DISPLAY ON YARP
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

#ifndef __OTFRDISPTHRD_H__
#define __OTFRDISPTHRD_H__

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
#include <stereo_msgs_DisparityImage.h>

class DispThread: public yarp::os::RateThread
{
protected:
    // EXTERNAL VARIABLES: set on call
    std::string name;

    typedef yarp::sig::ImageOf<yarp::sig::PixelFloat> typeDepth;

    yarp::os::BufferedPort<typeDepth >          dispOutPort;             // outputs disparity image
    yarp::os::Subscriber<stereo_msgs_DisparityImage >   subs_disp;
    stereo_msgs_DisparityImage                  dispIn;

    bool gettingInput;


public:
    // CONSTRUCTOR
    DispThread(int period, const std::string &_name);
    // INIT
    virtual bool threadInit();
    // RUN
    virtual void run();
    // RELEASE
    virtual void threadRelease();
   
};

#endif

