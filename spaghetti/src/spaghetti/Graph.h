
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

struct Edge{
    std::array<VertexIndex, 2> vertices;
    Capacity capacity, demand;
    Cost basic_cost, history_cost;
};

struct Vertex{
    std::array<EdgeIndex, 4> edges;
    Cost basic_cost, history_cost;
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
    bool isRouted( Net const & n ) const;

    void unrouteUnusedEdges ( Net & n );
    void unrouteSimplePaths ( Net & n );

    void biroute  ( EdgeCostFunction, VertexCostFunction, Net & n );
    void triroute ( EdgeCostFunction, VertexCostFunction, Net & n );

    struct NeighbourAccess{
        EdgeIndex edge;
        VertexIndex vertex;
    };
    std::array<NeighbourAccess, 4> neighbours(VertexIndex v) const;

    public:
    void selfcheck() const;
};

class RoutableGraph : public Graph{
    std::vector<Net>    nets;

    public:
    void biRoute  ( EdgeCostFunction, VertexCostFunction );
    void triRoute ( EdgeCostFunction, VertexCostFunction );
    void unroute  ( EdgePredicate );
    bool isRouted   () const;
};

}

#endif


