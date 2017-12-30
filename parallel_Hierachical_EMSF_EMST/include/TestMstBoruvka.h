#ifndef TESTMSTBORUVKA_H
#define TESTMSTBORUVKA_H
/*
 ***************************************************************************
 * Author : Wen-bao Qiao
 * Creation date : Septembre. 2016
 *
 * Note: This application aims at building parallel hierarchical minimum spanning forest or minimum spanning tree
 * of an input N Euclidean points, these points are uniformly distributed in the plane.
 * The algorithm begins with these input points without preparing the N*N edge list. It uses massively parallel local spiral search to find each component's closest outgoing vertex in parallel,
 * and applies parallel Breadth-first Search algorithm to find Borůvka's shortest outgoing edge for each sub-tree.
 * Thanks: thank for the implementation advices from Jean-charles Créput
 ***************************************************************************
 */
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <math.h>
#include <stdlib.h>

#include <cuda_runtime.h>
#include <cuda.h>
//#include <helper_functions.h>
#include <device_launch_parameters.h>
#include <curand_kernel.h>
#include <sm_20_atomic_functions.h>

#include <time.h>
//#include <sys/time.h>

#include "macros_cuda.h"
#include "Node.h"
#include "GridOfNodes.h"
#include "NeuralNet.h"
#include "distances_matching.h"
#include "ViewGrid.h"
#include "Cell.h"
#include "SpiralSearch.h"
#include "Objectives.h"
#include "Trace.h"
#include "ConfigParams.h"
#include "CellularMatrix.h"
#include "MstOperator.h"
#include "SomOperator.h"
#include "ImageRW.h"
#include "Converter.h"
#include "filters.h"

#include "SpiralSearch.h"


using namespace std;
using namespace components;
using namespace operators;

#define use2dMr 0
#define use1dMr 1



namespace meshing
{

/*!
 * \brief The TestSom class
 */
class TestMstBoruvka {
    char* fileData;
    char* fileSolution;
    char* fileStats;
    ConfigParams params;
    Trace trace;

public:
    // Types
    typedef ViewGridQuad ViewG;
    typedef NIterQuad NIter;
    typedef NIterQuadDual NIterDual;

    typedef CellB<CM_DistanceEuclidean,
    CM_ConditionTrue,
    NIter, ViewG> CB;
    typedef CellSpS<CM_DistanceEuclidean,
    CM_ConditionTrue,
    NIter, ViewG> CSpS;

    typedef CellularMatrix<CSpS, ViewG> CMSpS;
    typedef CellularMatrix<CB, ViewG> CMB;

    typedef MstBoruvkaOperator<CMB, CMB, CB, CB, NIter, NIterDual, ViewG, BufferLink2D> MstFloatLinksFloatAdaptive;
    MstFloatLinksFloatAdaptive mstFFLinks;


    // Data
    NN city;
    NN md;


    //! qiao 190615 add to use link for SOM, change the typedef if you want use dynamic buffer links
    NetLinkPointCoord md_links;
    NetLinkPointCoord md_links_gpu;


    ViewG vgd;
    CMB cmd; // for tsp
    CMB cmd_cpu;


    tspResultInfo traceParallelMST;


    TestMstBoruvka(char* fileData, char* fileSolution, char* fileStats, ConfigParams params) :
        fileData(fileData),
        fileSolution(fileSolution),
        fileStats(fileStats),
        params(params), vgd()
    {}

