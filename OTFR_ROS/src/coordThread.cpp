#include "coordThread.h"

using namespace std;
using namespace yarp::os;

// Empty constructor
CoordThread::CoordThread(int period, const string &_name):RateThread(period), name(_name){}

// Initialize Variables
bool CoordThread::threadInit()
{
    // Inits
    // Open input YARP port
    if (!coordsPortIn.open("/OTFR_ROS/coords:i"))     {
        printf("Problems opening coords:i port\n");
        return false;
    }

    // Open output ROS publisher to topic
    if (!pub_coords.topic("/OTFR_ROS/coords_out")) {
         cerr<< "Failed to create publisher to /OTFR_ROS/coords_out \n";
         return -1;
     }

    cout << "Coord thread running" << endl;

    gettingInput = true;

    return true;
}

void CoordThread::run()
{
    Bottle *coordBot = coordsPortIn.read(false);
    if (coordBot!=NULL){

        // Read coordinates from YARP port
        int u = coordBot->get(0).asList()->get(0).asInt();
        int v = coordBot->get(0).asList()->get(1).asInt();
        //cout << "Reading coordinates (" << u << ", " << v << ")." << endl;


        // Write them into message and publish in ROS topic
        coords_msg.clear();

        coords_msg.data.push_back(u);
        coords_msg.data.push_back(v);

        /* publish it to the topic */
        pub_coords.write(coords_msg);
    }


    return;
}



void CoordThread::threadRelease()
{
    cout << "Closing Coord Thread" << endl;
    this->stop();

    return;
}
