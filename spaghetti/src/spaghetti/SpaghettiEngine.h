// -*- C++ -*-
//
// This file is part of the Coriolis Software.
// Copyright (c) UPMC 2006-2015, All Rights Reserved
//
// +-----------------------------------------------------------------+
// |                   C O R I O L I S                               |
// |        K n i k  -  G l o b a l   R o u t e r                    |
// |                                                                 |
// |  Author      :                     Gabriel Gouvine              |
// |  E-mail      :             Gabriel.Gouvine@lip6.fr              |
// | =============================================================== |
// |  C++ Header  :  "./spaghetti/SpaghettiEngine.h"                 |
// +-----------------------------------------------------------------+


#include "hurricane/Timer.h"
#include "hurricane/Property.h"
#include "hurricane/Net.h"
#include "hurricane/RoutingPad.h"
#include "crlcore/ToolEngine.h"

namespace CRL {
  class RoutingGauge;
}

namespace spaghetti{

class BidimensionalGrid;

using Hurricane::Name;
using Hurricane::Cell;
using CRL::RoutingGauge;


class SpaghettiEngine : public CRL::ToolEngine {

// Attributes
// **********

    private:
        static const Name    _toolName;
        size_t               _hEdgeReservedLocal;
        size_t               _vEdgeReservedLocal;
        RoutingGauge*        _routingGauge;
        unsigned int         _allowedDepth;

        BidimensionalGrid*   _routingGrid;

// Constructors & Destructors
// **************************
    protected:
        SpaghettiEngine ( Cell* cell, float edgeCost, float turnCost );
        ~SpaghettiEngine();

    public:
        static SpaghettiEngine* create ( Cell* cell, float edgeCost = 3.0, float turnCost = 0.5 );
        void  _postCreate();
        void   destroy();
        void  _preDestroy();

// Modifiers
// *********

  public:
           void          setHEdgeReservedLocal   ( size_t reserved ) { _hEdgeReservedLocal = reserved; };
           void          setVEdgeReservedLocal   ( size_t reserved ) { _vEdgeReservedLocal = reserved; };
           void          setRoutingGauge         ( RoutingGauge* );
           RoutingGauge* getRoutingGauge         () const { return _routingGauge; }
           void          initGlobalRouting       ( const std::map<Hurricane::Name,Hurricane::Net*>& excludedNets ); // Making it public, so it can be called earlier and then capacities on edges can be ajusted
           void          run                     ( const std::map<Hurricane::Name,Hurricane::Net*>& excludedNets );


// Others
// ******
    public:
        static  SpaghettiEngine* get                  ( const Cell* );
        static  const Name& staticGetName             () { return _toolName; };
                float       getHEdgeReservedLocal     () { return _hEdgeReservedLocal; };
                float       getVEdgeReservedLocal     () { return _vEdgeReservedLocal; };
                const Name& getName                   () const { return _toolName; };


};

} // End namespace spaghetti

