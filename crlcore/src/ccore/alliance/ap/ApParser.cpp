
// -*- C++ -*-
//
// This file is part of the Coriolis Project.
// Copyright (C) Laboratoire LIP6 - Departement ASIM
// Universite Pierre et Marie Curie
//
// Main contributors :
//        Christophe Alexandre   <Christophe.Alexandre@lip6.fr>
//        Sophie Belloeil             <Sophie.Belloeil@lip6.fr>
//        Hugo Cl�ment                   <Hugo.Clement@lip6.fr>
//        Jean-Paul Chaput           <Jean-Paul.Chaput@lip6.fr>
//        Damien Dupuis                 <Damien.Dupuis@lip6.fr>
//        Christian Masson           <Christian.Masson@lip6.fr>
//        Marek Sroka                     <Marek.Sroka@lip6.fr>
// 
// The  Coriolis Project  is  free software;  you  can redistribute it
// and/or modify it under the  terms of the GNU General Public License
// as published by  the Free Software Foundation; either  version 2 of
// the License, or (at your option) any later version.
// 
// The  Coriolis Project is  distributed in  the hope that it  will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY  or FITNESS FOR  A PARTICULAR PURPOSE.   See the
// GNU General Public License for more details.
// 
// You should have  received a copy of the  GNU General Public License
// along with the Coriolis Project; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
//
// License-Tag
// Authors-Tag
//
// ===================================================================
//
// $Id$
//
// x-----------------------------------------------------------------x
// |                                                                 |
// |                   C O R I O L I S                               |
// |          Alliance / Hurricane  Interface                        |
// |                                                                 |
// |  Author      :                    Christian Masson              |
// |  E-mail      :            Christian.Masson@lip6.fr              |
// | =============================================================== |
// |  C++ Module  :       "./ApParser.cpp"                           |
// | *************************************************************** |
// |  U p d a t e s                                                  |
// |                                                                 |
// |  05/06/2008 - Jean-Paul Chaput                                  |
// |    Complete rewrite & simplification.                           |
// x-----------------------------------------------------------------x


#include  <cstdio>
#include  <cstdlib>
#include  <cstring>
#include  <cstdarg>

#include  "hurricane/DataBase.h"
#include  "hurricane/Technology.h"
#include  "hurricane/Pin.h"
#include  "hurricane/Contact.h"
#include  "hurricane/Vertical.h"
#include  "hurricane/Horizontal.h"
#include  "hurricane/UpdateSession.h"
#include  "hurricane/NetExternalComponents.h"

#include  "crlcore/Utilities.h"
#include  "crlcore/AllianceFramework.h"
#include  "crlcore/ToolBox.h"
#include  "Ap.h"


namespace {


# define    LINE_SIZE    4096


  using namespace Hurricane;
  using namespace CRL;


  class LayerInformation {
    public:
            Name   _layerName;
            bool   _isConnector;
            bool   _isBlockage;
      const Layer* _layer;
    public:
                   LayerInformation ();
                   LayerInformation ( const Name&  layerName
                                    , const bool   isConnector
                                    , const bool   isBlockage
                                    , const Layer* layer
                                    );
      bool         isConnector      () const;
      bool         isBlockage       () const;
      Name         getName          () const;
      const Layer* getLayer         () const;
  };


  LayerInformation::LayerInformation () : _layerName()
                                        , _isConnector(false)
                                        , _isBlockage(false)
                                        , _layer(NULL)
  { }


  LayerInformation::LayerInformation ( const Name&  layerName
                                     , const bool   isConnector
                                     , const bool   isBlockage
                                     , const Layer* layer
                                     ) : _layerName(layerName)
                                       , _isConnector(isConnector)
                                       , _isBlockage(isBlockage)
                                       , _layer(layer)
  { }


  bool  LayerInformation::isConnector () const
  { return _isConnector; }


  bool  LayerInformation::isBlockage () const
  { return _isBlockage; }


  Name  LayerInformation::getName () const
  { return _layerName; }


