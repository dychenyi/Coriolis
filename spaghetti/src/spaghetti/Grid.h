
#ifndef SPAGHETTI_GRID
#define SPAGHETTI_GRID

#include "Graph.h"

namespace spaghetti{

struct PlanarCoord{
    unsigned x, y;

    PlanarCoord(){}
    PlanarCoord(unsigned xi, unsigned yi) : x(xi), y(yi) {}
};
struct VolumeCoord{
    unsigned x, y, layer;

    VolumeCoord(){}
    VolumeCoord(unsigned xi, unsigned yi, unsigned li) : x(xi), y(yi), layer(li) {}
};

struct CNet : NetProperties{
    std::vector<std::vector<PlanarCoord> > components;
};

struct RoutedCNet : CNet{
    std::vector<std::pair<PlanarCoord, PlanarCoord> > routing;
};

class BidimensionalGrid : public RoutableGraph{
    public:
    typedef PlanarCoord GridCoord;
    enum options{
        HPins = 0x0001,
        VPins = 0x0002,
        Bidir = HPins | VPins
    };

    private:
    unsigned xdim, ydim;

    EdgeIndex getTurnEdgeIndex        ( unsigned x, unsigned y ) const;
    EdgeIndex getHorizontalEdgeIndex  ( unsigned x, unsigned y ) const;
    EdgeIndex getVerticalEdgeIndex    ( unsigned x, unsigned y ) const;

    GridCoord getCoord(VertexIndex v) const;

    VertexIndex getHorizontalVertexIndex ( unsigned x, unsigned y ) const;
    VertexIndex getVerticalVertexIndex   ( unsigned x, unsigned y ) const;

    void steinerRouteNet  ( EdgeCostFunction, Net & n );

    public:
    BidimensionalGrid(unsigned x, unsigned y, std::vector<CNet> const &, unsigned mask=HPins | VPins);

    void pushNet( CNet, unsigned mask = Bidir );

    // Access and modify the edges
    EdgeProperties const & getTurnEdge        ( unsigned x, unsigned y ) const;
    EdgeProperties const & getHorizontalEdge  ( unsigned x, unsigned y ) const;
    EdgeProperties const & getVerticalEdge    ( unsigned x, unsigned y ) const;
    EdgeProperties       & getTurnEdge        ( unsigned x, unsigned y );
    EdgeProperties       & getHorizontalEdge  ( unsigned x, unsigned y );
    EdgeProperties       & getVerticalEdge    ( unsigned x, unsigned y );

    // Routing using 2D-aware algorithms
    void steinerRoute ( EdgeCostFunction );

    // Get the results
    std::vector<RoutedCNet> getRouting() const;

    unsigned getXDim() const{ return xdim; }
    unsigned getYDim() const{ return ydim; }

    Cost avgHCost            ( EdgeEvalFunction ) const;
    Cost maxHCost            ( EdgeEvalFunction ) const;
    Cost qAvgHCost           ( EdgeEvalFunction ) const;
    Cost avgVCost            ( EdgeEvalFunction ) const;
    Cost maxVCost            ( EdgeEvalFunction ) const;
    Cost qAvgVCost           ( EdgeEvalFunction ) const;
    Cost avgTCost            ( EdgeEvalFunction ) const;
    Cost maxTCost            ( EdgeEvalFunction ) const;
    Cost qAvgTCost           ( EdgeEvalFunction ) const;

};

class MultilayerGrid : public RoutableGraph{
    public:
    typedef VolumeCoord GridCoord;

    MultilayerGrid( unsigned x, unsigned y, std::vector<bool> layerDirections );

    // Access and modify the edges
    EdgeProperties   & getViaEdge  ( unsigned x,     unsigned y,   unsigned lowerLayer );
    VertexProperties & getVertex   ( unsigned x,     unsigned y,   unsigned layer );
    EdgeProperties   & getEdge     ( unsigned track, unsigned ind, unsigned layer );

    // Create nets by x,y

};

} // End namespace spaghetti

#endif

