#include<QMenu>
#include<QMenuBar>
#include<QAction>

#include "MainWindow.h"
#include "MyWidget.h"

MainWindow::MainWindow() 
{
    MyWidget* mWidget = new MyWidget(this);
    setCentralWidget(mWidget);

    QAction* configAct = new QAction(tr("&Configure"), this);
    configAct->setStatusTip(tr("Configure the application"));
    connect(configAct, SIGNAL(triggered()), mWidget, SLOT(reconfig()));
    QAction* aboutAct = new QAction(tr("&About"), this);
    aboutAct->setStatusTip(tr("Show the about dialog box"));
    connect(aboutAct, SIGNAL(triggered()), mWidget, SLOT(about()));
    QMenu* editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(configAct);
    QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAct);
} 