  const Layer* LayerInformation::getLayer () const
  { return _layer; }


  class LayerInformations : public map<const Name,LayerInformation> {
    public:
      void        setTechnology  ( Technology* technology );
      void        add            ( const Name& apLayer
                                 , const Name& hLayer
                                 , bool        isConnectorapLayer
                                 , bool        isBlockage
                                 );
    private:
      Technology* _technology;
  };


  void  LayerInformations::setTechnology ( Technology* technology )
  { _technology = technology; }


  void  LayerInformations::add ( const Name& apLayer
                               , const Name& hLayer
                               , bool        isConnector
                               , bool        isBlockage
                               )
  {
    insert ( make_pair( apLayer
                      , LayerInformation(hLayer
                                        ,isConnector
                                        ,isBlockage
                                        ,_technology->getLayer(hLayer))
                      )
           );
  }


  class ApParser {
    public:
    // Methods.
                   ApParser     ( AllianceFramework* af );
             void  loadFromFile ( const string& cellPath, Cell* cell );

    private:
    // Internal: enum.
             enum ParserState { StateVersion
                              , StateHeader
                              , StateBody
                              , StateEOF
                              };
    // Internal: static Attributes.
      static LayerInformations  _layerInformations;
    // Internal: Attributes.
             AllianceFramework* _framework;
             string             _cellPath;
             Cell*              _cell;
             double             _scaleRatio;
             unsigned int       _anonymousId;
             int                _parserState;
             size_t             _lineNumber;
             char               _rawLine[LINE_SIZE];

    protected:
    // Internal: Methods.
      static LayerInformation*  _getLayerInformation ( const Name& layerName );
      inline DbU::Unit          _getUnit             ( long value );
      inline DbU::Unit          _getUnit             ( const char* value );
             vector<char*>      _splitString         ( char* s, char separator );
             Net*               _getNet              ( const char* apName );
             Net*               _getAnonymousNet     ();
             Net*               _safeGetNet          ( const char* apName );
             void               _parseVersion        ();
             void               _parseHeader         ();
             void               _parseAbutmentBox    ();
             void               _parseReference      ();
             void               _parseConnector      ();
             void               _parseVia            ();
             void               _parseBigVia         ();
             void               _parseSegment        ();
             void               _parseInstance       ();
             void               _printWarning        ( const char* format, ... );
             void               _printError          ( bool interrupt, const char* format, ... );
  };


  LayerInformations  ApParser::_layerInformations;


