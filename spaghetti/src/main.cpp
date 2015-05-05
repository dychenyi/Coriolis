#include "spaghetti/Grid.h"
#include "spaghetti/CostFunctions.h"
#include <iostream>

using namespace spaghetti;
using namespace std;

void outputStats(RoutableGraph const & grid){
    auto costEval = basicEdgeEvalFunction(); 
    cout << "Routing is finished" << endl;
    cout << "Avg cost: " << grid.avgCost(costEval) << endl;
    cout << "Qud cost: " << grid.qAvgCost(costEval) << endl;
    cout << "Max cost: " << grid.maxCost(costEval) << endl;
}

int main(){
    //cout << "Dimensions:" << endl;
    unsigned xdim, ydim;
    cin >> xdim >> ydim;
    //cout << "Costs:" << endl;
    Cost xCst, yCst;
    cin >> xCst >> yCst;
    //cout << "Turn penalty:" << endl;
    Cost turnCost;
    cin >> turnCost;

    std::vector<std::vector<Capacity> > hcaps, vcaps, tcaps;

    for(unsigned x=0; x<xdim; ++x){
        std::vector<Capacity> newCaps;
        for(unsigned y=0; y+1<ydim; ++y){
            Capacity cur;
            cin >> cur;
            newCaps.push_back(cur);
        }
        vcaps.push_back(newCaps);
    }
    for(unsigned y=0; y<ydim; ++y){
        std::vector<Capacity> newCaps;
        for(unsigned x=0; x+1<xdim; ++x){
            Capacity cur;
            cin >> cur;
            newCaps.push_back(cur);
        }
        hcaps.push_back(newCaps);
    }
    for(unsigned x=0; x<xdim; ++x){
        std::vector<Capacity> newCaps;
        for(unsigned y=0; y<ydim; ++y){
            Capacity cur;
            cin >> cur;
            newCaps.push_back(cur);
        }
        tcaps.push_back(newCaps);
    }

    //cout << "Number of nets:" << endl;
    int netCount;
    cin >> netCount;

    std::vector<CNet> nets;
    for(int n=0; n<netCount; ++n){
        //cout << "Number of pins of net " << n << " and their coordinates:"<< endl;
        int nbComps;
        cin >> nbComps;
        CNet net;
        net.demand = 1;
        for(int i=0; i<nbComps;++i){
            PlanarCoord cur;
            cin >> cur.x >> cur.y;
            net.components.push_back(std::vector<PlanarCoord>(1, cur));
        }
        nets.push_back(net);
    }

    //for(int n=0; n<netCount; ++n){
    //    cout << "Net n°" << n << ":" << endl;
    //    for(auto const & comp : nets[n].components){
    //        for(auto const & c : comp)
    //            cout << "(" << c.x << ", " << c.y << "), ";
    //        cout << endl;
    //    }
    //}

    auto grid = BidimensionalGrid(xdim, ydim, nets);

    cout << "Final grid has " << grid.vertexCount() << " vertices, " << grid.edgeCount() << " edges and " << grid.netCount() << " nets. "<< endl;
    for(unsigned x=0; x<xdim; ++x){
        for(unsigned y=0; y<ydim; ++y){
            grid.getTurnEdge(x, y).capacity = tcaps[x][y];
            grid.getTurnEdge(x, y).basicCost = turnCost;
            //cout << "At (" << x << ", " << y << ") via, cap: " << grid.getTurnEdge(x, y).capacity 
            //     << ", cost: " << grid.getTurnEdge(x, y).basicCost << endl;
            if(x+1 < xdim){
                grid.getHorizontalEdge(x, y).capacity  = hcaps[y][x];
                grid.getHorizontalEdge(x, y).basicCost = xCst;
                //cout << "At (" << x << ", " << y << ") H, cap: " << grid.getHorizontalEdge(x, y).capacity 
                //     << ", cost: " << grid.getHorizontalEdge(x, y).basicCost << endl;
            }
            if(y+1 < ydim){
                grid.getVerticalEdge(x, y).capacity  = vcaps[x][y];
                grid.getVerticalEdge(x, y).basicCost = yCst;
                //cout << "At (" << x << ", " << y << ") V, cap: " << grid.getVerticalEdge(x, y).capacity 
                //     << ", cost: " << grid.getVerticalEdge(x, y).basicCost << endl;
            }
        }
    }

    grid.selfcheck();

    grid.biroute(
        dualthresholdEdgeCostFunction(1.01)
    );
    outputStats(grid);
    grid.rebiroute(
        overflowPredicate(0.8),
        dualthresholdEdgeCostFunction(1.2)
    );
    outputStats(grid);
    grid.rebiroute(
        overflowPredicate(0.8),
        dualthresholdEdgeCostFunction(2.0)
    );
    outputStats(grid);
    grid.rebiroute(
        overflowPredicate(0.8),
        dualthresholdEdgeCostFunction(4.0)
    );
    grid.rebiroute(
        overflowPredicate(0.8),
        dualthresholdEdgeCostFunction(10.0)
    );
    outputStats(grid);
    grid.rebiroute(
        overflowPredicate(0.8),
        dualthresholdEdgeCostFunction(100.0)
    );
    outputStats(grid);
    grid.rebiroute(
        overflowPredicate(0.8),
        dualthresholdEdgeCostFunction(10000.0)
    );
    outputStats(grid);


    if(grid.isRouted())
        cout << "All nets are connected" << endl;
    else
        cout << "Some nets have not been routed!!" << endl;
    auto res = grid.getRouting();
    //for(int n=0; n<netCount; ++n){
    //    cout << "Routing for net " << n << endl;
    //    for(auto e : res[n]){
    //        cout << "\t(" << e.first.x << " " << e.first.y << " -> " << e.second.x << " " << e.second.y << ")" << endl;
    //    }
    //}

    return 0;
}


