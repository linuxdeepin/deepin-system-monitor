#ifndef FINDWINDOWTITLE_H
#define FINDWINDOWTITLE_H

#include <QObject>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>

class FindWindowTitle : public QObject
{
    Q_OBJECT
    
public:
    FindWindowTitle();
    ~FindWindowTitle();
    
    void updateWindowInfos();
    QString findWindowTitle(int pid);
    
    QString getWindowName(xcb_window_t window);
    xcb_get_property_reply_t* getProperty(xcb_window_t window, QString propertyName, xcb_atom_t type);
    QStringList getWindowTypes(xcb_window_t window);
    xcb_atom_t getAtom(QString name);
    QString getAtomName(xcb_atom_t atom);
    int getWindowPid(xcb_window_t window);
    
private:
    xcb_window_t rootWindow;
    xcb_connection_t* conn;
    
    QMap<int, QString> *windowTitles;
};

#endif
