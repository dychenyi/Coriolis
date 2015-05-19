
#include "hurricane/Error.h"
#include "hurricane/Warning.h"
#include "hurricane/Component.h"
#include "hurricane/RoutingPad.h"
#include "hurricane/Net.h"
#include "hurricane/DeepNet.h"
#include "hurricane/Cell.h"
#include "hurricane/Technology.h"
#include "hurricane/DataBase.h"
#include "hurricane/UpdateSession.h"
#include "hurricane/Property.h"
#include "hurricane/Contact.h"
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
#include <unordered_set>
#include <unordered_map>

namespace spaghetti{

using Hurricane::Error;

const Hurricane::Name SpaghettiEngine::_toolName           = "spaghetti::SpaghettiEngine";

namespace{
    using namespace Hurricane;
    using namespace std;

    Hurricane::Layer const * getGMetalH(){
      DataBase*   db         = DataBase::getDB ();
      Technology* technology = db->getTechnology ();

      const Layer* gMetalH = technology->getLayer("gmetalh");
      if ( !gMetalH ) {
        cerr << Warning("Knik::Configuration() - No \"gmetalh\" in technology, falling back to \"metal2\".") << endl;
        gMetalH = technology->getLayer("metal2");
        if ( !gMetalH )
          throw Error("Knik::Configuration() - No \"gmetalh\" nor \"metal2\" in technology.");
      }
      return gMetalH;
    }
    Hurricane::Layer const * getGMetalV(){
      DataBase*   db         = DataBase::getDB ();
      Technology* technology = db->getTechnology ();

      const Layer* gMetalV = technology->getLayer("gmetalv");
      if ( !gMetalV ) {
        cerr << Warning("Knik::Configuration() - No \"gmetalv\" in technology, falling back to \"metal3\".") << endl;
        gMetalV = technology->getLayer("metal3");
        if ( !gMetalV )
          throw Error("Knik::Configuration() - No \"gmetalv\" nor \"metal3\" in technology.");
      }
      return gMetalV;
    }
    Hurricane::Layer const * getGContact(){
      DataBase*   db         = DataBase::getDB ();
      Technology* technology = db->getTechnology ();

      const Layer* gContact = technology->getLayer("gcontact");
      if ( !gContact ) {
        cerr << Warning("Knik::Configuration() - No \"gcontact\" in technology, falling back to \"VIA23\".") << endl;
        gContact = technology->getLayer("VIA23");
        if ( !gContact )
          throw Error("Knik::Configuration() - No \"gmetalh\" nor \"VIA23\" in technology.");
      }
      return gContact;
    }
}

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
            Capacity cap = _routingGrid->getTurnEdge(x, y).capacity;
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
      forEach ( RoutingPad*, ipad, inet->getRoutingPads() ) {
        newNet.components.emplace_back();
        // Bounding box or center?
        Point cur = ipad->getCenter();
        newNet.components.back().emplace_back(getGridX(cur.getX()), getGridY(cur.getY()));
        /*
        Box cur = ipad->getBoundingBox();
        for(unsigned x=getGridX(cur.getXMin()); x<=getGridX(cur.getXMax()); ++x){
            for(unsigned y=getGridY(cur.getYMin()); y<=getGridY(cur.getYMax()); ++y){
                newNet.components.back().push_back(PlanarCoord(x, y));
            }
        }
        */
      }
      _grNets.push_back(*inet);
      _routingGrid->pushNet(newNet);
    }
}

