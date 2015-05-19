// -*- C++ -*-
//
// This file is part of the Coriolis Software.
// Copyright (c) UPMC 2008-2015, All Rights Reserved
//
// +-----------------------------------------------------------------+
// |                   C O R I O L I S                               |
// |      K i t e  -  D e t a i l e d   R o u t e r                  |
// |                                                                 |
// |  Author      :                    Jean-Paul CHAPUT              |
// |  E-mail      :       Jean-Paul.Chaput@asim.lip6.fr              |
// | =============================================================== |
// |  C++ Module  :       "./KiteEngine.cpp"                         |
// +-----------------------------------------------------------------+


#include <Python.h>
#include <sstream>
#include <fstream>
#include <iomanip>
#include "vlsisapd/utilities/Path.h"
#include "hurricane/DebugSession.h"
#include "hurricane/UpdateSession.h"
#include "hurricane/Bug.h"
#include "hurricane/Error.h"
#include "hurricane/Warning.h"
#include "hurricane/Breakpoint.h"
#include "hurricane/Layer.h"
#include "hurricane/Net.h"
#include "hurricane/Pad.h"
#include "hurricane/Plug.h"
#include "hurricane/Cell.h"
#include "hurricane/Instance.h"
#include "hurricane/Vertical.h"
#include "hurricane/Horizontal.h"
#include "hurricane/viewer/Script.h"
#include "crlcore/Measures.h"
#include "katabatic/AutoContact.h"
#include "katabatic/GCellGrid.h"
#include "kite/DataNegociate.h"
#include "kite/RoutingPlane.h"
#include "kite/Session.h"
#include "kite/TrackSegment.h"
#include "kite/NegociateWindow.h"
#include "kite/KiteEngine.h"
#include "kite/PyKiteEngine.h"
#include "spaghetti/SpaghettiEngine.h"
#include "spaghetti/Grid.h"

namespace Kite {

  using std::cout;
  using std::cerr;
  using std::endl;
  using std::dec;
  using std::setw;
  using std::left;
  using std::ostream;
  using std::ofstream;
  using std::ostringstream;
  using std::setprecision;
  using std::vector;
  using std::make_pair;
  using Hurricane::dbo_ptr;
  using Hurricane::DebugSession;
  using Hurricane::tab;
  using Hurricane::inltrace;
  using Hurricane::ltracein;
  using Hurricane::ltraceout;
  using Hurricane::ForEachIterator;
  using Hurricane::Bug;
  using Hurricane::Error;
  using Hurricane::Warning;
  using Hurricane::Breakpoint;
  using Hurricane::Box;
  using Hurricane::Torus;
  using Hurricane::Layer;
  using Hurricane::Cell;
  using CRL::System;
  using CRL::addMeasure;
  using CRL::Measures;
  using CRL::MeasuresSet;
  using Katabatic::AutoContact;
  using Katabatic::AutoSegmentLut;
  using Katabatic::ChipTools;


  const char* missingRW =
    "%s :\n\n"
    "    Cell %s do not have any KiteEngine (or not yet created).\n";


// -------------------------------------------------------------------
// Class  :  "Kite::KiteEngine".

  Name  KiteEngine::_toolName = "Kite";


  const Name& KiteEngine::staticGetName ()
  { return _toolName; }


  KiteEngine* KiteEngine::get ( const Cell* cell )
  { return static_cast<KiteEngine*>(ToolEngine::get(cell,staticGetName())); }


  KiteEngine::KiteEngine ( Cell* cell )
    : KatabaticEngine  (cell)
    , _viewer          (NULL)
    , _globalRouter    (NULL)
    , _blockageNet     (cell->getNet("blockagenet"))
    , _configuration   (new Configuration(getKatabaticConfiguration()))
    , _routingPlanes   ()
    , _negociateWindow (NULL)
    , _minimumWL       (0.0)
    , _toolSuccess     (false)
  { }


