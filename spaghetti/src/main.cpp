#include "spaghetti/Grid.h"
#include "spaghetti/CostFunctions.h"
#include <iostream>

#include <unordered_map>
#include <unordered_set>

using namespace spaghetti;
using namespace std;

void outputStats(BidimensionalGrid const & grid){
    auto costEval = edgeDemandFunction(); 
    if(grid.isRouted())
        cout << "Routing has been done" << endl;
    else
        cout << "Some nets are left unrouted" << endl;
    cout << "Avg demand:  \t" << grid.avgCost(costEval)  << endl;
    cout << "Quad demand: \t" << grid.qAvgCost(costEval) << endl;
    cout << "Max demand:\t" << grid.maxCost(costEval)  << endl;
    cout << "Avg H demand:  \t" << grid.avgHCost(costEval)  << endl;
    cout << "Avg V demand:  \t" << grid.avgVCost(costEval)  << endl;
    cout << "Avg T demand:  \t" << grid.avgTCost(costEval)  << endl;
    auto ovEval = overflowPredicate();
    cout << "Overflows: " << grid.overflowCount(ovEval) << endl; 
}

void outputNet(BidimensionalGrid const & grid, RoutedCNet const & n){
    std::unordered_map<unsigned, int> pins;
    std::unordered_set<unsigned>      routes;

    
    auto getInd = [&](PlanarCoord a)->unsigned{return a.y*grid.getXDim() + a.x; };

    for(int i=0; i<n.components.size(); ++i){
        for(PlanarCoord p : n.components[i])
            pins.emplace(getInd(p), i);
    }
    for(auto e : n.routing){
        routes.emplace(getInd(e.first));
        routes.emplace(getInd(e.second));
    }

    for(unsigned y=0; y<grid.getXDim(); ++y){
        for(unsigned x=0; x<grid.getXDim(); ++x){
            unsigned ind = getInd(PlanarCoord(x, y));
            if(pins.count(ind) != 0){
                cout << pins[ind];
            }
            else if(routes.count(ind) != 0){
                cout << "#";
            }
            else{
                cout << " ";
            }
        }
        cout << endl;
    }
    cout << endl;
}

int main(){
    unsigned xdim, ydim;
    cin >> xdim >> ydim;
    Cost xCst, yCst;
    cin >> xCst >> yCst;
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
        net.cost = 1.0f; net.demand = 1;
        for(int i=0; i<nbComps;++i){
            PlanarCoord cur;
            cin >> cur.x >> cur.y;
            net.components.push_back(std::vector<PlanarCoord>(1, cur));
        }
        nets.push_back(net);
    }

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
        dualthresholdEdgeCostFunction(0.1)
    );
    grid.steinerRoute(
        dualthresholdEdgeCostFunction(0.1)
    );
    outputStats(grid);
    float mul = 0.1;
    while(not grid.isCorrectlyRouted(overflowPredicate())){
        grid.updateHistoryCosts(overflowPredicate());
        grid.rebiroute(
            overflowPredicate(1.0),
            //dualthresholdEdgeCostFunction(mul)
            thresholdEdgeCostFunction(mul)
        );
        outputStats(grid);
        mul *= 2.0f;
    }
    if(grid.isRouted())
        cout << "All nets are connected" << endl;
    else
        cout << "Some nets have not been routed!!" << endl;
    auto res = grid.getRouting();
    for(int n=0; n<netCount; ++n){
        cout << "Pins for net " << n << endl;
        for(auto const & comp : nets[n].components){
            for(auto const & c : comp)
                cout << "(" << c.x << ", " << c.y << "), ";
            cout << endl;
        }

        cout << "Routing for net " << n << endl;
        outputNet(grid, res[n]);
        //for(auto e : res[n].routing){
        //    cout << "\t(" << e.first.x << " " << e.first.y << " -> " << e.second.x << " " << e.second.y << ")" << endl;
        //}
    }

    return 0;
}


