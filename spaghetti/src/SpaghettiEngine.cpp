
#include "crlcore/RoutingGauge.h"
#include "crlcore/Utilities.h"
#include "crlcore/ToolBox.h"

#include "spaghetti/SpaghettiEngine.h"

#include "spaghetti/Grid.h"

namespace spaghetti{

const Hurricane::Name SpaghettiEngine::_toolName           = "spaghetti::SpaghettiEngine";

SpaghettiEngine::SpaghettiEngine ( Cell* cell, float edgeCost, float turnCost )
    : CRL::ToolEngine  ( cell )
    , _routingGauge    ( NULL )
    , _allowedDepth    ( 0)
{
}

SpaghettiEngine* SpaghettiEngine::create ( Cell* cell, float edgeCost, float turnCost )
{
    CRL::deleteEmptyNets ( cell );
    SpaghettiEngine *ret  = new SpaghettiEngine(cell, edgeCost, turnCost);
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

} // End namespace spaghetti


