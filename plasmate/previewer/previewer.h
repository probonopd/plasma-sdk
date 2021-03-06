/*
Copyright 2010 Lim Yuen Hoe <yuenhoe@hotmail.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of
the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef PREVIEWER_H
#define PREVIEWER_H

#include <QDockWidget>

// This is the base previewer dockwidget class
// All previewer types should inherit from this class
class Previewer : public QDockWidget {

    Q_OBJECT

public:
    Previewer(const QString & title, QWidget * parent = 0, Qt::WindowFlags flags = 0 );
    Previewer(QWidget * parent = 0, Qt::WindowFlags flags = 0 );

    virtual ~Previewer();

    /**
     * Subclasses should override this method with actual previewing code
     */
    virtual void showPreview(const QString &packagePath) = 0;

    /**
     * Subclasses should override this method with code that refreshes the preview
     */
    virtual void refreshPreview() = 0;
Q_SIGNALS:
    void toggleKonsole();
};

#endif // PREVIEWER_H
