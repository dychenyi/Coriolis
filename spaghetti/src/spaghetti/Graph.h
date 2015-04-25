
#ifndef SPAGHETTI_GRAPH
#define SPAGHETTI_GRAPH

#include <vector>
#include <array>
#include <cstdint>
#include <functional>
#include <limits>

namespace spaghetti{

typedef std::uint32_t EdgeIndex;
typedef std::uint32_t VertexIndex;
typedef std::int32_t  Capacity;
typedef float         Cost;

const EdgeIndex nullEdgeIndex   = std::numeric_limits<EdgeIndex>::max();
const EdgeIndex nullVertexIndex = std::numeric_limits<VertexIndex>::max();

struct EdgeProperties{
    Capacity capacity, demand;
    Cost basic_cost, history_cost;
};

struct Edge : EdgeProperties{
    std::array<VertexIndex, 2> vertices;
};

struct VertexProperties{
    Cost basic_cost, history_cost;
};

struct Vertex : VertexProperties{
    std::array<EdgeIndex, 4> edges;
};

typedef std::function<Cost (Edge   const &, Capacity demand)> EdgeCostFunction;
typedef std::function<Cost (Vertex const &, Capacity demand)> VertexCostFunction;
typedef std::function<bool (Edge   const &)> EdgePredicate;

struct Net{
    Capacity demand;
    std::vector<std::vector<VertexIndex> > initialComponents;
    std::vector<EdgeIndex> routing;

    void selfcheck() const;
};

class Graph{
    std::vector<Edge>   edges;
    std::vector<Vertex> vertices;

    protected:
    Edge   & getEdge    ( EdgeIndex );
    Vertex & getVertex  ( EdgeIndex );

    bool isRouted( Net const & n ) const;
    std::vector<std::vector<VertexIndex> > getConnectedComponents( Net const & n ) const;

    void unrouteUnusedEdges ( Net & n );
    void unrouteSimplePaths ( Net & n );

    void biroute  ( EdgeCostFunction, Net & n );
    void triroute ( EdgeCostFunction, Net & n );

    struct NeighbourAccess{
        EdgeIndex   edge;
        VertexIndex vertex;
    };
    std::array<NeighbourAccess, 4> neighbours(VertexIndex v) const;

    public:
    Graph(unsigned vertexCount, unsigned edgeCount);

    void selfcheck() const;
};

class RoutableGraph : public Graph{
    protected:
    std::vector<Net>    nets;

    public:
    void unroute  ( EdgePredicate );

    void biRoute  ( EdgeCostFunction );
    void triRoute ( EdgeCostFunction );

    bool rebiroute    ( EdgePredicate, EdgeCostFunction );
    bool retriroute   ( EdgePredicate, EdgeCostFunction );
    bool reroutePaths ( EdgePredicate, EdgeCostFunction );

    bool isRouted   () const;
    bool noOverflow ( EdgePredicate ) const;

    RoutableGraph(unsigned vertexCount, unsigned edgeCount);
};

}

#endif


