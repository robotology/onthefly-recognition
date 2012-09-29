#include <iostream>
#include <string>
#include "svm.h"
#include <vector>
#include <fstream>
#define Malloc(type,n) (type *)malloc((n)*sizeof(type))

using namespace std;

class SVMNonLin
{
    private:
        
        struct svm_problem  SVMProblem;
        string className;
        struct svm_model* modelSVM;

    public:
        
        SVMNonLin(string className);
        ~SVMNonLin();

        void trainModel(std::vector<std::vector<double>> &features, vector<double> &labels, svm_parameter &param);
        double predictModel(vector<double> features);
        svm_parameter initialiseParam(int solverTYPE=C_SVC, double C=1.0, double eps=0.1, int kernelType=RBF, double gamma=0.1);
        void saveModel(string pathFile);
        void loadModel(string pathFile);
        vector<vector<double>> readFeatures(string filePath);

};
