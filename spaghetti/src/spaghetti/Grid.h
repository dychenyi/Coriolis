
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

class BidimensionalGrid : public RoutableGraph{
    public:
    typedef PlanarCoord GridCoord;

    private:
    unsigned xdim, ydim;

    EdgeIndex getTurnEdgeIndex        ( unsigned x, unsigned y ) const;
    EdgeIndex getHorizontalEdgeIndex  ( unsigned x, unsigned y ) const;
    EdgeIndex getVerticalEdgeIndex    ( unsigned x, unsigned y ) const;

    GridCoord getCoord(VertexIndex v) const;

    VertexIndex getHorizontalVertexIndex ( unsigned x, unsigned y ) const;
    VertexIndex getVerticalVertexIndex   ( unsigned x, unsigned y ) const;

    public:
    BidimensionalGrid(unsigned x, unsigned y, std::vector<CNet> const &);

    // Access and modify the edges
    EdgeProperties & getTurnEdge        ( unsigned x, unsigned y );
    EdgeProperties & getHorizontalEdge  ( unsigned x, unsigned y );
    EdgeProperties & getVerticalEdge    ( unsigned x, unsigned y );

    // Get the results
    std::vector<std::vector<std::pair<GridCoord, GridCoord> > > getRouting() const;

    unsigned getXDim() const{ return xdim; }
    unsigned getYDim() const{ return ydim; }
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