  ApParser::ApParser ( AllianceFramework* framework )
    : _framework(framework)
    , _cellPath()
    , _cell(NULL)
    , _scaleRatio(100.0)
    , _anonymousId(0)
    , _parserState(StateVersion)
    , _lineNumber(0)
  {
    if ( _layerInformations.empty() ) {
      _layerInformations.setTechnology ( DataBase::getDB()->getTechnology() );

      _layerInformations.add ( "NWELL"      , "NWELL"      , false, false );
      _layerInformations.add ( "PWELL"      , "PWELL"      , false, false );
      _layerInformations.add ( "NTIE"       , "NTIE"       , false, false );
      _layerInformations.add ( "PTIE"       , "PTIE"       , false, false );
      _layerInformations.add ( "NDIF"       , "NDIF"       , false, false );
      _layerInformations.add ( "PDIF"       , "PDIF"       , false, false );
      _layerInformations.add ( "NTRANS"     , "NTRANS"     , false, false );
      _layerInformations.add ( "PTRANS"     , "PTRANS"     , false, false );
      _layerInformations.add ( "POLY"       , "POLY"       , false, false );
      
      _layerInformations.add ( "ALU1"       , "METAL1"     , false, false );
      _layerInformations.add ( "ALU2"       , "METAL2"     , false, false );
      _layerInformations.add ( "ALU3"       , "METAL3"     , false, false );
      _layerInformations.add ( "ALU4"       , "METAL4"     , false, false );
      _layerInformations.add ( "ALU5"       , "METAL5"     , false, false );
      _layerInformations.add ( "ALU6"       , "METAL6"     , false, false );

      _layerInformations.add ( "CALU1"      , "METAL1"     ,  true, false );
      _layerInformations.add ( "CALU2"      , "METAL2"     ,  true, false );
      _layerInformations.add ( "CALU3"      , "METAL3"     ,  true, false );
      _layerInformations.add ( "CALU4"      , "METAL4"     ,  true, false );
      _layerInformations.add ( "CALU5"      , "METAL5"     ,  true, false );
      _layerInformations.add ( "CALU6"      , "METAL6"     ,  true, false );

      _layerInformations.add ( "TALU1"      , "BLOCKAGE1"  , false,  true );
      _layerInformations.add ( "TALU2"      , "BLOCKAGE2"  , false,  true );
      _layerInformations.add ( "TALU3"      , "BLOCKAGE3"  , false,  true );
      _layerInformations.add ( "TALU4"      , "BLOCKAGE4"  , false,  true );
      _layerInformations.add ( "TALU5"      , "BLOCKAGE5"  , false,  true );
      _layerInformations.add ( "TALU6"      , "BLOCKAGE6"  , false,  true );

      _layerInformations.add ( "CONT_BODY_N", "CONT_BODY_N", false, false );
      _layerInformations.add ( "CONT_BODY_P", "CONT_BODY_P", false, false );
      _layerInformations.add ( "CONT_DIF_N" , "CONT_DIF_N" , false, false );
      _layerInformations.add ( "CONT_DIF_P" , "CONT_DIF_P" , false, false );
      _layerInformations.add ( "CONT_POLY"  , "CONT_POLY"  , false, false );
      _layerInformations.add ( "CONT_VIA"   , "VIA12"      , false, false );
      _layerInformations.add ( "CONT_VIA2"  , "VIA23"      , false, false );
      _layerInformations.add ( "CONT_VIA3"  , "VIA34"      , false, false );
      _layerInformations.add ( "CONT_VIA4"  , "VIA45"      , false, false );
      _layerInformations.add ( "CONT_VIA5"  , "VIA56"      , false, false );
      _layerInformations.add ( "CONT_TURN1" , "METAL1"     , false, false );
      _layerInformations.add ( "CONT_TURN2" , "METAL2"     , false, false );
      _layerInformations.add ( "CONT_TURN3" , "METAL3"     , false, false );
      _layerInformations.add ( "CONT_TURN4" , "METAL4"     , false, false );
      _layerInformations.add ( "CONT_TURN5" , "METAL5"     , false, false );
      _layerInformations.add ( "CONT_TURN6" , "METAL6"     , false, false );
    }
  }


  LayerInformation* ApParser::_getLayerInformation ( const Name& layerName )
  {
    map<const Name, LayerInformation>::iterator  it = _layerInformations.find ( layerName );
    if ( it != _layerInformations.end() )
      return &(it->second);

    return NULL;
  }


  vector<char*>  ApParser::_splitString ( char* s, char separator )
  {
    vector<char*>  fields;

    fields.push_back ( s );
    while ( *s != '\0' ) {
      if ( *s == separator ) {
        fields.push_back ( s+1 );
        *s = '\0';
      }
      s++;
    }

    return fields;
  }


  inline DbU::Unit  ApParser::_getUnit ( long value )
  {
    return DbU::lambda ( _scaleRatio*value );
  }


  inline DbU::Unit  ApParser::_getUnit ( const char* value )
  {
    char* end;
    long  convert = strtol ( value, &end, 10 );

    if ( *end != '\0' )
      _printError ( false
                  , "Incomplete string to integer conversion for \"%s\" (%ld)."
                  , value
                  , convert
                  );

    return _getUnit ( convert );
  }