  void  KiteEngine::_postCreate ()
  {
    KatabaticEngine::_postCreate ();
  }


  void  KiteEngine::_runKiteInit ()
  {
    Utilities::Path pythonSitePackages = System::getPath("pythonSitePackages");
    Utilities::Path systemConfDir      = pythonSitePackages / "kite";
    Utilities::Path systemConfFile     = systemConfDir      / "kiteInit.py";

    if (systemConfFile.exists()) {
      Isobar::Script::addPath( systemConfDir.string() );

      dbo_ptr<Isobar::Script> script = Isobar::Script::create( systemConfFile.stem().string() );
      script->addKwArgument( "kite"    , (PyObject*)PyKiteEngine_Link(this) );
      script->runFunction  ( "kiteHook", getCell() );

      Isobar::Script::removePath( systemConfDir.string() );
    } else {
      cerr << Warning("Kite system configuration file:\n  <%s> not found."
                     ,systemConfFile.string().c_str()) << endl;
    }
  }


  void  KiteEngine::_initDataBase ()
  {
    ltrace(90) << "KiteEngine::_initDataBase()" << endl;
    ltracein(90);

    Session::open( this );
    createGlobalGraph( KtNoFlags );
    createDetailedGrid();
    findSpecialNets();
    buildPreRouteds();
    buildPowerRails();
    protectRoutingPads();
    Session::close();
    _runKiteInit();

    ltraceout(90);
  }


  KiteEngine* KiteEngine::create ( Cell* cell )
  {
    KiteEngine* kite = new KiteEngine ( cell );

    kite->_postCreate();
    kite->_initDataBase();

    return kite;
  }

  void  KiteEngine::_preDestroy ()
  {
    ltrace(90) << "KiteEngine::_preDestroy()" << endl;
    ltracein(90);

    cmess1 << "  o  Deleting ToolEngine<" << getName() << "> from Cell <"
           << _cell->getName() << ">" << endl;

    if (getState() < Katabatic::EngineGutted)
      setState( Katabatic::EnginePreDestroying );

    _gutKite();
    KatabaticEngine::_preDestroy();

    cmess2 << "     - RoutingEvents := " << RoutingEvent::getAllocateds() << endl;

    if (not ToolEngine::inDestroyAll()) {
      if(_globalRouter) _globalRouter->destroy();
    }

    ltraceout(90);
  }

  void KiteEngine::wipeOutRouting( Cell * cell ){
    if(KiteEngine::get(cell) != NULL or KatabaticEngine::get(cell) != NULL)
        throw Error("Trying to wipe out a routing with a routing engine\n");
    using namespace Hurricane;
    UpdateSession::open();
    forEach(Net*, inet, cell->getNets()){
      if(NetRoutingExtension::isManualGlobalRoute(*inet))
        continue;
      // First pass: destroy the contacts
      std::vector<Contact*> contactPointers;
      forEach(Component*, icom, (*inet)->getComponents()){
        Contact * contact = dynamic_cast<Contact*>(*icom);
        if(contact){
          contactPointers.push_back(contact);
        }
      }
      for(Contact* contact : contactPointers)
        contact->destroy();
      // Second pass: destroy unconnected segments added by Knik as blockages
      std::vector<Component*> compPointers;
      forEach(Component*, icom, (*inet)->getComponents()){
        Horizontal * h = dynamic_cast<Horizontal*>(*icom);
        if(h){
          compPointers.push_back(h);
        }
        Vertical * v = dynamic_cast<Vertical*>(*icom);
        if(v){
          compPointers.push_back(v);
        }
      }
      for(Component* comp : compPointers)
        comp->destroy();
    }
    UpdateSession::close();
  }

  KiteEngine::~KiteEngine ()
  { delete _configuration; }


  const Name& KiteEngine::getName () const
  { return _toolName; }


  Configuration* KiteEngine::getConfiguration ()
  { return _configuration; }


