/*
  Copyright (c) 2009 Riccardo Iaconelli <riccardo@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include <QDockWidget>
#include <QListWidgetItem>
#include <QModelIndex>
#include <QLabel>
#include <QGridLayout>

#include <KTextEdit>

#include <KAction>
#include <KConfig>
#include <KConfigGroup>
#include <KDebug>
#include <KMenu>
#include <KMenuBar>
#include <KStandardAction>
#include <KUrl>
#include <KListWidget>
#include <KActionCollection>
#include <KParts/Part>

#include "startpage.h"
#include "sidebar.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : KParts::MainWindow(parent, 0),
    m_factory(0),
    m_part(0),
    oldTab(0) // we start from startPage
{    
    setXMLFile("plasmateui.rc");
    createMenus();
    
        // this routine will find and load our Part.  
//     m_factory = KLibLoader::self()->factory("katepart");
//     if (m_factory)
//     {
//         // now that the Part is loaded, we cast it to a Part to get
//         // our hands on it
//         m_part = static_cast<KParts::ReadWritePart *>
//                  (m_factory->create(this, "KatePart" ));
//  
//         if (m_part)
//         {
//             // tell the KParts::MainWindow that this is indeed
//             // the main widget
//             setCentralWidget(m_part->widget());
//  
//             setupGUI(ToolBar | Keys | StatusBar | Save);
//  
//             // and integrate the part's GUI with the shell's
//             createGUI(m_part);
//         }
//     }
//     
    m_startPage = new StartPage(this);
    connect(m_startPage, SIGNAL(projectSelected(QString)), this, SLOT(loadProject(QString)));
    setCentralWidget(m_startPage);
}

MainWindow::~MainWindow()
{
    delete m_startPage;
    if (m_factory) {
        delete m_factory;
    }
    
    if (m_part) {
        delete m_part;
    }
    
    delete sidebar;
    delete workflow;
}

void MainWindow::createMenus()
{
    KStandardAction::quit(this, SLOT(quit()), actionCollection());  
    menuBar()->addMenu(helpMenu());
    setupGUI();
}

void MainWindow::createDockWidgets()
{
    workflow = new QDockWidget(i18n("Workflow"), this);
    
    sidebar = new Sidebar(workflow);
    
    sidebar->addItem(KIcon("go-home"), i18n("Start page"));
    sidebar->addItem(KIcon("accessories-text-editor"), i18n("Edit"));
    sidebar->addItem(KIcon("krfb"), i18n("Publish"));
    sidebar->addItem(KIcon("help-contents"), i18n("Documentation"));
    sidebar->addItem(KIcon("system-run"), i18n("Preview"));
    
    workflow->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
        
    connect(sidebar, SIGNAL(currentIndexChanged(int)),
            this, SLOT(changeTab(int)));
    
    workflow->setWidget(sidebar);
    addDockWidget(Qt::LeftDockWidgetArea, workflow);
}

void MainWindow::quit()
{
    qApp->closeAllWindows();
//     deleteLater();
}

void MainWindow::showKatePart()
{
    if (!m_factory) {
        m_factory = KLibLoader::self()->factory("katepart");
    
        m_part = 0;
        if (m_factory) {
            kDebug() << "babla";
            m_part = static_cast<KParts::ReadWritePart *>(m_factory->create(this, "KatePart"));
            if (m_part) {
                kDebug() << "if mpart";
                setCentralWidget(m_part->widget());
                kDebug() << "set";
    //                 setupGUI(ToolBar | Keys | StatusBar | Save);
                kDebug() << "setup or create?!;";
                //createGUI(m_part);
                kDebug() << "created";
            } 
        }
    }
    if (m_part) {
        kDebug() << "we do have m_part";
        m_part = static_cast<KParts::ReadWritePart *>(m_factory->create(this, "KatePart"));
        setCentralWidget(m_part->widget());
        kDebug() << "and it's not even crashy";
//             kDebug() << "setCentralWidget i said";
//             setupGUI();
        createGUI(m_part);
        kDebug() << "woo, we created a GUI";
    }
}

void MainWindow::hideKatePart()
{
    m_part->closeUrl();
    createGUI(0);
}

void MainWindow::changeTab(int tab)
{
//     kDebug() << "Clicked sidebar item number" << tab;
    
    if (tab == oldTab) { // user clicked on the current tab 
        if (tab == 0) {
            m_startPage->resetStatus();
        }
        
        return;
    }
    
//     centralWidget()->deleteLater(); // clean
    if (oldTab == 1) {
        hideKatePart();
    } else {
        centralWidget()->deleteLater();
    }
//     kDebug() << "here we are";
    
    kDebug() << "deletalater";
    
    if (tab == 0) {
        m_startPage = new StartPage(this);
        setCentralWidget(m_startPage);
    } else if (tab == 1) {
        kDebug() << "Show Kate Part!";
        showKatePart();
        kDebug() << "kate part without crashing, please";
    } else if (tab == 2) {
        QLabel *l = new QLabel(i18n("Publish widget will go here!"));
        setCentralWidget(l);
    } else if (tab == 3) {
        QLabel *l = new QLabel(i18n("Documentation widget will go here!"));
        setCentralWidget(l);
    } else if (tab == 4) {
        QLabel *l = new QLabel(i18n("Preview widget will go here!"));
        setCentralWidget(l);
    }
    
    oldTab = tab;
//     removeDockWidget(workflow);
//     addDockWidget(Qt::LeftDockWidgetArea, workflow);
//     workflow->hide();
//     workflow->show();
}

void MainWindow::loadProject(const QString &name)
{
    kDebug() << "Loading project named" << name << "...";

    // Add it to the recent files first.
    
    QStringList recentFiles;
    KConfig c;
    KConfigGroup cg = c.group("General");
    recentFiles = recentProjects();
    
    if (recentFiles.contains(name)) {
        recentFiles.removeAt(recentFiles.indexOf(name));
    }
    
    if (!name.isEmpty()) {
        recentFiles.prepend(name);
    }
    
    kDebug() << "Writing the following sidebar of recent files to the config:" << recentFiles;
    
    cg.writeEntry("recentFiles", recentFiles);
    
    c.sync();
    
    // Load the needed widgets, switch to page 1 (edit)...
    createDockWidgets();
    sidebar->setCurrentIndex(1);
}

QStringList MainWindow::recentProjects() // TODO Limit to 5 
{
    KConfig c;
    KConfigGroup cg = c.group("General");
    QStringList l = cg.readEntry("recentFiles", QStringList());
//     kDebug() << l.toStringList();
    
    return l;
}

