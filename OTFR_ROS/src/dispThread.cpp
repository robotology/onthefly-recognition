#include "dispThread.h"

using namespace std;
using namespace yarp::os;

// Empty constructor
DispThread::DispThread(int period, const string &_name):RateThread(period), name(_name){}

// Initialize Variables
bool DispThread::threadInit()
{
    // Inits

    if (!dispOutPort.open("/OTFR_ROS/disp:o"))     {
     printf("Problems opening disp:o port\n");
     return false;
    }

    /* subscribe to ROS topics */
    if (!subs_disp.topic("/multisense/left/disparity_image")) {
           cerr<< "/multisense/left/disparity_image" << endl;
           return -1;       }

    cout << "Disp thread running" << endl;

    gettingInput = true;

    return true;
}

void DispThread::run()
{
    if(subs_disp.getInputCount()) {
        subs_disp.read(dispIn);

        if (gettingInput == false){
            cout << "Reading ROS disparity input of size (HxW): " << dispIn.image.height << " x " << dispIn.image.width << endl;
            gettingInput = true;
        }

        typeDepth &dispOut  = dispOutPort.prepare();
        dispOut.setExternal(dispIn.image.data.data(), dispIn.image.width, dispIn.image.height);
        dispOutPort.write();
    }else{
        if (gettingInput == true){
            cout << "Waiting to read ROS disparity (might take up to a minute)" << endl;
            gettingInput = false;
        }
    }

    return;
}


void DispThread::threadRelease()
{
    cout << "Closing Disp Thread" << endl;
    this->stop();

    return;
}
