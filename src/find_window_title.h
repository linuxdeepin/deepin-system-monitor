#ifndef FINDWINDOWTITLE_H
#define FINDWINDOWTITLE_H

#include <QObject>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include "window_manager.h"

class FindWindowTitle : public WindowManager
{
    Q_OBJECT
    
public:
    FindWindowTitle();
    
    void updateWindowInfos();
    QString findWindowTitle(int pid);
    
private:
    QMap<int, QString> *windowTitles;
};

#endif