  unsigned int  KiteEngine::getRipupLimit ( const TrackElement* segment ) const
  {
    if (segment->isBlockage()) return 0;

    if (segment->isStrap ()) return _configuration->getRipupLimit( Configuration::StrapRipupLimit );
    if (segment->isGlobal()) {
      Katabatic::GCellVector gcells;
      segment->getGCells( gcells );
      if (gcells.size() > 2)
        return _configuration->getRipupLimit( Configuration::LongGlobalRipupLimit );
      return _configuration->getRipupLimit( Configuration::GlobalRipupLimit );
    }
    return _configuration->getRipupLimit( Configuration::LocalRipupLimit );
  }


  RoutingPlane* KiteEngine::getRoutingPlaneByIndex ( size_t index ) const
  {
    if (index >= getRoutingPlanesSize() ) return NULL;
    return _routingPlanes[index];
  }


  RoutingPlane* KiteEngine::getRoutingPlaneByLayer ( const Layer* layer ) const
  {
    for ( size_t index=0 ; index < getRoutingPlanesSize() ; index++ ) {
      if (_routingPlanes[index]->getLayer() == layer)
        return _routingPlanes[index];
    }
    return NULL;
  }


  Track* KiteEngine::getTrackByPosition ( const Layer* layer, DbU::Unit axis, unsigned int mode ) const
  {
    RoutingPlane* plane = getRoutingPlaneByLayer( layer );
    if (not plane) return NULL;

    return plane->getTrackByPosition( axis, mode );
  }


  void  KiteEngine::setInterrupt ( bool state )
  {
    if (_negociateWindow) {
      _negociateWindow->setInterrupt( state ); 
      cerr << "Interrupt [CRTL+C] of " << this << endl;
    }
  }


