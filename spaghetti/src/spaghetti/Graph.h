
#ifndef SPAGHETTI_GRAPH
#define SPAGHETTI_GRAPH

#include <vector>
#include <array>
#include <functional>

#include "Common.h"

namespace spaghetti{

struct EdgeProperties{
    Capacity  capacity;
    Capacity  demand;
    Cost      basicCost;
    Cost      historyCost;

    EdgeProperties(Capacity cap = 0, Cost c = 1.0)
  : capacity(cap), demand(0), basicCost(c), historyCost(0.0) {}
};

struct Edge : EdgeProperties{
    std::array<VertexIndex, 2> vertices;

    Edge() : EdgeProperties() {
        vertices[0] = nullVertexIndex;
        vertices[1] = nullVertexIndex;
    }
};

struct VertexProperties{
    Cost basicCost;
    Cost historyCost;

    VertexProperties(Cost c = 0.0)
  : basicCost(c), historyCost(0.0) {}
};

struct Vertex : VertexProperties{
    std::array<EdgeIndex, 4> edges;

    Vertex() : VertexProperties(){
        for(int i=0; i<4; ++i)
            edges[i] = nullEdgeIndex;
    }    
};

struct NetProperties{
    Capacity demand;
    Cost     cost;

    NetProperties() : demand(1), cost(1.0f) {}
};

struct Net : NetProperties{
    std::vector<std::vector<VertexIndex> > initialComponents;
    std::vector<EdgeIndex> routing;

    void selfcheck() const;
};

typedef std::function<Cost ( EdgeProperties   const &, NetProperties const & )> EdgeCostFunction;
typedef std::function<Cost ( VertexProperties const &, NetProperties const & )> VertexCostFunction;
typedef std::function<Cost ( EdgeProperties   const & )> EdgeEvalFunction;
typedef std::function<Cost ( VertexProperties const & )> VertexEvalFunction;
typedef std::function<bool ( EdgeProperties   const & )> EdgePredicate;

class Graph{
    protected:
    std::vector<Vertex> vertices;
    std::vector<Edge>   edges;

    bool isNetRouted( Net const & n ) const;
    std::vector<std::vector<VertexIndex> > getConnectedComponents( Net const & n ) const;

    void unrouteNet             ( Net & n );
    void unrouteOverflowedEdges ( Net & n, EdgePredicate edgePredicate );
    void unrouteSelectedEdges   ( Net & n, std::vector<EdgeIndex> const & sortedEdges );
    void unrouteDuplicatedEdges ( Net & n );
    void unrouteInternalEdges   ( Net & n );
    void unrouteUnusedEdges     ( Net & n );
    void unrouteSimplePaths     ( Net & n );

    void birouteNet  ( EdgeCostFunction, Net & n );
    void trirouteNet ( EdgeCostFunction, Net & n );

    struct NeighbourAccess{
        EdgeIndex   edge;
        VertexIndex vertex;
    };
    std::array<NeighbourAccess, 4> neighbours(VertexIndex v) const;

    public:
    Graph(unsigned vertexCnt, unsigned edgeCnt) : vertices(vertexCnt), edges(edgeCnt) {}

    void selfcheck() const;
    VertexIndex vertexCount () const{ return vertices.size(); }
    EdgeIndex   edgeCount   () const{ return edges.size(); }

    Cost avgCost            ( EdgeEvalFunction ) const;
    Cost maxCost            ( EdgeEvalFunction ) const;
    Cost qAvgCost           ( EdgeEvalFunction ) const;

    bool overflow           ( EdgePredicate ) const;
    EdgeIndex overflowCount ( EdgePredicate ) const;

    void updateHistoryCosts ( EdgePredicate edgePredicate, Cost mul=1.0f, Cost inc=0.5f );

    void routeEdge( EdgeIndex e, Net & n );
};

class RoutableGraph : public Graph{
    protected:
    std::vector<Net>    nets;

    // TODO: ordering for the nets (based on history, size, or whatever)

    public:
    void unroute  ( EdgePredicate );

    void biroute  ( EdgeCostFunction );
    void triroute ( EdgeCostFunction );

    void rebiroute    ( EdgePredicate, EdgeCostFunction );
    void retriroute   ( EdgePredicate, EdgeCostFunction );

    bool isRouted          ()                const;
    bool isCorrectlyRouted ( EdgePredicate ) const;

    size_t netCount        ()                const{ return nets.size(); }

    RoutableGraph(unsigned vertexCnt, unsigned edgeCnt) : Graph(vertexCnt, edgeCnt) {}
};

} // End namespace spaghetti

#endif


