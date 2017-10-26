/*
 * OnTheFlyRecognition_ROS
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

#include <OTFR_ROS.h>

using namespace std;
using namespace yarp::os;
using namespace yarp::sig;


/************************* RF overwrites ********************************/
/************************************************************************/
bool  OTFR_ROS::configure(ResourceFinder &rf)
{
    img_flag = true;
    disp_flag = true;

    /* creates a node called /yarp/listener */
    node_yarp = new Node("/yarp/OTFR_node");

    // add and initialize the port to send out the features via thrift.
    string name = rf.check("name",Value("OTFR_ROS")).asString().c_str();
    rf.setDefaultContext("OntHeFlyRecognition");

    // Check default .ini variables
    verb = rf.check("verbose",Value(true)).asBool();

    // open rpc port
    bool retRPC = true;
    retRPC =  retRPC && rpcInPort.open("/"+name+"/rpc:i");
    if (!retRPC){
        printf("Problems opening ports\n");
        return false;
    }
    attach(rpcInPort);


    /* CONFIGURE ROS VARIABLES */
    //Threads
    if (img_flag){

        // Start image conversion thread
        imThrd = new ImThread(50, "img");
        if (!imThrd->start())
        {
            delete imThrd;
            imThrd = 0;
            cout << "\nERROR!!! imThrd wasn't instantiated!!\n";
            return false;
        }
        cout << "Img conversion thread istantiated..." << endl;
    }

    if (disp_flag){

        // Start disparity image conversion thread
        dispThrd = new DispThread(50, "disp");
        if (!dispThrd->start())
        {
            delete dispThrd;
            dispThrd = 0;
            cout << "\nERROR!!! dispThrd wasn't instantiated!!\n";
            return false;
        }
        cout << "Disp conversion thread istantiated..." << endl;

        // start coordinate conversion thread
        coordThrd = new CoordThread(20, "coord");
        if (!coordThrd->start())
        {
            delete coordThrd;
            coordThrd = 0;
            cout << "\nERROR!!! coordThrd wasn't instantiated!!\n";
            return false;
        }
        cout << "Coordinate conversion thread istantiated..." << endl;



    }

    /* Module rpc parameters */
    closing = false;

    cout << endl << "Configuring done."<<endl;

    return true;
}

bool  OTFR_ROS::updateModule()
{
     /* read data from the ROS topics */

    return !closing;
}


double  OTFR_ROS::getPeriod()
{
    return 0.05; //module periodicity (seconds)
}


bool  OTFR_ROS::interruptModule()
{
    closing = true;

    cout << "Ports interrupted" << endl;
    return true;
}


bool  OTFR_ROS::close()
{

    if (imThrd)
    {
        imThrd->stop();
        delete imThrd;
        imThrd =  0;
    }

    if (dispThrd)
    {
        dispThrd->stop();
        delete dispThrd;
        dispThrd =  0;

        coordThrd->stop();
        delete coordThrd;
        coordThrd =  0;

    }

    delete node_yarp;

    cout << "Module ports closed" << endl;
    return true;
}

/**************************** THRIFT CONTROL*********************************/
bool  OTFR_ROS::attach(RpcServer &source)
{
    return this->yarp().attachAsServer(source);
}

/**********************************************************
                    PUBLIC METHODS
/**********************************************************/

// RPC Accesible via trhift.
/**********************************************************/


/***************** MORE PUBLIC METHODS **************/

/**********************************************************/
bool OTFR_ROS::quit()
{
    closing = true;
    return true;
}


/**********************************************************
                    PRIVATE METHODS
/**********************************************************/

/***************** Helper Functions *************************************/

/***************** MORE PRIVATE METHOTS**************/


/************************************************************************/
/************************************************************************/
int main(int argc, char * argv[])
{
    Network yarp;
    if (!yarp.checkNetwork())
    {
        printf("YARP server not available!\n");
        return -1;
    }

    OTFR_ROS module;
    ResourceFinder rf;
    rf.setDefaultContext("OnTheFlyRecognition");
    rf.setDefaultConfigFile("OTFR_ROS.ini");
    rf.setDefault("name","OTFR_ROS");
    rf.setVerbose(true);
    rf.configure(argc, argv);

    cout<<"Configure and Start module..."<<endl;
    return module.runModule(rf);

}

