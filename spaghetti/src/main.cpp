#include "spaghetti/Grid.h"
#include <iostream>

using namespace spaghetti;
using namespace std;

int main(){
    cout << "Dimensions:" << endl;
    unsigned xdim, ydim;
    cin >> xdim >> ydim;
    cout << "Capacities:" << endl;
    Capacity xCap, yCap;
    cin >> xCap >> yCap;
    cout << "Costs:" << endl;
    Cost xCst, yCst;
    cin >> xCst >> yCst;
    cout << "Turn penalty:" << endl;
    Cost turnCost;
    cin >> turnCost;

    cout << "Number of nets:" << endl;
    int netCount;
    cin >> netCount;

    std::vector<CNet> nets;
    for(int n=0; n<netCount; ++n){
        cout << "Number of pins of net " << n << " and their coordinates:"<< endl;
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

    for(int n=0; n<netCount; ++n){
        cout << "Net n°" << n << ":" << endl;
        for(auto const & comp : nets[n].components){
            for(auto const & c : comp)
                cout << "(" << c.x << ", " << c.y << "), ";
            cout << endl;
        }
    }

    auto grid = BidimensionalGrid(xdim, ydim, nets);

    cout << "Final grid has " << grid.vertexCount() << " vertices, " << grid.edgeCount() << " edges and " << grid.netCount() << " nets. "<< endl;
    for(unsigned x=0; x<xdim; ++x){
        for(unsigned y=0; y<ydim; ++y){
            cout << "At (" << x << ", " << y << ") via, cap: " << grid.getTurnEdge(x, y).capacity 
                 << ", cost: " << grid.getTurnEdge(x, y).basicCost << endl;
            grid.getTurnEdge(x, y).capacity = (xCap+yCap);
            grid.getTurnEdge(x, y).basicCost = turnCost;
            if(x+1 < xdim){
                cout << "At (" << x << ", " << y << ") H, cap: " << grid.getHorizontalEdge(x, y).capacity 
                     << ", cost: " << grid.getHorizontalEdge(x, y).basicCost << endl;
                grid.getHorizontalEdge(x, y).capacity  = xCap;
                grid.getHorizontalEdge(x, y).basicCost = xCst;
            }
            if(y+1 < ydim){
                cout << "At (" << x << ", " << y << ") V, cap: " << grid.getVerticalEdge(x, y).capacity 
                     << ", cost: " << grid.getVerticalEdge(x, y).basicCost << endl;
                grid.getVerticalEdge(x, y).capacity  = yCap;
                grid.getVerticalEdge(x, y).basicCost = yCst;
            }
        }
    }

    grid.selfcheck();

    grid.biroute(
        [](EdgeProperties const & e, Capacity d)->Cost{
            Cost ratio1 = (Cost) e.demand / (Cost) e.capacity;
            Cost ratio2 = (Cost) (e.demand+d) / (Cost) e.capacity;
            return e.basicCost * ( 1.0f + 0.01f * (ratio2*ratio2-ratio1*ratio1));
        }
    );

    auto res = grid.getRouting();
    for(int n=0; n<netCount; ++n){
        cout << "Routing for net " << n << endl;
        for(auto e : res[n]){
            cout << "\t(" << e.first.x << " " << e.first.y << " -> " << e.second.x << " " << e.second.y << ")" << endl;
        }
    }

    return 0;
}


