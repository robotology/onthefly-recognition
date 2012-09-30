#include <iostream>
#include <string>
#include <yarp/sig/all.h>
#include <yarp/math/Math.h>
#include <yarp/os/all.h>
#include <yarp/os/RFModule.h>
#include <yarp/os/Network.h>
#include <yarp/os/Thread.h>
#include <iCub/ctrl/math.h>
#include <yarp/os/Os.h>
#include "SVMLinear.h"
#include "SVMNonLin.h"
#include <yarp/os/Semaphore.h>
#include "dirent.h"


#define STATE_DONOTHING         0
#define STATE_SAVING            1
#define STATE_RECOGNIZING       2

using namespace std; 
using namespace yarp::os; 
using namespace yarp::sig;
using namespace yarp::math;


class linearClassifierThread : public Thread
{
private:
        string inputFeatures;
        string outputPortName;

        string currPath;
        string pathObj;
        Port *commandPort;
        BufferedPort<Bottle> featuresPort;
        BufferedPort<Bottle> outputPort;
        Semaphore * mutex;
        fstream objFeatures;

        vector<pair<string,vector<string> > > knownObjects;
        vector<SVMLinear> linearClassifiers;
        vector<int> datasetSizes;
        vector<vector<vector<double> > > Features;
        vector<vector<double > > bufferScores;
        vector<vector<int > > countBuffer;
        int bufferSize;

        int currentState;

        void checkKnownObjects();
        int getdir(string dir, vector<string> &files);

public:

    linearClassifierThread(yarp::os::ResourceFinder &rf,Port* commPort);
    void prepareObjPath(string objName);
    bool threadInit();
    void threadRelease();
    void run(); 
    void onStop();
    void createFullPath(const char * path);
    void stopAll();
    bool loadFeatures();
    bool trainClassifiers();
    bool startRecognition();

};
