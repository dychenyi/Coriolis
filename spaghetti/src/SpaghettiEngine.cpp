
#include "crlcore/RoutingGauge.h"
#include "crlcore/Utilities.h"
#include "crlcore/ToolBox.h"
#include "hurricane/Error.h"

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

SpaghettiEngine* SpaghettiEngine::get (const Cell* cell )
{
  return ( dynamic_cast<SpaghettiEngine*> ( ToolEngine::get ( cell, SpaghettiEngine::staticGetName() ) ) );
}


void SpaghettiEngine::initGlobalRouting ( const std::map<Hurricane::Name,Hurricane::Net*>& excludedNets )
{

}

void SpaghettiEngine::run ( const std::map<Hurricane::Name,Hurricane::Net*>& excludedNets )
{
}


} // End namespace spaghetti