  Net* ApParser::_getNet ( const char* apName )
  {
    string hName = apName;

    size_t  separator = hName.find ( ' ' );
    if ( separator != string::npos ) {
      hName[separator] = '(';
      hName += ')';
    }

    Net* net = _cell->getNet ( hName );
    if ( !net ) {
      net = Net::create ( _cell, hName );
      if ( _framework->isPOWER(hName) ) {
        net->setType   ( Net::Type::POWER );
        net->setGlobal ( true );
      }
      if ( _framework->isGROUND(hName) ) {
        net->setType   ( Net::Type::GROUND );
        net->setGlobal ( true );
      }
    }

    return net;
  }


  Net* ApParser::_getAnonymousNet ()
  {
    ostringstream  anonymousName ( "anonymous_" );
    anonymousName << _anonymousId++;

    Net* net = Net::create ( _cell, anonymousName.str() );
    net->setAutomatic ( true );
    return net;
  }


  Net* ApParser::_safeGetNet ( const char* apName )
  {
    if ( ( apName[0] == '\0' ) || !strcmp(apName,"*") )
      return _getAnonymousNet ();

    return _getNet ( apName );
  }


  void  ApParser::_parseVersion ()
  {
    if ( strncmp(_rawLine,"V ALLIANCE : ",13) )
      _printError ( true, "Missing Alliance Version Header." );

    int version = atoi ( _rawLine+13 );
    if ( version < 3 )
      _printError ( true, "AP version prior to 3 are not supporteds (version: %d).", version );
  }


  void  ApParser::_parseHeader ()
  {
    if ( _rawLine[0] != 'H' ) _printError ( true, "Missing Cell Header." );
          
    vector<char*>  fields = _splitString ( _rawLine+2, ',' );

    if ( fields.size() < 4 )
      _printError ( true, "Malformed header line." );

    Name  cellName    = fields[0];
          _scaleRatio = 1 / strtod ( fields[3], NULL );

    if ( cellName != _cell->getName() )
      _printError ( true
                  , "Cell name discrepency <Cell %s>: file refers to %s."
                  , getString(_cell->getName()).c_str()
                  , getString(cellName).c_str()
                  );
  }


  void  ApParser::_parseAbutmentBox ()
  {
    if ( _cell->getAbutmentBox().isEmpty() ) {
      DbU::Unit  XAB1 = 0;
      DbU::Unit  YAB1 = 0;
      DbU::Unit  XAB2 = 10;
      DbU::Unit  YAB2 = 10;

      vector<char*>  fields = _splitString ( _rawLine+2, ',' );
      if ( fields.size() < 4 )
        _printError ( false, "Malformed Abutment Box line." );
      else {
        XAB1 = _getUnit ( fields[0] );
        YAB1 = _getUnit ( fields[1] );
        XAB2 = _getUnit ( fields[2] );
        YAB2 = _getUnit ( fields[3] );
      }

      _cell->setAbutmentBox ( Box(XAB1, YAB1, XAB2, YAB2) );
    } else
      _printWarning ( "Duplicated Abutment Box line (ignored)." );
  }


  void  ApParser::_parseReference ()
  {
    static DbU::Unit  XREF, YREF;

    vector<char*>  fields = _splitString ( _rawLine+2, ',' );
    if ( fields.size() < 4 )
      _printError ( false, "Malformed Reference line." );
    else {
      XREF = _getUnit ( fields[0] );
      YREF = _getUnit ( fields[1] );

      if ( !strcmp(fields[2],"ref_ref") )
        Reference::create ( _cell, fields[3], XREF, YREF  );
    }
  }


