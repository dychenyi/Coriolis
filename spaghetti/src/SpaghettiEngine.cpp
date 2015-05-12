
#include "hurricane/Error.h"
#include "crlcore/Utilities.h"
#include "crlcore/AllianceFramework.h"
#include "crlcore/RoutingGauge.h"
#include "crlcore/RoutingLayerGauge.h"
#include "crlcore/ToolBox.h"
#include "crlcore/AllianceFramework.h"
#include "crlcore/CellGauge.h"

#include "spaghetti/SpaghettiEngine.h"

#include "spaghetti/Grid.h"

namespace spaghetti{

using Hurricane::Error;

const Hurricane::Name SpaghettiEngine::_toolName           = "spaghetti::SpaghettiEngine";

SpaghettiEngine::SpaghettiEngine ( Cell* cell )
    : CRL::ToolEngine  ( cell )
    , _routingGauge    ( NULL )
    , _allowedDepth    ( 0)
{
    if(!cell)
        throw Error("Trying to init the SpaghettiEngine with a NULL cell\n");
}

SpaghettiEngine* SpaghettiEngine::create ( Cell* cell )
{
    CRL::deleteEmptyNets ( cell );
    SpaghettiEngine *ret  = new SpaghettiEngine(cell);
    ret->_postCreate();
    return ret;
}

void SpaghettiEngine::_postCreate()
{
    CRL::ToolEngine::_postCreate();
}

void SpaghettiEngine::_preDestroy()
{
    if(_routingGrid != NULL)
        delete(_routingGrid);
    
    CRL::ToolEngine::_preDestroy();
}

void SpaghettiEngine::destroy()
{
    _preDestroy();
    delete this;
}

SpaghettiEngine::~SpaghettiEngine()
{
}

void  SpaghettiEngine::setRoutingGauge ( RoutingGauge* rg )
{
  _routingGauge = rg;
  _allowedDepth = rg->getDepth();
}

void  SpaghettiEngine::setAllowedDepth ( unsigned int allowedDepth )
{
  if (not _routingGauge)
    throw Error( "SpaghettiEngine::setAllowedDepth(): Must set the RoutingGauge before the allowed depth.\n" );

  _allowedDepth = (allowedDepth < _routingGauge->getDepth()) ? allowedDepth : _routingGauge->getDepth();
}

SpaghettiEngine* SpaghettiEngine::get (const Cell* cell )
{
  return ( dynamic_cast<SpaghettiEngine*> ( ToolEngine::get ( cell, SpaghettiEngine::staticGetName() ) ) );
}

void SpaghettiEngine::createRoutingGraph ( Capacity hreserved, Capacity vreserved, float edgeCost, float turnCost )
{
    using namespace std;
    using namespace CRL;

    // Get the cell's dimensions and chose an appropriate routing grid (one standard cell height pitch)
    Cell* cell = getCell();
    DbU::Unit sliceHeight = AllianceFramework::get()->getCellGauge()->getSliceHeight();
    DbU::Unit cellWidth  = cell->getAbutmentBox().getWidth();
    DbU::Unit cellHeight = cell->getAbutmentBox().getHeight();
    unsigned xdim = ceil(float(cellWidth)  / float(sliceHeight));
    unsigned ydim = ceil(float(cellHeight) / float(sliceHeight));

    _routingGrid = new BidimensionalGrid(xdim, ydim);

    // Now initialize the capacities correctly (they should initially be 0)
    vector<RoutingLayerGauge*> rtLGauges = getRoutingGauge()->getLayerGauges();
    for ( vector<RoutingLayerGauge*>::iterator it = rtLGauges.begin() ; it != rtLGauges.end() ; it++ ) {
        RoutingLayerGauge* routingLayerGauge = (*it);
        if (routingLayerGauge->getType() != Constant::Default) continue;
        if (routingLayerGauge->getDepth() > getAllowedDepth()) continue;

        // Horizontal capacity
        if (routingLayerGauge->getDirection() == Constant::Horizontal){
            for(unsigned y=0; y<ydim; ++y){
                Capacity tracks = routingLayerGauge->getTrackNumber ( getHorizontalCut(y), getHorizontalCut(y+1) ) - 1;
                for(unsigned x=0; x+1<xdim; ++x){
                    _routingGrid->getHorizontalEdge(x, y).capacity += tracks;
                } 
                for(unsigned x=0; x<xdim; ++x){ // And via capacity
                    _routingGrid->getTurnEdge(x, y).capacity += tracks;
                }
            } 
        }
        // Vertical capacity
        if (routingLayerGauge->getDirection() == Constant::Vertical){
            for(unsigned x=0; x<xdim; ++x){
                Capacity tracks = routingLayerGauge->getTrackNumber ( getVerticalCut(x), getVerticalCut(x+1) ) - 1;
                for(unsigned y=0; y+1<ydim; ++y){
                    _routingGrid->getVerticalEdge(x, y).capacity += tracks;
                }
                for(unsigned y=0; y<ydim; ++y){ // And via capacity
                    _routingGrid->getTurnEdge(x, y).capacity += tracks;
                }
            }
        }
    }
    // Take reserved tracks into account and initialize costs
    for(unsigned y=0; y<ydim; ++y){
        for(unsigned x=0; x+1<xdim; ++x){
            Capacity cap = _routingGrid->getHorizontalEdge(x, y).capacity;
            cap = std::max(0, cap-hreserved);
            _routingGrid->getHorizontalEdge(x, y).capacity  = cap;
            _routingGrid->getHorizontalEdge(x, y).basicCost = edgeCost;
        }
    }
    for(unsigned x=0; x<xdim; ++x){
        for(unsigned y=0; y+1<ydim; ++y){
            Capacity cap = _routingGrid->getVerticalEdge(x, y).capacity;
            cap = std::max(0, cap-vreserved);
            _routingGrid->getVerticalEdge(x, y).capacity    = cap;
            _routingGrid->getVerticalEdge(x, y).basicCost   = edgeCost;
        }
    }
    for(unsigned x=0; x<xdim; ++x){
        for(unsigned y=0; y<ydim; ++y){
            Capacity cap = _routingGrid->getVerticalEdge(x, y).capacity;
            cap = std::max(0, cap-hreserved-vreserved);
            _routingGrid->getTurnEdge(x, y).capacity    = cap;
            _routingGrid->getTurnEdge(x, y).basicCost   = turnCost;
        }
    }
       
}

void SpaghettiEngine::initGlobalRouting ( const std::map<Hurricane::Name,Hurricane::Net*>& excludedNets )
{

}

void SpaghettiEngine::run ( const std::map<Hurricane::Name,Hurricane::Net*>& excludedNets )
{
}

std::vector<DbU::Unit> SpaghettiEngine::getHorizontalCutLines   () const
{
    if(!_routingGrid)
        throw Error("The global routing graph hasn't been initialized yet\n");
    std::vector<DbU::Unit> ret;
    unsigned ydim = _routingGrid->getYDim();
    for(unsigned y=0; y<=ydim; ++y)
        ret.push_back( getHorizontalCut(y) );
    return ret;
}

std::vector<DbU::Unit> SpaghettiEngine::getVerticalCutLines     () const
{
    if(!_routingGrid)
        throw Error("The global routing graph hasn't been initialized yet\n");
    std::vector<DbU::Unit> ret;
    unsigned xdim = _routingGrid->getXDim();
    for(unsigned x=0; x<=xdim; ++x)
        ret.push_back( getVerticalCut(x) );
    return ret;
}

DbU::Unit     SpaghettiEngine::getHorizontalCut        ( unsigned y ) const
{
    if(!_routingGrid)
        throw Error("The global routing graph hasn't been initialized yet\n");
    unsigned ydim = _routingGrid->getYDim();
    auto box = getCell()->getBoundingBox();
    return
        ( box.getYMin() * static_cast<std::int64_t>(ydim - y)
        + box.getYMax() * static_cast<std::int64_t>(y) )
        / ydim;
}

DbU::Unit     SpaghettiEngine::getVerticalCut          ( unsigned x ) const
{
    if(!_routingGrid)
        throw Error("The global routing graph hasn't been initialized yet\n");
    unsigned xdim = _routingGrid->getXDim();
    auto box = getCell()->getBoundingBox();
    return
        ( box.getXMin() * static_cast<std::int64_t>(xdim - x)
        + box.getXMax() * static_cast<std::int64_t>(x) )
        / xdim;
}

} // End namespace spaghetti


