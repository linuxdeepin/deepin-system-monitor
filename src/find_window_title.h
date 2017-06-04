#ifndef FINDWINDOWTITLE_H
#define FINDWINDOWTITLE_H

#include "window_manager.h"
#include <QObject>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>

class FindWindowTitle : public WindowManager
{
    Q_OBJECT
    
public:
    FindWindowTitle();
    
    QString findWindowTitle(int pid);
    void updateWindowInfos();
    
private:
    QMap<int, QString> *windowTitles;
};

#endif