  void  ApParser::_parseConnector ()
  {
    static DbU::Unit                  XCON, YCON, WIDTH;
    static unsigned int          index;
    static char                  pinName[1024];
    static Net*                  net;
    static Pin*                  pin;
    static LayerInformation*     layerInfo;
    static Pin::AccessDirection  accessDirection;
    static Name                  orientation;
    static Name                  NORTH = "NORTH";
    static Name                  SOUTH = "SOUTH";
    static Name                  EAST  = "EAST";
    static Name                  WEST  = "WEST";

    vector<char*>  fields = _splitString ( _rawLine+2, ',' );
    if ( fields.size() < 7 )
      _printError ( false, "Malformed Connector line." );
    else {
      XCON        = _getUnit ( fields[0] );
      YCON        = _getUnit ( fields[1] );
      WIDTH       = _getUnit ( fields[2] );
      index       = atoi(fields[4]);
      orientation = fields[5];

      size_t length = strlen ( fields[3] );
      if ( length > 1000 ) {
        _printError ( false, "Connector name too long (exceed 1000 characters)." );
        return;
      }

      strncpy ( pinName, fields[3], 1023 );
      if ( index > 0 ) {
        pinName [ length ] = '.';
        strncpy ( pinName+length+1, fields[4], 1022-length );
      }

      net       = _getNet              ( fields[5] );
      layerInfo = _getLayerInformation ( fields[6] );

      if      ( orientation == NORTH ) accessDirection = Pin::AccessDirection::NORTH;
      else if ( orientation == WEST  ) accessDirection = Pin::AccessDirection::WEST;
      else if ( orientation == SOUTH ) accessDirection = Pin::AccessDirection::SOUTH;
      else if ( orientation == EAST  ) accessDirection = Pin::AccessDirection::EAST;
      else accessDirection = Pin::AccessDirection::UNDEFINED;

      if ( layerInfo && net ) {
        net->setExternal ( true );
        pin = Pin::create ( net
                          , pinName
                          , accessDirection
                          , Pin::PlacementStatus::PLACED
                          , layerInfo->getLayer()
                          , XCON
                          , YCON
                          , WIDTH
                          , WIDTH
                          );
      //setExternal ( pin );
      }
      if ( !layerInfo )
        _printError ( false, "Unknown layer name %s.", fields[6] );
    }
  }


  void  ApParser::_parseVia ()
  {
    static DbU::Unit              XVIA, YVIA;
    static Net*              net;
    static LayerInformation* layerInfo;

    vector<char*>  fields = _splitString ( _rawLine+2, ',' );
    if ( fields.size() < 4 )
      _printError ( false, "Malformed VIA line." );
    else {
      XVIA      = _getUnit ( fields[0] );
      YVIA      = _getUnit ( fields[1] );
      net       = _safeGetNet          ( fields[3] );
      layerInfo = _getLayerInformation ( fields[2] );

      if ( layerInfo )
        Contact::create ( net, layerInfo->getLayer(), XVIA, YVIA );
      else
        _printError ( false, "Unknown layer name %s.", fields[2] );
    }
  }


  void  ApParser::_parseBigVia ()
  {
    static DbU::Unit              XVIA, YVIA, WIDTH, HEIGHT;
    static Net*              net;
    static LayerInformation* layerInfo;

    vector<char*>  fields = _splitString ( _rawLine+2, ',' );
    if ( fields.size() < 6 )
      _printError ( false, "Malformed big VIA line." );
    else {
      XVIA      = _getUnit ( fields[0] );
      YVIA      = _getUnit ( fields[1] );
      WIDTH     = _getUnit ( fields[2] );
      HEIGHT    = _getUnit ( fields[3] );
      net       = _safeGetNet          ( fields[5] );
      layerInfo = _getLayerInformation ( fields[4] );

      if ( layerInfo )
        Contact::create ( net, layerInfo->getLayer(), XVIA, YVIA, WIDTH, HEIGHT );
      else
        _printError ( false, "Unknown layer name %s.", fields[4] );
    }
  }


