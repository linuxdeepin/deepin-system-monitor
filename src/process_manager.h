#ifndef PROCESSMANAGER_H
#define PROCESSMANAGER_H

#include "process_view.h"
#include <QMap>
#include <QPixmap>
#include <QString>
#include <QWidget>
#include <proc/readproc.h>
#include <QMenu>

class ProcessManager : public QWidget
{
    Q_OBJECT
    
public:
    ProcessManager(QWidget *parent = 0);
    ~ProcessManager();
    
public slots:
    void updateStatus(QList<ListItem*> items);
    void popupMenu(QPoint pos, QList<ListItem*> items);
    
    void killProcesses();
    void stopProcesses();
    void resumeProcesses();
                                       
private:
    ProcessView *processView;
    QMenu *rightMenu;
    QAction *killAction;
    QAction *pauseAction;
    QAction *resumeAction;
    
    QList<ListItem*> *actionItems;
};

#endif
