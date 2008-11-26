
// -*- C++ -*-
//
// This file is part of the Coriolis Software.
// Copyright (c) UPMC/LIP6 2008-2008, All Rights Reserved
//
// ===================================================================
//
// $Id$
//
// x-----------------------------------------------------------------x 
// |                                                                 |
// |                  H U R R I C A N E                              |
// |                                                                 |
// |  Author      :                    Jean-Paul CHAPUT              |
// |  E-mail      :       Jean-Paul.Chaput@asim.lip6.fr              |
// | =============================================================== |
// |  C++ Module  :       "./SelectionWidget.cpp"                    |
// | *************************************************************** |
// |  U p d a t e s                                                  |
// |                                                                 |
// x-----------------------------------------------------------------x


#include  <QFontMetrics>
#include  <QFrame>
#include  <QLabel>
#include  <QCheckBox>
#include  <QPushButton>
#include  <QLineEdit>
#include  <QHeaderView>
#include  <QKeyEvent>
#include  <QGroupBox>
#include  <QHBoxLayout>
#include  <QVBoxLayout>

#include "hurricane/Commons.h"
#include "hurricane/Cell.h"

#include "hurricane/viewer/Graphics.h"
//#include "hurricane/viewer/SelectionModel.h"
//#include "hurricane/viewer/SelectionWidget.h"
#include "hurricane/viewer/HSelectionModel.h"
#include "hurricane/viewer/HSelection.h"


namespace Hurricane {



  SelectionWidget::SelectionWidget ( QWidget* parent )
    : QWidget(parent)
    , _baseModel(new SelectionModel(this))
    , _sortModel(new QSortFilterProxyModel(this))
    , _view(new QTableView(this))
    , _filterPatternLineEdit(new QLineEdit(this))
    , _cumulative(new QCheckBox())
    , _showSelection(new QCheckBox())
    , _rowHeight(20)
  {
    setAttribute ( Qt::WA_DeleteOnClose );
    setAttribute ( Qt::WA_QuitOnClose, false );

    _rowHeight = QFontMetrics(Graphics::getFixedFont()).height() + 4;

    QLabel* filterPatternLabel = new QLabel(tr("&Filter pattern:"), this);
    filterPatternLabel->setBuddy(_filterPatternLineEdit);

    QHBoxLayout* hLayout1 = new QHBoxLayout ();
    hLayout1->addWidget ( filterPatternLabel );
    hLayout1->addWidget ( _filterPatternLineEdit );

    QFrame* separator = new QFrame ();
    separator->setFrameShape  ( QFrame::HLine );
    separator->setFrameShadow ( QFrame::Sunken );

    _cumulative->setText    ( tr("Cumulative Selection") );
    _cumulative->setChecked ( false );

    _showSelection->setText    ( tr("Show Selection") );
    _showSelection->setChecked ( false );

    QPushButton* clear = new QPushButton ();
    clear->setText ( tr("Clear") );

    QHBoxLayout* hLayout2 = new QHBoxLayout ();
    hLayout2->addWidget  ( _cumulative );
    hLayout2->addStretch ();
    hLayout2->addWidget  ( _showSelection );
    hLayout2->addStretch ();
    hLayout2->addWidget  ( clear );

    _sortModel->setSourceModel       ( _baseModel );
    _sortModel->setDynamicSortFilter ( true );
    _sortModel->setFilterKeyColumn   ( 0 );

    _view->setShowGrid(false);
    _view->setAlternatingRowColors(true);
    _view->setSelectionBehavior(QAbstractItemView::SelectRows);
    _view->setSortingEnabled(true);
    _view->setModel ( _sortModel );
    _view->horizontalHeader()->setStretchLastSection ( true );

    QHeaderView* horizontalHeader = _view->horizontalHeader ();
    horizontalHeader->setStretchLastSection ( true );
    horizontalHeader->setMinimumSectionSize ( 200 );

    QHeaderView* verticalHeader = _view->verticalHeader ();
    verticalHeader->setVisible ( false );

    QVBoxLayout* vLayout = new QVBoxLayout ();
  //vLayout->setSpacing ( 0 );
    vLayout->addLayout ( hLayout2 );
    vLayout->addWidget ( separator );
    vLayout->addWidget ( _view );
    vLayout->addLayout ( hLayout1 );

    setLayout ( vLayout );

    connect ( _filterPatternLineEdit, SIGNAL(textChanged(const QString &))
            , this                  , SLOT(textFilterChanged()) );
    connect ( _baseModel    , SIGNAL(layoutChanged()), this, SLOT(forceRowHeight()) );
    connect ( _cumulative   , SIGNAL(toggled(bool))  , this, SIGNAL(cumulativeToggled(bool)) );
    connect ( _showSelection, SIGNAL(toggled(bool))  , this, SIGNAL(showSelectionToggled(bool)) );
    connect ( clear         , SIGNAL(clicked())      , this, SIGNAL(selectionCleared()) );
    connect ( clear         , SIGNAL(clicked())      , _baseModel, SLOT(clear()) );
    connect ( _view->selectionModel(), SIGNAL(currentChanged(const QModelIndex&,const QModelIndex&))
            , this                   , SLOT(selectCurrent   (const QModelIndex&,const QModelIndex&)) );

    setWindowTitle ( tr("Selection<None>") );
    resize ( 500, 300 );
  }


