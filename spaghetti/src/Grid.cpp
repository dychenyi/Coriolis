
#include "spaghetti/Grid.h"
#include "spaghetti/UnionFindMap.h"

#include <cassert>
#include <cmath>
#include <unordered_set>

namespace spaghetti{

BidimensionalGrid::BidimensionalGrid(unsigned x, unsigned y, std::vector<CNet> const & netsByCoords, unsigned mask) :
    RoutableGraph(2*x*y, (3 * x * y) - x - y), xdim(x), ydim(y)
{
    assert(mask & (HPins | VPins));
    for(auto n : netsByCoords){
        pushNet(n, mask);
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

void BidimensionalGrid::pushNet( CNet n, unsigned mask ){
    Net newNet;
    for(auto comp : n.components){
        std::vector<VertexIndex> newComp;
        for(auto vertex : comp){
            // Push two vertices: one for the upper layer and one for the lower layer
            if(mask & HPins)
                newComp.push_back( getHorizontalVertexIndex (vertex.x, vertex.y) );
            if(mask & VPins)
                newComp.push_back( getVerticalVertexIndex   (vertex.x, vertex.y) );
        }
        newNet.initialComponents.push_back(newComp);
    }
    newNet.demand = n.demand;
    newNet.cost = n.cost;
    nets.push_back(newNet);
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

EdgeProperties const & BidimensionalGrid::getTurnEdge        ( unsigned x, unsigned y ) const{
    return edges[getTurnEdgeIndex(x, y)];
}
EdgeProperties const & BidimensionalGrid::getHorizontalEdge  ( unsigned x, unsigned y ) const{
    return edges[getHorizontalEdgeIndex(x, y)];
}
EdgeProperties const & BidimensionalGrid::getVerticalEdge    ( unsigned x, unsigned y ) const{
    return edges[getVerticalEdgeIndex(x, y)];
}
EdgeProperties & BidimensionalGrid::getTurnEdge        ( unsigned x, unsigned y ){
    return edges[getTurnEdgeIndex(x, y)];
}
EdgeProperties & BidimensionalGrid::getHorizontalEdge  ( unsigned x, unsigned y ){
    return edges[getHorizontalEdgeIndex(x, y)];
}
EdgeProperties & BidimensionalGrid::getVerticalEdge    ( unsigned x, unsigned y ){
    return edges[getVerticalEdgeIndex(x, y)];
}

bool BidimensionalGrid::isTurnEdge       ( EdgeIndex e ) const{
    return e < xdim * ydim;
}
bool BidimensionalGrid::isHorizontalEdge ( EdgeIndex e ) const{
    return e >= (2*xdim-1)*ydim;
}
bool BidimensionalGrid::isVerticalEdge   ( EdgeIndex e ) const{
    return (not isTurnEdge(e)) and (not isHorizontalEdge(e));
}

BidimensionalGrid::GridCoord BidimensionalGrid::getCoord(VertexIndex v) const{
    if(v >= xdim * ydim)
        v -= xdim*ydim;
    return GridCoord(v/ydim, v%ydim);
}

std::vector<RoutedCNet> BidimensionalGrid::getRouting() const{
    std::vector<RoutedCNet> ret;
    for(auto const & n : nets){
        RoutedCNet cur(static_cast<NetProperties>(n));
        for(auto const & comp : n.initialComponents){
            std::vector<PlanarCoord> newComp;
            for(VertexIndex v : comp)
                newComp.push_back(getCoord(v));
            cur.components.push_back(newComp);
        }
        for(EdgeIndex e : n.routing){
            if(not isTurnEdge(e))
                cur.routing.emplace_back(getCoord(edges[e].vertices[0]), getCoord(edges[e].vertices[1]));
        }
        ret.push_back(cur);
    }
    return ret;
}

std::vector<RoutedCNet> BidimensionalGrid::getPrunedRouting() const{
    std::vector<RoutedCNet> ret;
    for(auto const & n : nets){
        RoutedCNet cur(static_cast<NetProperties>(n));

        // Get all vertices corresponding to contacts
        std::unordered_set<VertexIndex> contactVertices;
        std::unordered_set<EdgeIndex> allEdges(n.routing.begin(), n.routing.end());

        for(auto const & comp : n.initialComponents){
            cur.components.emplace_back();
            for(VertexIndex v : comp){
                cur.components.back().push_back(getCoord(v));
                contactVertices.emplace(v);
            }
        }

        // Get all vertices corresponding to turns, which are contacts as well
        for(EdgeIndex e : n.routing){
            if(isTurnEdge(e))
               contactVertices.emplace(getVertexRepr(edges[e].vertices[0]));
        }

        // Find groups of edges connected by non-contact vertices
        UnionFindMap<EdgeIndex> uf;
        for(EdgeIndex e : n.routing){
            if(isTurnEdge(e)) continue;

            uf.find(e); // Add to the UF set
            for(VertexIndex v : edges[e].vertices){
                if(contactVertices.count(v) == 0){ // The vertex is not a contact
                    for(EdgeIndex ec : vertices[v].edges){
                        if(allEdges.count(ec) != 0){ // This other edge is used by the net too
                            uf.merge(e, ec);
                        }
                    }
                }
            }
        }

        // Get the groups of edges on the grid that can be rearranged into a single straight edge
        for(std::vector<EdgeIndex> const & finalEdge : uf.getConnectedComponents()){
            PlanarCoord min(std::numeric_limits<unsigned>::max(), std::numeric_limits<unsigned>::max()),
                        max(0, 0);
            assert(not finalEdge.empty());
            for(EdgeIndex e : finalEdge){
                PlanarCoord f = getCoord(edges[e].vertices[0]),
                            s = getCoord(edges[e].vertices[1]);
                PlanarCoord tmpmin(std::min(f.x, s.x), std::min(f.y, s.y));
                PlanarCoord tmpmax(std::max(f.x, s.x), std::max(f.y, s.y));
                max.x = std::max(max.x, tmpmax.x); max.y = std::max(max.y, tmpmax.y);
                min.x = std::min(min.x, tmpmin.x); min.y = std::min(min.y, tmpmin.y);
            }
            assert(min.x == max.x or min.y == max.y);
            cur.routing.emplace_back(min, max);
        }

        ret.push_back(cur);
    }
    return ret;

}

Cost BidimensionalGrid::avgHCost   ( EdgeEvalFunction edgeEvalFunction ) const{
    Cost tot = 0.0f;
    for(unsigned y=0; y<ydim; ++y){
        for(unsigned x=0; x+1<xdim; ++x){
            tot += edgeEvalFunction(getHorizontalEdge(x, y));
        }
    }
    if(edges.size() == 0) return 0.0f;
    else                  return tot/(ydim*(xdim-1));
}
Cost BidimensionalGrid::maxHCost   ( EdgeEvalFunction edgeEvalFunction ) const{
    Cost tot = 0.0f;
    for(unsigned y=0; y<ydim; ++y){
        for(unsigned x=0; x+1<xdim; ++x){
            tot = std::max(edgeEvalFunction(getHorizontalEdge(x, y)), tot);
        }
    }
    return tot;
}
Cost BidimensionalGrid::qAvgHCost  ( EdgeEvalFunction edgeEvalFunction ) const{
    Cost tot = 0.0f;
    for(unsigned y=0; y<ydim; ++y){
        for(unsigned x=0; x+1<xdim; ++x){
            Cost cur = edgeEvalFunction(getHorizontalEdge(x, y));
            tot += cur*cur;
        }
    }
    if(edges.size() == 0) return 0.0f;
    else                  return std::sqrt(tot/(ydim*(xdim-1)));
}

Cost BidimensionalGrid::avgVCost   ( EdgeEvalFunction edgeEvalFunction ) const{
    Cost tot = 0.0f;
    for(unsigned x=0; x<xdim; ++x){
        for(unsigned y=0; y+1<ydim; ++y){
            tot += edgeEvalFunction(getVerticalEdge(x, y));
        }
    }
    if(edges.size() == 0) return 0.0f;
    else                  return tot/(xdim*(ydim-1));
}
Cost BidimensionalGrid::maxVCost   ( EdgeEvalFunction edgeEvalFunction ) const{
    Cost tot = 0.0f;
    for(unsigned x=0; x<xdim; ++x){
        for(unsigned y=0; y+1<ydim; ++y){
            tot = std::max(edgeEvalFunction(getVerticalEdge(x, y)), tot);
        }
    }
    return tot;
}
Cost BidimensionalGrid::qAvgVCost  ( EdgeEvalFunction edgeEvalFunction ) const{
    Cost tot = 0.0f;
    for(unsigned x=0; x<xdim; ++x){
        for(unsigned y=0; y+1<ydim; ++y){
            Cost cur = edgeEvalFunction(getVerticalEdge(x, y));
            tot += cur*cur;
        }
    }
    if(edges.size() == 0) return 0.0f;
    else                  return std::sqrt(tot/(xdim * (ydim-1)));
}

Cost BidimensionalGrid::avgTCost   ( EdgeEvalFunction edgeEvalFunction ) const{
    Cost tot = 0.0f;
    for(unsigned x=0; x<xdim; ++x){
        for(unsigned y=0; y<ydim; ++y){
            tot += edgeEvalFunction(getTurnEdge(x, y));
        }
    }
    if(edges.size() == 0) return 0.0f;
    else                  return tot/(xdim*ydim);
}
Cost BidimensionalGrid::maxTCost   ( EdgeEvalFunction edgeEvalFunction ) const{
    Cost tot = 0.0f;
    for(unsigned x=0; x<xdim; ++x){
        for(unsigned y=0; y<ydim; ++y){
            tot = std::max(edgeEvalFunction(getTurnEdge(x, y)), tot);
        }
    }
    return tot;
}
Cost BidimensionalGrid::qAvgTCost  ( EdgeEvalFunction edgeEvalFunction ) const{
    Cost tot = 0.0f;
    for(unsigned x=0; x<xdim; ++x){
        for(unsigned y=0; y<ydim; ++y){
            Cost cur = edgeEvalFunction(getTurnEdge(x, y));
            tot += cur*cur;
        }
    }
    if(edges.size() == 0) return 0.0f;
    else                  return std::sqrt(tot/(xdim*ydim));
}




} // End namespace spaghetti

