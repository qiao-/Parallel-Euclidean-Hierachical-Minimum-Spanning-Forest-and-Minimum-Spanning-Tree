#include <iostream>
#include "ConfigParams.h"
#include "Node.h"
#include "GridOfNodes.h"
#include "NIter.h"
#include "ViewGrid.h"


#include "TestMstBoruvka.h"

using namespace std;
using namespace components;
using namespace meshing;

#define TEST_CODE  0
#define SECTION_PARAMETRES  0

int main(int argc, char *argv[])
{
    char* fileData;
    char* fileSolution;
    char* fileStats;
    char* fileConfig;
    //! qiao 092015 add
    char* fileCalibSt;

    /*
     * Lecture des fichiers d'entree
     */
    if (argc <= 1)
    {
        fileData = "input.data";
        fileSolution = "output.data";
        fileStats = "output.stats";
        fileConfig = "config.cfg";
        fileCalibSt = "calib.txt";
    }
    else
        if (argc == 2)
        {
            fileData = argv[1];
            fileSolution = "output.data";
            fileStats = "output.stats";
            fileConfig = "config.cfg";
            fileCalibSt = "calib.txt";
        }
        else
            if (argc == 3)
            {
                fileData = argv[1];
                fileSolution = argv[2];
                fileStats = "output.stats";
                fileConfig = "config.cfg";
                fileCalibSt = "calib.txt";
            }
            else
                if (argc == 4)
                {
                    fileData = argv[1];
                    fileSolution = argv[2];
                    fileStats = argv[3];
                    fileConfig = "config.cfg";
                    fileCalibSt = "calib.txt";
                }
                else
                    if (argc >= 5)
                    {
                        fileData = argv[1];
                        fileSolution = argv[2];
                        fileStats = argv[3];
                        fileConfig = argv[4];
                        fileCalibSt = argv[5];
                    }

    /*
     * Lecture des parametres
     */
    ConfigParams params(fileConfig);
    params.readConfigParameters();



    if (params.functionModeChoice == 1) {
        cout << "TEST Parallel Euclidean Minimum Spanning Forest: " << endl;
        TestMstBoruvka t(fileData, fileSolution, fileStats, params);
        t.initialize();
        t.run();
        cout << "Fin de test " << params.functionModeChoice << endl;
    }

    return 0;
}//main

