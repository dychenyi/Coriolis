
#include "hurricane/Error.h"
#include "hurricane/Component.h"
#include "hurricane/Net.h"
#include "hurricane/DeepNet.h"
#include "hurricane/Cell.h"
#include "hurricane/Technology.h"
#include "hurricane/DataBase.h"
#include "hurricane/UpdateSession.h"
#include "hurricane/NetRoutingProperty.h"

#include "hurricane/Segment.h"
#include "hurricane/Horizontal.h"
#include "hurricane/Vertical.h"

#include "crlcore/Utilities.h"
#include "crlcore/AllianceFramework.h"
#include "crlcore/RoutingGauge.h"
#include "crlcore/RoutingLayerGauge.h"
#include "crlcore/ToolBox.h"
#include "crlcore/AllianceFramework.h"
#include "crlcore/CellGauge.h"

#include "spaghetti/SpaghettiEngine.h"
#include "spaghetti/Grid.h"
#include "spaghetti/CostFunctions.h"

#include <sstream>

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
    RoutingGauge* gauge = getRoutingGauge();
    if(!gauge)
        throw Error("SpaghettiEngine::createRoutingGraph(): No routing gauge specified\n");
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
    using namespace std;
    using namespace Hurricane;

    if(!_routingGrid)
        throw Error("SpaghettiEngine::initGlobalRouting(): the routing grid hasn't been initialized\n");

    Name obstacleNetName ("obstaclenet");
    // Careful with name clash between Hurricane::Net and Spaghetti::Net
    forEach ( Hurricane::Net*, inet, getCell()->getNets() ) {
      if (excludedNets.find(inet->getName()) != excludedNets.end()) {
        if (NetRoutingExtension::isUnconnected(*inet))
          cparanoid << "     - <" << inet->getName() << "> not routed (unconnected)." << endl;
        else
          cparanoid << "     - <" << inet->getName() << "> not routed (pre-routing found)." << endl;
        continue;
      }
  
      if (   inet->isGlobal()
         or  inet->isSupply()
         or (inet->getName() == obstacleNetName) ) {
        cmess2 << "     - <" << inet->getName() << "> not routed (global, supply, clock or obstacle)." << endl;
        continue;
      }

      CNet newNet; newNet.demand = 1; newNet.cost = 1.0;
      // TODO: Now add all existing segments to the initial components
      for_each_component ( component, inet->getComponents() ) {
        if ( dynamic_cast<RoutingPad*>(component) or dynamic_cast<Segment*>(component) ){
            newNet.components.emplace_back();
            Box cur;
            if( dynamic_cast<RoutingPad*>(component) ) cur = dynamic_cast<RoutingPad*>(component)->getBoundingBox();
            else if( dynamic_cast<Horizontal*>(component) ) cur = dynamic_cast<Horizontal*>(component)->getBoundingBox();
            else if( dynamic_cast<Vertical*>(component) ) cur = dynamic_cast<Vertical*>(component)->getBoundingBox();
            else throw Error("The Segment is neither Horizontal nor Vertical\n");

            for(unsigned x=getGridX(cur.getXMin()); x<=getGridX(cur.getXMax()); ++x){
                for(unsigned y=getGridY(cur.getYMin()); y<=getGridY(cur.getYMax()); ++y){
                    newNet.components.back().push_back(PlanarCoord(x, y));
                }
            }
        }
        else if ( dynamic_cast<Contact*>(component) ) {
            //cerr << Error("In net <%s>: found <%s> before routing\n", inet->_getString().c_str(), component->_getString().c_str()) << endl;
            /*if ( isAGlobalRoutingContact ( contact ) )
                vContacts.push_back ( contact ); */
        }
        end_for;
      }
      _routingGrid->pushNet(newNet);

    }
}

void SpaghettiEngine::run ( const std::map<Hurricane::Name,Hurricane::Net*>& excludedNets )
{
    initGlobalRouting(excludedNets);
    _routingGrid->steinerRoute(
        basicEdgeCostFunction()
    );
    cmess2 << "Steiner-based routing finished" << std::endl;
    outputStats();
    float mul = 0.1;
    int i=1;
    while(not _routingGrid->isCorrectlyRouted(overflowPredicate())){
        _routingGrid->updateHistoryCosts(overflowPredicate(), 1.0, mul * 0.25);
        _routingGrid->rebiroute(
            overflowPredicate(1.0),
            //dualthresholdEdgeCostFunction(mul)
            thresholdEdgeCostFunction(mul)
        );
        cmess2 << "Global routing iteration " << i << std::endl;
        outputStats();
        if(not _routingGrid->isRouted())
            throw Error("Ahem. Somehow the global routing algorithm didn't manage to connect some nets even when allowing overflows\n");
        mul *= 1.5f;
        ++i;
    }
    cmess2 << "Global routing completed" << std::endl;
}

void SpaghettiEngine::saveRoutingSolution () const
{
    // TODO: for each net, add all edges in the routing grid to the corresponding nets, create contacts    
}

void SpaghettiEngine::outputStats () const
{
    using namespace std;
    using namespace Hurricane;
    if(!_routingGrid) return;

    if(not _routingGrid->isRouted())
        cmess2 << "Some nets are left unrouted" << endl;
    auto costEval = edgeDemandFunction();
    Cost hcost = _routingGrid->avgHCost(costEval);
    Cost vcost = _routingGrid->avgVCost(costEval);
    size_t xdim = _routingGrid->getXDim();
    size_t ydim = _routingGrid->getYDim();

    cmess2 << "\t" << xdim << " * " << ydim << " grid" << endl;
    cmess2 << "\tAvg H demand:  \t" << hcost  << endl;
    cmess2 << "\tAvg V demand:  \t" << vcost  << endl;
    cmess2 << "\tGR wirelength: \t" << hcost * ydim * (xdim-1) + vcost * xdim * (ydim-1) << endl;
    auto ovEval = overflowPredicate();
    cout << "Overflows: " << _routingGrid->overflowCount(ovEval) << endl;

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

DbU::Unit SpaghettiEngine::getHorizontalCut ( unsigned y ) const
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

DbU::Unit SpaghettiEngine::getVerticalCut   ( unsigned x ) const
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

unsigned SpaghettiEngine::getGridX ( DbU::Unit x ) const
{
    if(!_routingGrid)
        throw Error("The global routing graph hasn't been initialized yet\n");
    unsigned xdim = _routingGrid->getXDim();
    auto box = getCell()->getBoundingBox();
    unsigned ret = (( x - box.getXMin() ) * xdim ) / box.getWidth();
    return ret;
}

unsigned SpaghettiEngine::getGridY ( DbU::Unit y ) const
{
    if(!_routingGrid)
        throw Error("The global routing graph hasn't been initialized yet\n");
    unsigned ydim = _routingGrid->getYDim();
    auto box = getCell()->getBoundingBox();
    unsigned ret = (( y - box.getYMin() ) * ydim ) / box.getHeight();
    return ret;
}

} // End namespace spaghetti