  void  KiteEngine::createGlobalGraph ( unsigned int mode )
  {
    Cell* cell   = getCell();
  //Box   cellBb = cell->getBoundingBox();
    if (not _globalRouter) {
      unsigned int  flags = Cell::WarnOnUnplacedInstances;
      flags |= (mode & KtBuildGlobalRouting) ? Cell::BuildRings : 0;
    //if (not cell->isFlattenedNets()) cell->flattenNets( flags );
      cell->flattenNets( flags );

      KatabaticEngine::chipPrep();
  
      _globalRouter = SpaghettiEngine::create( cell );

      _globalRouter->setRoutingGauge( getConfiguration()->getRoutingGauge() );
      _globalRouter->setAllowedDepth( getConfiguration()->getAllowedDepth() );
      _globalRouter->createRoutingGraph(
          0, //getHTracksReservedLocal(),
          0, //getVTracksReservedLocal(),
          3.0,
          0.5
        );
  
    // Decrease the edge's capacity only under the core area.
      const ChipTools&    chipTools      = getChipTools();
      spaghetti::Capacity coronaReserved = 4;

      unsigned xdim = _globalRouter->getRoutingGrid()->getXDim();
      unsigned ydim = _globalRouter->getRoutingGrid()->getYDim();

      // VEdges
      for(unsigned x=0; x<xdim; ++x){
        for(unsigned y=0; y+1<ydim; ++y){
          DbU::Unit midX = (_globalRouter->getVerticalCut(x) + _globalRouter->getVerticalCut(x+1))/2;
          auto bb = Box(
            midX,
            _globalRouter->getHorizontalCut(y),
            midX,
            _globalRouter->getHorizontalCut(y+1)
          );
          spaghetti::Capacity & edgecap = _globalRouter->getRoutingGrid()->getVerticalEdge(x, y).capacity;
          if (chipTools.vPadsEnclosed(bb)) {
            edgecap = 0;
          }
          else{
            spaghetti::Capacity edgeReserved = 0;
            if (chipTools.getCorona().getInnerBox().contains(bb))
              edgeReserved = getVTracksReservedLocal();
            else if (chipTools.getCorona().getOuterBox().contains(bb))
              edgeReserved = coronaReserved;
            edgecap = (edgecap>edgeReserved) ? (edgecap-edgeReserved) : 0;
          }
        }
      }
      // HEdges
      for(unsigned y=0; y<ydim; ++y){
        for(unsigned x=0; x+1<xdim; ++x){
          DbU::Unit midY = (_globalRouter->getHorizontalCut(y) + _globalRouter->getHorizontalCut(y+1))/2;
          auto bb = Box(
            _globalRouter->getVerticalCut(x),
            midY,
            _globalRouter->getVerticalCut(x+1),
            midY
          );
          spaghetti::Capacity & edgecap = _globalRouter->getRoutingGrid()->getHorizontalEdge(x, y).capacity;
          if (chipTools.hPadsEnclosed(bb)) {
            edgecap = 0;
          }
          else{
            spaghetti::Capacity edgeReserved = 0;
            if (chipTools.getCorona().getInnerBox().contains(bb))
              edgeReserved = getHTracksReservedLocal();
            else if (chipTools.getCorona().getOuterBox().contains(bb))
              edgeReserved = coronaReserved;
            edgecap = (edgecap>edgeReserved) ? (edgecap-edgeReserved) : 0;
          }
        }
      }
      // TODO: create the edges
      /*  
      forEach ( Knik::Vertex*, ivertex, _knik->getRoutingGraph()->getVertexes() ) {
        for ( int i=0 ; i<2 ; ++i ) {
          Knik::Edge* edge    = NULL;
  
          if (i==0) {
            edge = ivertex->getHEdgeOut();
            if (not edge) continue;
  
            if (chipTools.hPadsEnclosed(edge->getBoundingBox())) {
              edge->setCapacity( 0 );
              continue;
            }
            coreReserved = getHTracksReservedLocal();
          } else {
            edge = ivertex->getVEdgeOut();
            if (not edge) continue;
  
            if (chipTools.vPadsEnclosed(edge->getBoundingBox())) {
              edge->setCapacity( 0 );
              continue;
            }
            coreReserved = getVTracksReservedLocal();
          }
  
          size_t edgeReserved = 0;
          if (chipTools.getCorona().getInnerBox().contains(edge->getBoundingBox())) {
            edgeReserved = coreReserved;
          } else if (chipTools.getCorona().getOuterBox().contains(edge->getBoundingBox())) {
            edgeReserved = coronaReserved;
          }
  
          size_t capacity = (edge->getCapacity()>edgeReserved)
                          ? (edge->getCapacity()-edgeReserved) : 0;
        //cerr << "Setting capacity " << capacity << " ("
        //     << capacity << ") on: " << edge << endl;
          edge->setCapacity( capacity );
        }
      }
    */
    }
  }


  void  KiteEngine::createDetailedGrid ()
  {
    KatabaticEngine::createDetailedGrid();

    size_t maxDepth = getRoutingGauge()->getDepth();

    _routingPlanes.reserve( maxDepth );
    for ( size_t depth=0 ; depth < maxDepth ; depth++ ) {
      _routingPlanes.push_back( RoutingPlane::create ( this, depth ) );
    }
  }


  void  KiteEngine::saveGlobalSolution ()
  {
    if (getState() < Katabatic::EngineGlobalLoaded)
      throw Error ("KiteEngine::saveGlobalSolution(): Global routing not present yet.");

    if (getState() > Katabatic::EngineGlobalLoaded)
      throw Error ("KiteEngine::saveGlobalSolution(): Cannot save after detailed routing.");

    // TODO: save global routing
    // See _knik->saveSolution();
  }