void SpaghettiEngine::run ( const std::map<Hurricane::Name,Hurricane::Net*>& excludedNets )
{
    initGlobalRouting(excludedNets);
    _routingGrid->selfcheck();
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
        _routingGrid->cleanupRouting();
        _routingGrid->selfcheck();
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
    using namespace Hurricane;

    std::vector<RoutedCNet> prunedRoutes = _routingGrid->getPrunedRouting();
    assert(prunedRoutes.size() == _grNets.size());

    for(size_t i=0; i<_grNets.size(); ++i){
      //cmess2 << "Handling net: " << _grNets[i] << endl;
      std::unordered_set<VertexIndex> contactPos, pinPos;
      for(auto const & comp : prunedRoutes[i].components)
        for(PlanarCoord c : comp){
          contactPos.emplace(getRepr(c.x, c.y));
          pinPos.emplace(getRepr(c.x, c.y));
        }
      for(auto e : prunedRoutes[i].routing){
        contactPos.emplace(getRepr( e.first.x, e.first.y ));
        contactPos.emplace(getRepr( e.second.x, e.second.y ));
        //cmess2 << "\tEdge from (" << e.first.x << "," << e.first.y << ") to (" << e.second.x << "," << e.second.y << ")" << endl;
      }

      DbU::Unit epsilon = DbU::lambda(2);

      // Create the contacts where we need it
      std::unordered_map<VertexIndex, Contact*> contacts;
      for(VertexIndex v : contactPos){
        PlanarCoord c = _routingGrid->getCoord(v);
        DbU::Unit contactX = (getVerticalCut(c.x)   + getVerticalCut(c.x+1))/2,
                  contactY = (getHorizontalCut(c.y) + getHorizontalCut(c.y+1))/2;
        DbU::Unit wX = epsilon, //getVerticalCut(c.x+1)   - getVerticalCut(c.x),
                  wY = epsilon; //getHorizontalCut(c.y+1) - getHorizontalCut(c.y);
        //cmess2 << "\tContact at (" << c.x << "," << c.y << ") translated to (" << DbU::getValueString(contactX) << "," << DbU::getValueString(contactY) << ")" << endl;
        contacts[v] = Contact::create(
              _grNets[i]
            , getGContact() //DataBase::getDB()->getTechnology()->getLayer("metal2")
            , contactX
            , contactY
            , wX 
            , wY
        );
      }
      // Create the segments associated with the contacts
      for(auto e : prunedRoutes[i].routing){
        PlanarCoord e1 = e.first, e2 = e.second;
        VertexIndex v1 = getRepr(e1.x, e1.y),
                    v2 = getRepr(e2.x, e2.y);
        Contact* c1 = contacts.at(v1);
        Contact* c2 = contacts.at(v2);
        if(e1.x == e2.x)
          if ( e1.y <= e2.y )
            Vertical::create ( c1, c2, getGMetalV(), c1->getX(), epsilon );
          else
            Vertical::create ( c2, c1, getGMetalV(), c1->getX(), epsilon );
        else if(e1.y == e2.y)
          if ( e1.x <= e2.x )
            Horizontal::create ( c1, c2, getGMetalH(), c1->getY(), epsilon );
          else
            Horizontal::create ( c2, c1, getGMetalH(), c1->getY(), epsilon );
        else throw Error("The segment is neither horizontal nor vertical\n");
      }

      // Get the contact associated with each pad
      std::unordered_set<VertexIndex> pads;
      forEach ( RoutingPad*, ipad, _grNets[i]->getRoutingPads() ) {
        Point cur = ipad->getCenter();
        //cmess2 << "Routing pad at (" << getGridX(cur.getX()) << ", " << getGridY(cur.getY()) << ") translated from (" << DbU::getValueString(cur.getX()) << "," << DbU::getValueString(cur.getY()) << ")" << endl;
        VertexIndex repr = getRepr(getGridX(cur.getX()), getGridY(cur.getY()));
        Contact* c = contacts.at(repr);
        pads.emplace(repr);
        ipad->getBodyHook()->detach();
        ipad->getBodyHook()->attach(c->getBodyHook());
      }
      for(VertexIndex v : pinPos){
        if(pads.count(v)==0){
            PlanarCoord c = _routingGrid->getCoord(v);
            throw Error("Didn't find any pad associated with the component (%d, %d)\n", c.x, c.y);
        }
      }
    }
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
    if( y > ydim )
        throw Error("Bigger than the grid's Y dimension\n");
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
    if( x > xdim )
        throw Error("Bigger than the grid's X dimension\n");
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
    if( x < box.getXMin() or x >= box.getXMax())
        throw Error("Not enclosed in the bounding box\n");
    unsigned ret = (( x - box.getXMin() ) * xdim ) / box.getWidth();
    return ret;
}

unsigned SpaghettiEngine::getGridY ( DbU::Unit y ) const
{
    if(!_routingGrid)
        throw Error("The global routing graph hasn't been initialized yet\n");
    unsigned ydim = _routingGrid->getYDim();
    auto box = getCell()->getBoundingBox();
    if( y < box.getYMin() or y >= box.getYMax())
        throw Error("Not enclosed in the bounding box\n");
    unsigned ret = (( y - box.getYMin() ) * ydim ) / box.getHeight();
    return ret;
}

VertexIndex SpaghettiEngine::getRepr ( unsigned x, unsigned y ) const{
    return _routingGrid->getVertexRepr(x, y);
}


} // End namespace spaghetti


