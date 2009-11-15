/*
  Copyright (c) 2009 Lim Yuen Hoe <yuenhoe@hotmail.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>

#include <KUrlRequester>
#include <KLocalizedString>
#include <KProcess>
#include <KMessageBox>

#include "publisher.h"

Publisher::Publisher(QWidget *parent, const KUrl &path)
    :QWidget(parent),
    m_projectPath(path)
{
    // something simple and plasmoid-specific for now
    // These should probably be refined at some point
    QString exportLabel = i18n("Export plasmoid");
    QString exportText = i18n("Choose a target file to export the current plasmoid ") +
	i18n("to an installable package file on your system.");
    QString installLabel = i18n("Install plasmoid");
    QString installText = i18n("Click to install the current plasmoid ") +
	i18n("directly onto your computer.");
    QString publishLabel = i18n("Publish plasmoid");
    QString publishText = i18n("Click to publish the current plasmoid ") +
	i18n("online, so that other people can find and install it over ") +
	i18n("the internet.");

    m_exporterUrl = new KUrlRequester(this);
    m_exporterUrl->setFilter("*.plasmoid");
    m_exporterUrl->setMode(KFile::File | KFile::LocalOnly);

    m_exporterButton = new QPushButton(i18n("Export current plasmoid"), this);
    m_installerButton = new QPushButton(i18n("Install current plasmoid"), this);
    m_publisherButton = new QPushButton(i18n("Publish current plasmoid"), this);

    connect(m_exporterButton, SIGNAL(clicked()), this, SLOT(doExport()));
    connect(m_installerButton, SIGNAL(clicked()), this, SLOT(doInstall()));
    connect(m_publisherButton, SIGNAL(clicked()), this, SLOT(doPublish()));

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(new QLabel("<h1>" + exportLabel + "</h1>", this));

    QFrame *separator = new QFrame();
    separator->setFrameStyle(QFrame::HLine | QFrame::Sunken);

    layout->addWidget(separator);
    layout->addWidget(new QLabel(exportText, this));
    layout->addWidget(m_exporterUrl);
    layout->addWidget(m_exporterButton);

    layout->addWidget(new QLabel("<h1>" + installLabel + "</h1>", this));

    separator = new QFrame();
    separator->setFrameStyle(QFrame::HLine | QFrame::Sunken);

    layout->addWidget(separator);
    layout->addWidget(new QLabel(installText, this));
    layout->addWidget(m_installerButton);

    layout->addWidget(new QLabel("<h1>" + publishLabel + "</h1>", this));

    separator = new QFrame();
    separator->setFrameStyle(QFrame::HLine | QFrame::Sunken);

    layout->addWidget(separator);
    layout->addWidget(new QLabel(publishText, this));
    layout->addWidget(m_publisherButton);

    QWidget *spaceSoaker = new QWidget(this);
    layout->addWidget(spaceSoaker);
    layout->setStretchFactor(spaceSoaker, 100);

    setLayout(layout);
}

void Publisher::doExport()
{
    if (!m_exporterUrl->url().isLocalFile() ||
          QDir(m_exporterUrl->url().path()).exists()) {
        KMessageBox::error(this, i18n("The file you entered is invalid!"));
        return;
    }
    exportPackage(m_projectPath, m_exporterUrl->url());
    // TODO: probably check for errors and stuff instead of announcing
    // succcess in a 'leap of faith'
    KMessageBox::information(this, i18n("Plasmoid has been exported to ") + m_exporterUrl->url().path());
}

void Publisher::doInstall()
{
    KUrl tempPackage(m_projectPath.path(KUrl::AddTrailingSlash) + "package.plasmoid");
    exportPackage(m_projectPath, tempPackage); // create temporary package

    // we do a plasmapkg -u in case the package was installed before
    // in which case it should be updated. -u also installs the
    // package if it hasn't been installed before, so it's all good.
    QStringList argv("plasmapkg");
    argv.append("-u");
    argv.append(tempPackage.path());
    KProcess::execute(argv);
    QFile::remove(tempPackage.path()); // delete temporary package
    // TODO: probably check for errors and stuff instead of announcing
    // succcess in a 'leap of faith'
    KMessageBox::information(this, i18n("Plasmoid has been installed"));
}

void Publisher::doPublish()
{
    KMessageBox::information(this, "Do funky stuff here");
}

void Publisher::exportPackage(const KUrl &toExport, const KUrl &targetFile)
{
    QStringList argv("zip");
    argv.append("-r");
    argv.append(targetFile.path());
    argv.append(".");
    KProcess p;
    p.setProgram(argv);
    p.setWorkingDirectory(toExport.path());
    p.execute();
}

void Publisher::importPackage(const KUrl &toImport, const KUrl &targetLocation)
{
    // TODO: implement this, then use this to import packages in the
    // start page
}