  void  KiteEngine::annotateGlobalGraph ()
  {
    cmess1 << "  o  Back annotate global routing graph." << endl;

    const Torus& chipCorona = getChipTools().getCorona();

    int hEdgeCapacity = 0;
    int vEdgeCapacity = 0;
    for ( size_t depth=0 ; depth<_routingPlanes.size() ; ++depth ) {
      RoutingPlane* rp = _routingPlanes[depth];
      if (rp->getLayerGauge()->getType()  == Constant::PinOnly ) continue;
      if (rp->getLayerGauge()->getDepth() > getConfiguration()->getAllowedDepth() ) continue;

      if (rp->getDirection() == KbHorizontal) ++hEdgeCapacity;
      else ++vEdgeCapacity;
    }

    for ( size_t depth=0 ; depth<_routingPlanes.size() ; ++depth ) {
      RoutingPlane* rp = _routingPlanes[depth];
      if (rp->getLayerGauge()->getType() == Constant::PinOnly ) continue;
      if (rp->getLayerGauge()->getDepth() > getConfiguration()->getAllowedDepth() ) continue;

      size_t tracksSize = rp->getTracksSize();
      for ( size_t itrack=0 ; itrack<tracksSize ; ++itrack ) {
        Track*        track   = rp->getTrackByIndex ( itrack );

        ltrace(300) << "Capacity from: " << track << endl;

        if (track->getDirection() == KbHorizontal) {
          for ( size_t ielement=0 ; ielement<track->getSize() ; ++ielement ) {
            TrackElement* element = track->getSegment( ielement );
         
            if (element->getNet() == NULL) {
              ltrace(300) << "Reject capacity from (not Net): " << element << endl;
              continue;
            }
            if (   (not element->isFixed())
               and (not element->isBlockage())
               and (not element->isUserDefined()) ) {
              cmess2 << "Reject capacity from (neither fixed, blockage nor user defined): " << element << endl;
            //ltrace(300) << "Reject capacity from (neither fixed nor blockage): " << element << endl;
              continue;
            }

            Box elementBb       = element->getBoundingBox();
            int elementCapacity = (chipCorona.contains(elementBb)) ? -hEdgeCapacity : -1;

            ltrace(300) << "Capacity from: " << element << ":" << elementCapacity << endl;

            Katabatic::GCell* gcell = getGCellGrid()->getGCell( Point(element->getSourceU(),track->getAxis()) );
            Katabatic::GCell* end   = getGCellGrid()->getGCell( Point(element->getTargetU(),track->getAxis()) );
            Katabatic::GCell* right = NULL;
            if (not gcell) {
              cerr << Warning("annotageGlobalGraph(): TrackElement outside GCell grid.") << endl;
              continue;
            }

            while ( gcell and (gcell != end) ) {
              right = gcell->getRight();
              if (right == NULL) break;
              _globalRouter->getRoutingGrid()->getHorizontalEdge(gcell->getRow(), gcell->getColumn()).capacity += elementCapacity;
              assert(right->getColumn() == gcell->getColumn()+1);
              assert(right->getRow() == gcell->getRow());
              gcell = right;
            }
          }
        } else {
          for ( size_t ielement=0 ; ielement<track->getSize() ; ++ielement ) {
            TrackElement* element = track->getSegment( ielement );

            if (element->getNet() == NULL) {
              ltrace(300) << "Reject capacity from (not Net): " << element << endl;
              continue;
            }
            if ( (not element->isFixed()) and not (element->isBlockage()) ) {
              ltrace(300) << "Reject capacity from (neither fixed nor blockage): " << element << endl;
              continue;
            }

            Box elementBb       = element->getBoundingBox();
            int elementCapacity = (chipCorona.contains(elementBb)) ? -vEdgeCapacity : -1;

            ltrace(300) << "Capacity from: " << element << ":" << elementCapacity << endl;

            Katabatic::GCell* gcell = getGCellGrid()->getGCell( Point(track->getAxis(),element->getSourceU()) );
            Katabatic::GCell* end   = getGCellGrid()->getGCell( Point(track->getAxis(),element->getTargetU()) );
            Katabatic::GCell* up    = NULL;
            if (not gcell) {
              cerr << Warning("annotageGlobalGraph(): TrackElement outside GCell grid.") << endl;
              continue;
            }
            while ( gcell and (gcell != end) ) {
              up = gcell->getUp();
              if (up == NULL) break;
              _globalRouter->getRoutingGrid()->getVerticalEdge(gcell->getRow(), gcell->getColumn()).capacity += elementCapacity;
              assert(up->getColumn() == gcell->getColumn());
              assert(up->getRow() == gcell->getRow()+1);
              gcell = up;
            }
          }
        }
      }
    }
  }


