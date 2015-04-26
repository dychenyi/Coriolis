
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

struct CNet{
    Capacity demand;
    std::vector<std::vector<PlanarCoord> > components;
};

class BidimensionalGrid : RoutableGraph{
    unsigned xdim, ydim;

    public:
    typedef PlanarCoord GridCoord;

    BidimensionalGrid(unsigned x, unsigned y, std::vector<CNet> const &);

    // Access and modify the edges
    EdgeProperties & getTurnEdge        ( unsigned x, unsigned y );
    EdgeProperties & getHorizontalEdge  ( unsigned x, unsigned y );
    EdgeProperties & getVerticalEdge    ( unsigned x, unsigned y );

    // Get the results
    std::vector<std::vector<std::pair<GridCoord, GridCoord> > > getRouting() const;

    private:
    GridCoord getCoord(VertexIndex v) const;
};

class MultilayerGrid : RoutableGraph{
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

