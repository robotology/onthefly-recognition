/*
 * OTFR_ROS COLLECTOR MODULE
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

#ifndef __OTFR_ROS_H__
#define __OTFR_ROS_H__

// Includes
#include <iostream>

// YARP - iCub libs
#include <yarp/os/all.h>
#include <yarp/sig/all.h>
#include <yarp/os/Node.h>

// ROS related libs
#include <OTFR_ROS_IDLServer.h>

// Threads
#include "imThread.h"
#include "dispThread.h"
#include "coordThread.h"


/**********************************************************
    PUBLIC METHODS
/**********************************************************/

/**********************************************************/
class OTFR_ROS : public yarp::os::RFModule, public OTFR_ROS_IDLServer
{
protected:
    /* module parameters */
    std::string name;                     // string containing module name

    /* Ports RPC */
    yarp::os::RpcServer rpcInPort;                                      // port to handle incoming commands
    ImThread *imThrd;
    DispThread *dispThrd;
    CoordThread *coordThrd;


    /* ROS related types: */


    /* creates a node for the image and another for the disparity */
    yarp::os::Node *node_yarp;

    bool img_flag;
    bool disp_flag;

    /* class variables */

    bool verb;
    bool closing;

    /* functions*/

    // Private Functions

    // Helper functions

public:

    // RPC Accesible methods    
    bool						quit();

    // RF modules overrides
    bool						configure(yarp::os::ResourceFinder &rf);
    bool						interruptModule();
    bool						close();
    bool						updateModule();
   // void                        onRead( yarp::sig::ImageOf<yarp::sig::PixelRgb> &img );
    double						getPeriod();

    // thrift connection //
    bool						attach(yarp::os::RpcServer &source);
};

#endif
//empty line to make gcc happy
