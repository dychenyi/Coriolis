
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
                newComp.push_back( getHorizontalVertexIndex (vertex.x, vertex.y) );
                newComp.push_back( getVerticalVertexIndex   (vertex.x, vertex.y) );
            }
            newNet.initialComponents.push_back(newComp);
        }
        newNet.demand = n.demand;
        nets.push_back(newNet);
    }

    // Create all relevant connections for vertices and turn (i.e. via) edges
    for(unsigned x=0; x<xdim; ++x){
        for(unsigned y=0; y<ydim; ++y){
            VertexIndex hIndex = getHorizontalVertexIndex (x, y);
            VertexIndex vIndex = getVerticalVertexIndex   (x, y);
            Vertex & h = vertices[hIndex];
            Vertex & v = vertices[vIndex];

            if(x>0) // First horizontal edge
                h.edges[0] = getHorizontalEdgeIndex(x-1, y);
            if(x+1 < xdim) // Second horizontal edge
                h.edges[1] = getHorizontalEdgeIndex(x, y);

            if(y>0) // First vertical edge
                v.edges[0] = getVerticalEdgeIndex(x, y-1);
            if(y+1 < ydim) // Second vertical edge
                v.edges[1] = getVerticalEdgeIndex(x, y);

            // Via edge
            EdgeIndex tIndex = getTurnEdgeIndex(x, y);
            h.edges[3] = tIndex;
            v.edges[3] = tIndex;

            Edge & t = edges[tIndex];
            t.vertices[0] = hIndex;
            t.vertices[1] = vIndex;
        }
    }

    // Vertical edges
    for(unsigned x=0; x<xdim; ++x){
        for(unsigned y=0; y+1<ydim; ++y){
            Edge & ve = edges[ getVerticalEdgeIndex(x, y) ];
            ve.vertices[0] = getVerticalVertexIndex(x, y);
            ve.vertices[1] = getVerticalVertexIndex(x, y+1);
        }
    }

    // Horizontal edges
    for(unsigned y=0; y<ydim; ++y){
        for(unsigned x=0; x+1<xdim; ++x){
            Edge & he = edges[ getHorizontalEdgeIndex(x, y) ];
            he.vertices[0] = getHorizontalVertexIndex(x, y);
            he.vertices[1] = getHorizontalVertexIndex(x+1, y);
        }
    }
}

EdgeIndex BidimensionalGrid::getTurnEdgeIndex        ( unsigned x, unsigned y ) const{
    assert(x < xdim and y < ydim);
    return xdim*y + x;
}
EdgeIndex BidimensionalGrid::getHorizontalEdgeIndex  ( unsigned x, unsigned y ) const{
    assert(x+1 < xdim and y < ydim);
    return xdim*ydim + x*ydim + y; // But only ydim*(xdim-1) of them
}
EdgeIndex BidimensionalGrid::getVerticalEdgeIndex    ( unsigned x, unsigned y ) const{
    assert(x < xdim and y+1 < ydim);
    return (2*xdim-1)*ydim + y*xdim + x; // But only xdim*(ydim-1) of them
}

VertexIndex BidimensionalGrid::getHorizontalVertexIndex ( unsigned x, unsigned y ) const{
    return x*ydim + y;
}
VertexIndex BidimensionalGrid::getVerticalVertexIndex   ( unsigned x, unsigned y ) const{
    return (x+xdim)*ydim + y;
}

EdgeProperties & BidimensionalGrid::getTurnEdge        ( unsigned x, unsigned y ){
    EdgeIndex ind = getTurnEdgeIndex(x, y);
    assert(ind < edges.size());
    return static_cast<EdgeProperties&>(edges[ind]);
}
EdgeProperties & BidimensionalGrid::getHorizontalEdge  ( unsigned x, unsigned y ){
    EdgeIndex ind = getHorizontalEdgeIndex(x, y);
    assert(ind < edges.size());
    return static_cast<EdgeProperties&>(edges[ind]);
}
EdgeProperties & BidimensionalGrid::getVerticalEdge    ( unsigned x, unsigned y ){
    EdgeIndex ind = getVerticalEdgeIndex(x, y);
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

