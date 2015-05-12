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

#include "spaghetti/Common.h"

namespace CRL {
  class RoutingGauge;
}

namespace spaghetti{

class BidimensionalGrid;

using Hurricane::Name;
using Hurricane::Cell;
using Hurricane::DbU;
using CRL::RoutingGauge;

class SpaghettiEngine : public CRL::ToolEngine {

// Attributes
// **********

    private:
        static const Name    _toolName;
        RoutingGauge*        _routingGauge;
        unsigned int         _allowedDepth;

        BidimensionalGrid*   _routingGrid;

// Constructors & Destructors
// **************************
    protected:
        SpaghettiEngine ( Cell* cell );
        ~SpaghettiEngine();

    public:
        static SpaghettiEngine* create ( Cell* cell );
        void  _postCreate();
        void   destroy();
        void  _preDestroy();

// Modifiers
// *********

  public:
           void          setRoutingGauge         ( RoutingGauge* );
           RoutingGauge* getRoutingGauge         () const { return _routingGauge; }
           void          setAllowedDepth         ( unsigned int );
           unsigned int  getAllowedDepth         () const { return _allowedDepth; }

           void          createRoutingGraph      ( Capacity hReserved, Capacity vreserved, float edgeCost = 3.0, float turnCost = 0.5 );
           void          initGlobalRouting       ( const std::map<Hurricane::Name,Hurricane::Net*>& excludedNets );
           void          run                     ( const std::map<Hurricane::Name,Hurricane::Net*>& excludedNets );
           void          saveRoutingSolution     () const;

  std::vector<DbU::Unit> getHorizontalCutLines   () const;
  std::vector<DbU::Unit> getVerticalCutLines     () const;

           DbU::Unit     getVerticalCut          ( unsigned x ) const;
           DbU::Unit     getHorizontalCut        ( unsigned y ) const;
// Others
// ******
    public:
        static  SpaghettiEngine* get                  ( const Cell* );
        static  const Name& staticGetName             () { return _toolName; };
                const Name& getName                   () const { return _toolName; };


};

} // End namespace spaghetti

