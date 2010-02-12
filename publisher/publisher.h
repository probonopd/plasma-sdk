/*
  Copyright (c) 2009 Lim Yuen Hoe <yuenhoe@hotmail.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#ifndef PUBLISHER_H
#define PUBLISHER_H

#include <QWidget>
#include <KUrl>

class KUrlRequester;
class QPushButton;

namespace Ui
{
class Publisher;
}

class Publisher : public QWidget
{
    Q_OBJECT;
public:
    Publisher(QWidget* parent, const KUrl &path);

    static bool exportPackage(const KUrl &toExport, const KUrl &targetFile);
    static bool importPackage(const KUrl &toImport, const KUrl &targetLocation);

private slots:
    void doExport();
    void addSuffix();
    void doInstall();
    void doPublish();

private:
    KUrlRequester* m_exporterUrl;
    QPushButton* m_exporterButton;
    QPushButton* m_installerButton;
    QPushButton* m_publisherButton;
    KUrl m_projectPath;
};

#endif // PUBLISHER_H