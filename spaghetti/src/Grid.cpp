
#include "spaghetti/Grid.h"

#include <cassert>

namespace spaghetti{

BidimensionalGrid::BidimensionalGrid(unsigned x, unsigned y, std::vector<CNet> const & netsByCoords) :
    RoutableGraph(2*x*y, (3 * x * y) - x - y), xdim(x), ydim(y)
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

EdgeProperties & BidimensionalGrid::getTurnEdge        ( unsigned x, unsigned y ){
    EdgeIndex ind = xdim*y + x;
    assert(ind < edges.size());
    return static_cast<EdgeProperties&>(edges[ind]);
}
EdgeProperties & BidimensionalGrid::getHorizontalEdge  ( unsigned x, unsigned y ){
    EdgeIndex ind = xdim*ydim + x*ydim + y; // But only ydim*(xdim-1) of them
    assert(ind < edges.size());
    return static_cast<EdgeProperties&>(edges[ind]);
}
EdgeProperties & BidimensionalGrid::getVerticalEdge    ( unsigned x, unsigned y ){
    return edges[(2*xdim-1)*ydim + y*xdim + x];
    EdgeIndex ind = (2*xdim-1)*ydim + y*xdim + x; // But only xdim*(ydim-1) of them
    assert(ind < edges.size());
    return static_cast<EdgeProperties&>(edges[ind]);
}

BidimensionalGrid::GridCoord BidimensionalGrid::getCoord(VertexIndex v) const{
    if(v >= xdim * ydim)
        v -= xdim*ydim;
    return GridCoord(v/ydim, v%ydim);
}

std::vector<std::vector<std::pair<PlanarCoord, PlanarCoord> > > BidimensionalGrid::getRouting() const{
    std::vector<std::vector<std::pair<GridCoord, GridCoord> > > ret;
    for(auto const & n : nets){
        std::vector<std::pair<GridCoord, GridCoord> > cur;
        for(EdgeIndex e : n.routing){
            cur.emplace_back(getCoord(edges[e].vertices[0]), getCoord(edges[e].vertices[1]));
        }
        ret.push_back(cur);
    }
    return ret;

}

} // End namespace spaghetti