  void  SelectionWidget::hideEvent ( QHideEvent* event )
  {
  //emit showSelected(false);
  }


  void  SelectionWidget::forceRowHeight ()
  {
    for (  int rows=_sortModel->rowCount()-1; rows >= 0 ; rows-- )
      _view->setRowHeight ( rows, _rowHeight );
  }


  void  SelectionWidget::keyPressEvent ( QKeyEvent* event )
  {
    if      ( event->key() == Qt::Key_I ) { inspect         ( _view->currentIndex() ); }
    else if ( event->key() == Qt::Key_T ) { toggleSelection ( _view->currentIndex() ); }
    else event->ignore();
  }


  void  SelectionWidget::textFilterChanged ()
  {
    _sortModel->setFilterRegExp ( _filterPatternLineEdit->text() );
    forceRowHeight ();
  }


  bool  SelectionWidget::isCumulative () const
  {
    return _cumulative->isChecked();
  }


  void  SelectionWidget::toggleSelection ( const QModelIndex& index )
  {
    Occurrence occurrence = _baseModel->toggleSelection ( index );
    if ( occurrence.isValid() )
      emit occurrenceToggled ( occurrence, false );
  }


  void  SelectionWidget::toggleSelection ( Occurrence occurrence )
  {
    _baseModel->toggleSelection ( occurrence );
  }


  void  SelectionWidget::setShowSelection ( bool state )
  {
    if ( state == _showSelection->isChecked() ) return;

    _showSelection->setChecked ( state );
    emit showSelection ( state );
  }


  void  SelectionWidget::setSelection ( const set<Selector*>& selection, Cell* cell )
  {
    _baseModel->setSelection ( selection );
     
    string windowTitle = "Selection";
    if ( cell ) windowTitle += getString(cell);
    else        windowTitle += "<None>";
    setWindowTitle ( tr(windowTitle.c_str()) );

    _view->selectRow ( 0 );
    _view->resizeColumnToContents ( 0 );
  }


  void  SelectionWidget::selectCurrent ( const QModelIndex& current, const QModelIndex& )
  {
    inspect ( current );
  }


  void  SelectionWidget::inspect ( const QModelIndex& index  )
  {
    if ( index.isValid() ) {
      Occurrence occurrence = _baseModel->getOccurrence ( _sortModel->mapToSource(index).row() );
      emit inspect ( getRecord(occurrence) );
    }
  }


} // End of Hurricane namespace.