  void  KiteEngine::runGlobalRouter ( unsigned int mode )
  {
    if (getState() >= Katabatic::EngineGlobalLoaded)
      throw Error ("KiteEngine::runGlobalRouter(): Global routing already done or loaded.");

    Session::open( this );

    if (mode & KtLoadGlobalRouting) {
      // TODO: _knik->loadSolution();
      throw Error("Loading a previous solution is not supported by Spaghetti\n");
    } else {
      annotateGlobalGraph();
      map<Name,Net*>  preRouteds;
      for ( auto istate : getNetRoutingStates() ) {
        if (istate.second->isMixedPreRoute())
          preRouteds.insert( make_pair(istate.first, istate.second->getNet()) );
      }
      _globalRouter->run( preRouteds );
    }
    _globalRouter->saveRoutingSolution();
    setState( Katabatic::EngineGlobalLoaded );

    Session::close();
  }


  void  KiteEngine::loadGlobalRouting ( unsigned int method )
  {
    KatabaticEngine::loadGlobalRouting( method );

    Session::open( this );
    getGCellGrid()->checkEdgeOverflow( getHTracksReservedLocal(), getVTracksReservedLocal() );
    Session::close();
  }


  void  KiteEngine::runNegociate ( unsigned int flags )
  {
    if (_negociateWindow) return;

    startMeasures();
    Session::open( this );

    _negociateWindow = NegociateWindow::create( this );
    _negociateWindow->setGCells( *(getGCellGrid()->getGCellVector()) );
    _computeCagedConstraints();
    _negociateWindow->run( flags );
    _negociateWindow->destroy();
    _negociateWindow = NULL;

    Session::close();
    stopMeasures();
  //if ( _editor ) _editor->refresh ();

    printMeasures( "algo" );

    Session::open( this );
    unsigned int overlaps             = 0;
    size_t       hTracksReservedLocal = getHTracksReservedLocal();
    size_t       vTracksReservedLocal = getVTracksReservedLocal();

    if (cparanoid.enabled()) {
      cparanoid << "  o  Post-checking capacity overload h:" << hTracksReservedLocal
                << " v:." << vTracksReservedLocal << endl;
      getGCellGrid()->checkEdgeOverflow( hTracksReservedLocal, vTracksReservedLocal );
    }

    _check( overlaps );
    Session::close();

    _toolSuccess = _toolSuccess and (overlaps == 0);
  }