  void  ApParser::_parseSegment ()
  {
    static DbU::Unit         X1, Y1, X2, Y2, WIDTH;
    static Net*              net;
    static LayerInformation* layerInfo;

    vector<char*>  fields = _splitString ( _rawLine+2, ',' );
    if ( fields.size() < 8 )
      _printError ( false, "Malformed Segment line." );
    else {
      X1        = _getUnit    ( fields[0] );
      Y1        = _getUnit    ( fields[1] );
      X2        = _getUnit    ( fields[2] );
      Y2        = _getUnit    ( fields[3] );
      WIDTH     = _getUnit    ( fields[4] );
      net       = _safeGetNet ( fields[5] );
      layerInfo = _getLayerInformation ( fields[7] );

      if ( layerInfo ) {
        Segment* segment = NULL;
        if ( X1 == X2 ) {
          segment =  Vertical::create ( net, layerInfo->getLayer(), X1, WIDTH, Y1, Y2 );
        } else if ( Y1 == Y2 ) {
          segment = Horizontal::create ( net, layerInfo->getLayer(), Y1, WIDTH, X1, X2 );
        } else { 
          _printError ( false, "Segment neither horizontal nor vertical." );
        }
        if ( layerInfo->isConnector()) {
          if ( not net->isExternal() ) {
            _printWarning ( "External component on non-external net %s", getString(net->getName()).c_str() );
            net->setExternal ( true );
          }
          NetExternalComponents::setExternal(segment);
        }
      }
      else
        _printError ( false, "Unknown layer name %s.", fields[7] );
    }
  }


  void  ApParser::_parseInstance ()
  {
    static DbU::Unit  XINS, YINS;
    static Name  masterCellName;
    static Name  instanceName;
    static Name  orientName;
    static Transformation::Orientation
                 orient         = Transformation::Orientation::ID;
    static Name  NOSYM          = "NOSYM";
    static Name  SYM_X          = "SYM_X";
    static Name  SYM_Y          = "SYM_Y";
    static Name  SYMXY          = "SYMXY";
    static Name  ROT_P          = "ROT_P";
    static Name  ROT_M          = "ROT_M";
    static Name  SY_RM          = "SY_RM";
    static Name  SY_RP          = "SY_RP";

    vector<char*>  fields = _splitString ( _rawLine+2, ',' );
    if ( fields.size() < 5 )
      _printError ( false, "Malformed instance line." );
    else {
      XINS           = _getUnit ( fields[0] );
      YINS           = _getUnit ( fields[1] );
      masterCellName = fields[2];
      instanceName   = fields[3];
      orientName     = fields[4];

      if      (orientName == "NOSYM") orient = Transformation::Orientation::ID;
      else if (orientName == "ROT_P") orient = Transformation::Orientation::R1;
      else if (orientName == "SYMXY") orient = Transformation::Orientation::R2;
      else if (orientName == "ROT_M") orient = Transformation::Orientation::R3;
      else if (orientName == "SYM_X") orient = Transformation::Orientation::MX;
      else if (orientName == "SY_RM") orient = Transformation::Orientation::XR;
      else if (orientName == "SYM_Y") orient = Transformation::Orientation::MY;
      else if (orientName == "SY_RP") orient = Transformation::Orientation::YR;
      else
        _printError ( false, "Unknown orientation (%s).", getString(orientName).c_str() );

      Instance* instance = _cell->getInstance ( instanceName );
      if ( instance ) {
        instance->setTransformation
          ( getTransformation ( instance->getMasterCell()->getAbutmentBox()
                              , XINS
                              , YINS
                              , orient
                              )
          );
        instance->setPlacementStatus ( Instance::PlacementStatus::FIXED );
      } else {
        Catalog::State* instanceState  = _framework->getCatalog()->getState ( masterCellName );
        if ( !instanceState || (!instanceState->isFeed()) ) {
          _printError ( false
                      , "No logical instance associated to physical instance %s."
                      , getString(instanceName).c_str()
                      );
          return;
        }

      // Load a cell that is not in the logical view. Only feedthrough Cell
      // could be in that case.
        tab++;
        Cell* masterCell = _framework->getCell ( getString(masterCellName)
                                               , Catalog::State::Views
                                               );
        tab--;

        if ( !masterCell ) {
          _printError ( "Unable to load model %s.", getString(masterCellName).c_str() );
          return;
        }

        instance = Instance::create ( _cell
                                    , instanceName
                                    , masterCell
                                    , getTransformation ( masterCell->getAbutmentBox()
                                                        , XINS
                                                        , YINS
                                                        , orient
                                                        )
                                    , Instance::PlacementStatus::FIXED
                                    , true  // Checking of recursive calls
                                    );
        _cell->setTerminal ( false );
      }
    }
  }


