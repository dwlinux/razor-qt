/* BEGIN_COMMON_COPYRIGHT_HEADER
 *
 * Razor - a lightweight, Qt based, desktop toolset
 * https://sourceforge.net/projects/razor-qt/
 *
 * Copyright: 2010-2011 Razor team
 * Authors:
 *   Petr Vanek <petr@scribus.info>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation;  only version 2 of
 * the License is valid for this program.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * END_COMMON_COPYRIGHT_HEADER */

#ifndef APPSWITCHER_H
#define APPSWITCHER_H

#include "ui_appswitcher.h"
#include <QWidget>
#include <QtGui/QToolButton>
#include <QtCore/QString>
#include <QtGui/QPixmap>
#include <QtCore/QList>
#include <X11/X.h>

class QxtGlobalShortcut;
class QShortcut;
class QHBoxLayout;

namespace RazorAppSwitcher {

/*! \brief One item (application) in the AppSwitcher dialog area.
It's only a lightweight wrapper around QToolButton holding additional
information about its X Window.
Signal infoChanged() is raised when the widget gets focus to inform
parent window's info label to change its content.
*/
class SwitcherItem : public QToolButton
{
    Q_OBJECT

public:
    SwitcherItem(Window window, const QString & text, const QPixmap & pixmap, QWidget * parent);

    Window window() { return m_window; }

signals:
    //! Inform parent that info label has to be changed with text
    void infoChanged(const QString & text);
    //! Sent by mouseReleaseEvent - to change X11 window by mouse
    void activateXWindow();

private:
    Window m_window;
    //! Just change parent's info label with item's title
    void focusInEvent(QFocusEvent * e);
    //! Allow to change focus/active button with mouse move over item
    void mouseMoveEvent(QMouseEvent * e);
    //! Allow to change apps by mouse too
    void mouseReleaseEvent(QMouseEvent * e);
};

/*! \brief Main switcher window. It's a borderless window (no title etc).
*/
class AppSwitcher : public QWidget, public Ui::AppSwitcher
{
    Q_OBJECT

public:
    AppSwitcher();

    /*! \brief Catch X11 events to track changes in window orders.
    Windows last usages is recorded in m_orderedWindows list. Then
    it is merged with all windows to create logical order of applications
    in widget (last used first... later used next) in handleApps()
    */
    bool handleEvent(XEvent * e);

private:
    //! 3rd party class to handle global keyboard shortcut
    QxtGlobalShortcut * m_key;
    /*! Ambiguous shortcut, similar to m_key to handle switches when is
        user in the dialog - m_key is blocked in this case */
    QShortcut * m_localKey;

    //! Keep history of used windows
    QList<Window> m_orderedWindows;

    //! List to keep items mapping
    QList<SwitcherItem*> m_list;

    //! QScrollarea QWidget's layout
    QHBoxLayout * m_layout;

    //! Enable m_key, the global shortcut.
    void hideEvent(QHideEvent *e);
    //! Disable m_key, the global shortcut.
    void showEvent(QShowEvent *e);

    //! Handle Escape (close()) or Enter (activate window) keys
    void keyPressEvent(QKeyEvent * e);
    //! Handle window closing when user releases ctrl or alt
    void keyReleaseEvent(QKeyEvent * e);

    /*! Filter events to close the window when it gets QEvent::WindowDeactivate.
        It makes sense to close window when user activates another app.
    */
    bool eventFilter(QObject * o, QEvent * e);


private slots:
    //! Setup m_layout and m_list - called by m_key
    void handleApps();
    //! Just change focus in items layout - called by m_localKey or keyPressEvent()
    void selectNextItem();
    //! Activate X11 window and close appswitcher
    void activateXWindow();
};

}; // namespace


#endif
