
#include "spaghetti/Grid.h"

namespace spaghetti{

BidimensionalGrid::BidimensionalGrid(unsigned x, unsigned y, std::vector<CNet> const & netsByCoords) :
    RoutableGraph(2*x*y, x*y-x-y)
    {
        for(auto n : netsByCoords){
            Net newNet;
            for(auto comp : n.components){
                std::vector<VertexIndex> newComp;
                for(auto vertex : comp){
                    // Push two vertices: one for the upper layer and one for the lower layer
                    newComp.push_back(              vertex.x * ydim + vertex.y);
                    newComp.push_back(xdim * ydim + vertex.x * ydim + vertex.y);
                }
                newNet.initialComponents.push_back(newComp);
            }
            newNet.demand = n.demand;
            nets.push_back(newNet);
        }
    }
}

EdgeProperties & BidimensionalGrid::getTurnEdge        ( unsigned x, unsigned y ){
    return edges[xdim * y + x];
}
EdgeProperties & BidimensionalGrid::getHorizontalEdge  ( unsigned x, unsigned y ){
    return edges[xdim*ydim + x*ydim + y]; // But only ydim*(xdim-1) of them
}
EdgeProperties & BidimensionalGrid::getVerticalEdge    ( unsigned x, unsigned y ){
    return edges[(2*xdim-1)*ydim + y*xdim + x]; // But only xdim*(ydim-1) of them
}

} // End namespace spaghetti