  void  KiteEngine::printCompletion () const
  {
    size_t                 routeds          = 0;
    unsigned long long     totalWireLength  = 0;
    unsigned long long     routedWireLength = 0;
    vector<TrackElement*>  unrouteds;
    ostringstream          result;

    AutoSegmentLut::const_iterator ilut = _getAutoSegmentLut().begin();
    for ( ; ilut != _getAutoSegmentLut().end() ; ilut++ ) {
      TrackElement* segment = _lookup( ilut->second );
      if (segment == NULL) continue;

      unsigned long long wl = (unsigned long long)DbU::toLambda( segment->getLength() );
      if (wl > 100000) {
        cerr << Error("KiteEngine::printCompletion(): Suspiciously long wire: %llu for %p:%s"
                     ,wl,ilut->first,getString(segment).c_str()) << endl;
        continue;
      }

      if (segment->isFixed() or segment->isBlockage()) continue;

      // if (segment->isSameLayerDogleg()) {
      //   cerr << "   Same layer:" << segment << endl;
      //   cerr << "     S: " << segment->base()->getAutoSource() << endl;
      //   cerr << "     T: " << segment->base()->getAutoTarget() << endl;
      // }

      totalWireLength += wl;
      if (segment->getTrack() != NULL) {
        routeds++;
        routedWireLength += wl;
      } else {
        unrouteds.push_back( segment );
      }
    }

    float segmentRatio    = (float)(routeds)          / (float)(routeds+unrouteds.size()) * 100.0;
    float wireLengthRatio = (float)(routedWireLength) / (float)(totalWireLength)   * 100.0;

    _toolSuccess = (unrouteds.empty());

    if (not unrouteds.empty()) {
      cerr << "  o  Routing did not complete, unrouted segments:" << endl;
      for ( size_t i=0; i<unrouteds.size() ; ++i ) {
        cerr << "   " << dec << setw(4) << (i+1) << "| " << unrouteds[i] << endl;
      }
    }

    result << setprecision(4) << segmentRatio
           << "% [" << routeds << "+" << unrouteds.size() << "]";
    cmess1 << Dots::asString( "     - Track Segment Completion Ratio", result.str() ) << endl;

    result.str("");
    result << setprecision(4) << wireLengthRatio
            << "% [" << totalWireLength << "+"
            << (totalWireLength - routedWireLength) << "]";
    cmess1 << Dots::asString( "     - Wire Length Completion Ratio", result.str() ) << endl;

    float expandRatio = 1.0;
    if (_minimumWL != 0.0) {
      expandRatio = ((totalWireLength-_minimumWL) / _minimumWL) * 100.0;

      result.str("");
      result << setprecision(3) << expandRatio << "% [min:" << setprecision(9) << _minimumWL << "]";
      cmess1 << Dots::asString( "     - Wire Length Expand Ratio", result.str() ) << endl;
    }

    addMeasure<size_t>            ( getCell(), "Segs"   , routeds+unrouteds.size() );
    addMeasure<unsigned long long>( getCell(), "DWL(l)" , totalWireLength                  , 12 );
    addMeasure<unsigned long long>( getCell(), "fWL(l)" , totalWireLength-routedWireLength , 12 );
    addMeasure<double>            ( getCell(), "WLER(%)", (expandRatio-1.0)*100.0 );
}


  void  KiteEngine::dumpMeasures ( ostream& out ) const
  {
    vector<Name> measuresLabels;
    measuresLabels.push_back( "Gates"   );
    measuresLabels.push_back( "GCells"  );
    measuresLabels.push_back( "knikT"   );
    measuresLabels.push_back( "knikS"   );
    measuresLabels.push_back( "GWL(l)"  );
    measuresLabels.push_back( "Area(l2)");
    measuresLabels.push_back( "Sat."    );
    measuresLabels.push_back( "loadT"   );
    measuresLabels.push_back( "loadS"   );
    measuresLabels.push_back( "Globals" );
    measuresLabels.push_back( "Edges"   );
    measuresLabels.push_back( "assignT" );
    measuresLabels.push_back( "algoT"   );
    measuresLabels.push_back( "algoS"   );
    measuresLabels.push_back( "finT"    );
    measuresLabels.push_back( "Segs"    );
    measuresLabels.push_back( "DWL(l)"  );
    measuresLabels.push_back( "fWL(l)"  );
    measuresLabels.push_back( "WLER(%)" );
    measuresLabels.push_back( "Events"  );
    measuresLabels.push_back( "UEvents" );

    const MeasuresSet* measures = Measures::get( getCell() );

    out << "#" << endl;
    out << "# " << getCell()->getName() << endl;
    out << measures->toStringHeaders(measuresLabels) << endl;
    out << measures->toStringDatas  (measuresLabels) << endl;

    measures->toGnuplot( "GCells Density Histogram", getString(getCell()->getName()) );
  }