  void  ApParser::_printWarning ( const char* format, ... )
  {
    static char     formatted [ 8192 ];
           va_list  args;

    va_start ( args, format );
    vsnprintf ( formatted, 8191, format, args );
    va_end ( args );

    cerr << "[WARNING] ApParser(): " << formatted << "\n"
         << "          (file: " << _cellPath << ", line: " << _lineNumber << ")" << endl;
  }


  void  ApParser::_printError ( bool interrupt, const char* format, ... )
  {
    static char     formatted [ 8192 ];
           va_list  args;

    va_start ( args, format );
    vsnprintf ( formatted, 8191, format, args );
    va_end ( args );

    cerr << "[ERROR] ApParser(): " << formatted << "\n"
         << "        (file: " << _cellPath << ", line: " << _lineNumber << ")" << endl;

    if ( interrupt )
      throw Error ( "ApParser processed" );
  }


  void  ApParser::loadFromFile ( const string& cellPath, Cell* cell )
  {
    if ( !cell ) throw Error ( "ApParser::loadFromFile(): Cell argument is NULL." );

    _cell     = cell;
    _cellPath = cellPath;

    CatalogProperty *catalogProperty
      = (CatalogProperty*)cell->getProperty ( CatalogProperty::getPropertyName() );

    if ( catalogProperty == NULL )
      throw Error ( "Missing CatalogProperty in cell %s.\n" , getString(cell->getName()).c_str() );

    Catalog::State *state = catalogProperty->getState ();

    state->setPhysical ( true );
    if ( state->isFlattenLeaf() ) _cell->setFlattenLeaf ( true );

    IoFile fileStream ( cellPath );
    fileStream.open ( "r" );

    UpdateSession::open();

    bool  materializationState = Go::autoMaterializationIsDisabled ();
    Go::disableAutoMaterialization ();

    _lineNumber  = 0;
    _parserState = StateVersion;
    _scaleRatio  = 100.0;

    try {
      while ( !fileStream.eof() ) {
        fileStream.readLine ( _rawLine, LINE_SIZE );
        _lineNumber++;

        if ( _rawLine[0] == '\0' ) {
          if ( _parserState == StateEOF ) break;

          _printError ( true, "Premature end of file." );
        } else {
          if ( _parserState == StateEOF )
            _printError ( true, "Garbage after EOF." );
        }
        if ( !strcmp(_rawLine,"EOF") ) { _parserState = StateEOF; continue; }

        if ( _parserState == StateVersion ) {
          _parseVersion ();
          _parserState = StateHeader;
          continue;
        }

        if ( _parserState == StateHeader ) {
          _parseHeader ();
          _parserState = StateBody;
          continue;
        }

        if ( _parserState == StateBody ) {
          switch ( _rawLine[0] ) {
            case 'A': _parseAbutmentBox (); break;
            case 'R': _parseReference   (); break;
            case 'V': _parseVia         (); break;
            case 'B': _parseBigVia      (); break;
            case 'S': _parseSegment     (); break;
            case 'I': _parseInstance    (); break;
          }
        }
      }
    } catch ( Error& e ) {
      if ( e.what() != "[ERROR] ApParser processed" )
        cerr << e.what() << endl;
    }

    Go::enableAutoMaterialization();
    _cell->materialize();

    UpdateSession::close ();

    if (materializationState) Go::disableAutoMaterialization ();

    fileStream.close ();
  }


} // End of anonymous namespace.


namespace CRL {


void  apParser ( const string& cellPath, Cell *cell )
{
  cmess2 << "     " << tab << "+ " << cellPath << endl;

  ApParser  parser ( AllianceFramework::get() );

  parser.loadFromFile ( cellPath, cell );
}


} // End of CRL namespace.