    void initialize() {

        //! WB.Q, read the tsp file into a NN (1D) and get the margin values.
        //! size numbers
        float max_x, max_y, min_x, min_y;

        IRW irw;
        irw.readTSP(fileData, city, min_x, min_y, max_x, max_y); // md: nNodes * 1

        size_t nNodes = city.adaptiveMap.getWidth();
        cout << "num cities = " << nNodes  << endl;
        cout << "max original x, y " << max_x << ", " << max_y << endl;
        cout << "min original x, y " << min_x << ", " << min_y << endl;

        int _w = int(max_x - min_x) + 2;
        int _h = int(max_y - min_y) + 2;
        cout << "city_area_w , city_area_h " << _w << ", " << _h << endl;

        //! chansfer the coordinate system
        float unitX = sqrt(nNodes) / _w;
        float unitY = sqrt(nNodes) / _h;
        cout << "unitX = " << unitX << endl;
        cout << "unitY = " << unitY << endl;


        //!QWB update city coordinates to sart(n)
        md.resize(nNodes, 1);
        float max_xNew = 0;
        float max_yNew = 0;
        float min_xNew = +INFINITY;
        float min_yNew = +INFINITY;
        for (int i = 0; i < nNodes; i++){
            md.adaptiveMap[0][i][0] = (city.adaptiveMap[0][i][0] - min_x) * unitX;
            md.adaptiveMap[0][i][1] = (city.adaptiveMap[0][i][1] - min_y) * unitY;
            if (md.adaptiveMap[0][i][0] >= max_xNew)
                max_xNew = md.adaptiveMap[0][i][0];
            if (md.adaptiveMap[0][i][1] >= max_yNew)
                max_yNew = md.adaptiveMap[0][i][1];
            if (md.adaptiveMap[0][i][0] < min_xNew)
                min_xNew = md.adaptiveMap[0][i][0];
            if (md.adaptiveMap[0][i][1] < min_yNew)
                min_yNew = md.adaptiveMap[0][i][1];
        }

        cout << "max x,y new 0 coor New " << max_xNew << ", " << max_yNew << endl;
        cout << "min x,y new 0 coor New " << min_xNew << ", " << min_yNew << endl;

        //! the new width and height on the new coordianate
        int _wn = int(max_xNew - min_xNew) + 3;
        int _hn = int(max_yNew - min_yNew) + 3;
        cout << "vgd area _w, _h " << _wn << ", " << _hn << endl;




        //! ViewGrid, this vg should be on the new coordinate system, _wn, _hn ~ 2*sqrt(n)
        PointCoord pc(_wn/2, _hn/2);
        int _R = params.levelRadius;
        vgd = ViewG(pc, _wn, _hn, _R); // To be verify, R
        cout << "vgd dual, num of cells , cmd: " << vgd.getWidthDual() << " "
             << vgd.getHeightDual() << ", " << vgd.getWidthDual() *  vgd.getHeightDual() << endl
             << "vgd.base ,  " << vgd.getWidthBase() << ", "
             << vgd.getHeightBase() << ", " << vgd.getWidthBase() * vgd.getHeightBase()<< endl << "sqrt (nNodes) " << sqrt(nNodes) << endl
             << "_wn/2 " << _wn/2 << endl;



        //! cread md_links to evaluate before or after optimization
        md_links.resize(nNodes, 1);
        md_links.adaptiveMap = md.adaptiveMap;

        md_links_gpu.gpuResize(nNodes, 1);
        md_links.gpuCopyHostToDevice(md_links_gpu);


        //! vgd in new coordinate
        cmd.setViewG(vgd);
        cmd.gpuResize(vgd.getWidthDual(), vgd.getHeightDual());
        cmd.K_initialize(vgd);
        cmd_cpu.setViewG(vgd);
        cmd_cpu.resize(vgd.getWidthDual(), vgd.getHeightDual());
        cmd_cpu.K_initialize_cpu(vgd);
        GetDivideAdaptor_CPU<CB> gda(md_links.adaptiveMap.getWidth_cpu(),
                                     md_links.adaptiveMap.getHeight_cpu(),
                                     cmd_cpu.getWidth_cpu(),
                                     cmd_cpu.getHeight_cpu());
        SearchFindCellAdaptor_cpu<CMB > sfa; // test md.activeMap, num of nodes to be inserted into cmd
        OperateInsertAdaptor<NeuralNet, CB> oia; // no use
        NeuralNetLinks<BufferLink2D, CB> nn_cmd;
        nn_cmd.resize(cmd_cpu.getWidth_cpu(), cmd_cpu.getHeight_cpu());
        nn_cmd.adaptiveMap = cmd_cpu;
        cmd_cpu.K_clearCells_cpu();
        mstFFLinks.refreshCell_cpu(cmd_cpu, cmd_cpu, md_links, nn_cmd, gda, sfa, oia);



        cmd_cpu.gpuCopyHostToDevice(cmd);
        cmd_cpu.gpuCopyDeviceToHost(cmd);
        int numNode = 0;
        for (int _y = 0; _y < (cmd_cpu.getHeight_cpu()); ++_y) {
            for (int _x = 0; _x < (cmd_cpu.getWidth_cpu()); ++_x) {
                cout << "cmd_cpu " <<  cmd_cpu[_y][_x].size << endl;
                numNode += cmd_cpu[_y][_x].size;
            }
        }
        cout << "=== check num of nodes inserted into cmd: " << numNode << endl;

        //! copy cmd to gpu
        cmd_cpu.gpuCopyHostToDevice(cmd);

        // initialize mst
        mstFFLinks.initialize(md_links_gpu, cmd, vgd);


        //! trace the results
        trace.initialize("traceMST");

        cout << "initialization end"  << endl;

    }

    void run(){

        traceParallelMST.benchMark = fileData;

        int radiusSearchCells = 0;
        params.readConfigParameter("test_2opt", "radiusSearchCells", radiusSearchCells);

        clock_t cpuMstBegin, cpuMstEnd;
        cpuMstBegin = clock();
        double cpuTotalTime;
        // cuda timer
        cudaEvent_t start, stop;
        cudaEventCreate(&start);
        cudaEventCreate(&stop);
        cudaEventRecord(start, 0);

        mstFFLinks.buildBoruvkaMST_GitHub(radiusSearchCells);

        float elapsedTime;
        cudaEventRecord(stop, 0);
        cudaEventSynchronize(stop);
        cudaEventElapsedTime(&elapsedTime, start, stop);
        cpuMstEnd =  clock();
        cpuTotalTime = cpuMstEnd - cpuMstBegin;
        printf(">> EMST constructon: CUDA Running time is %lf ms\n", elapsedTime);
        printf(">> EMST constructon: CPU Running time is %lf ms\n", cpuTotalTime);
        cout << "EMST done" << endl;


        // trace time
        traceParallelMST.cuda_runtime_som =  elapsedTime;
        traceParallelMST.runTimeCpuSom = cpuTotalTime;


        // copy result
        md_links.gpuCopyDeviceToHost(md_links_gpu);


        //evaluate total mst length
        md_links.fixedMap.resetValue(0);
        float EMSTdis = md_links.evaluateWeightOfSingleTree_Recursive<CM_DistanceEuclidean>();
        cout << "**ddd*****=== mst weight iterative : " << EMSTdis << endl;
        cout << "*******=== num of node being traversed: " << testGridNum<bool>(md_links.fixedMap) << endl;

        // trace dis
        traceParallelMST.somLinksDis = EMSTdis;


        // output
        md_links.write(fileSolution);
        md_links.writeLinks(fileSolution);


        trace.writeStatisticsTsp(traceParallelMST);

        // freeMem
        md_links_gpu.gpuFreeMem();

    }// end run


};
}//namespace meshing

#endif // TESTNETWORKLINK_H
