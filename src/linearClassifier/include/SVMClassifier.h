#include <iostream>
#include <string>
#include "linear.h"
#include <vector>
#include <fstream>
#define Malloc(type,n) (type *)malloc((n)*sizeof(type))

using namespace std;

class  SVMClassifier
{
    private:
        
        struct problem  SVMProblem;
        string className;

    public:
        struct model* modelLinearSVM;
        SVMClassifier(string className);

        void trainModel(std::vector<std::vector<double>> &features, vector<double> &labels, parameter &param, int bias=1);
        double predictModel(vector<double> features);
        parameter initialiseParam(int solverTYPE=L2R_L2LOSS_SVC_DUAL, double C=1.0, double eps=0.1);
        void saveModel(string pathFile);
        void loadModel(string pathFile);
        vector<vector<double>> readFeatures(string filePath);

};
