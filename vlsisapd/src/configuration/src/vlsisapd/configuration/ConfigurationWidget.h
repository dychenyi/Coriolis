
// -*- C++ -*-
//
// This file is part of the VSLSI Stand-Alone Software.
// Copyright (c) UPMC/LIP6 2010-2010, All Rights Reserved
//
// ===================================================================
//
// $Id$
//
// x-----------------------------------------------------------------x
// |                                                                 |
// |                   C O R I O L I S                               |
// |    C o n f i g u r a t i o n   D a t a - B a s e                |
// |                                                                 |
// |  Author      :                    Jean-Paul Chaput              |
// |  E-mail      :            Jean-Paul.Chaput@lip6.fr              |
// | =============================================================== |
// |  C++ Header  :       "./ConfigurationWidget.h"                  |
// | *************************************************************** |
// |  U p d a t e s                                                  |
// |                                                                 |
// x-----------------------------------------------------------------x



#ifndef  __CFG_CONFIGURATION_WIDGET__
#define  __CFG_CONFIGURATION_WIDGET__

//#include  <map>
#include  <QFont>
#include  <QTabWidget>


namespace Cfg {

  class Configuration;
  class Parameter;
  class ParameterWidget;
  class ConfTabWidget;


// -------------------------------------------------------------------
// Class  :  "Cfg::ConfigurationWidget".


  class ConfigurationWidget : public QTabWidget {
      Q_OBJECT;
    public:
                       ConfigurationWidget ( QWidget* parent=NULL );
    public:            
      QFont&           getBoldFont         ();
      ParameterWidget* find                ( Parameter* ) const;
      ParameterWidget* find                ( const std::string& id ) const;
      ConfTabWidget*   findOrCreate        ( const std::string& name );
      void             addRuler            ( const std::string& tabName );
      void             addTitle            ( const std::string& tabName, const std::string& title );
      void             addSection          ( const std::string& tabName, const std::string& section, int column=0 );
      ParameterWidget* addParameter        ( const std::string& tabName, Parameter*, const std::string& label, int column=0, int flags=0 );
      void             syncSlaves          ();
    private:
      QFont          _boldFont;
  };


// Functions Templates.
  template<typename QTypeWidget>
  QTypeWidget  rparent ( QObject* object )
  {
    QTypeWidget root = NULL;

    while ( object != NULL ) {
      object = object->parent();
      root   = qobject_cast<QTypeWidget>(object);
      if ( root != NULL )
        return root;
    }
    return NULL;
  }


} // End of Cfg namespace.


#endif  // __CFG_CONFIGURATION_WIDGET__