  void  KiteEngine::dumpMeasures () const
  {
    ostringstream path;
    path << getCell()->getName() << ".knik-kite.dat";

    ofstream sfile ( path.str().c_str() );
    dumpMeasures( sfile );
    sfile.close();
  }


  bool  KiteEngine::_check ( unsigned int& overlap, const char* message ) const
  {
    cmess1 << "  o  Checking Kite Database coherency." << endl;

    bool coherency = true;
    coherency = coherency and KatabaticEngine::_check( message );
    for ( size_t i=0 ; i<_routingPlanes.size() ; i++ )
      coherency = _routingPlanes[i]->_check(overlap) and coherency;

    Katabatic::Session* ktbtSession = Session::base ();
    forEach ( Net*, inet, getCell()->getNets() ) {
      forEach ( Segment*, isegment, inet->getComponents().getSubSet<Segment*>() ) {
        AutoSegment* autoSegment = ktbtSession->lookup( *isegment );
        if (not autoSegment) continue;
        if (not autoSegment->isCanonical()) continue;

        TrackElement* trackSegment = Session::lookup( *isegment );
        if (not trackSegment) {
          coherency = false;
          cerr << Bug( "%p %s without Track Segment"
                     , autoSegment
                     , getString(autoSegment).c_str() ) << endl;
        } else
          trackSegment->_check();
      }
    }

    return coherency;
  }


  void  KiteEngine::finalizeLayout ()
  {
    ltrace(90) << "KiteEngine::finalizeLayout()" << endl;
    if (getState() > Katabatic::EngineDriving) return;

    ltracein(90);

    setState( Katabatic::EngineDriving );
    _gutKite();

    KatabaticEngine::finalizeLayout();
    ltrace(90) << "State: " << getState() << endl;

    getCell()->setFlags( Cell::Routed );

    ltraceout(90);
  }


  void  KiteEngine::_gutKite ()
  {
    ltrace(90) << "KiteEngine::_gutKite()" << endl;
    ltracein(90);
    ltrace(90) << "State: " << getState() << endl;

    if (getState() < Katabatic::EngineGutted) {
      Session::open( this );

      size_t maxDepth = getRoutingGauge()->getDepth();
      for ( size_t depth=0 ; depth < maxDepth ; depth++ ) {
        _routingPlanes[depth]->destroy();
      }

      Session::close();
    }

    ltraceout(90);
  }


  TrackElement* KiteEngine::_lookup ( Segment* segment ) const
  {
    AutoSegment* autoSegment = KatabaticEngine::_lookup( segment );
    if (not autoSegment or not autoSegment->isCanonical()) return NULL;

    return _lookup( autoSegment );
  }


  void  KiteEngine::_check ( Net* net ) const
  {
    cerr << "     o  Checking " << net << endl;

    forEach ( Segment*, isegment, net->getComponents().getSubSet<Segment*>() ) {
      TrackElement* trackSegment = _lookup( *isegment );
      if (trackSegment) {
        trackSegment->_check();

        AutoContact* autoContact = trackSegment->base()->getAutoSource();
        if (autoContact) autoContact->checkTopology ();

        autoContact = trackSegment->base()->getAutoTarget();
        if (autoContact) autoContact->checkTopology ();
      }
    }
  }


  string  KiteEngine::_getTypeName () const
  { return "Kite::KiteEngine"; }


  string  KiteEngine::_getString () const
  {
    ostringstream  os;
    os << "<" << "KiteEngine " << _cell->getName () << ">";
    return os.str();
  }


  Record* KiteEngine::_getRecord () const
  {
    Record* record = KatabaticEngine::_getRecord ();
                                     
    if (record) {
      record->add( getSlot( "_routingPlanes", &_routingPlanes ) );
      record->add( getSlot( "_configuration",  _configuration ) );
    }
    return record;
  }


} // Kite namespace.
