#ifndef PROCESSMANAGER_H
#define PROCESSMANAGER_H

#include "process_view.h"
#include <QMap>
#include <QLabel>
#include <QPixmap>
#include <QString>
#include <QWidget>
#include <proc/readproc.h>
#include <QMenu>
#include "dimagebutton.h"

DWIDGET_USE_NAMESPACE

class ProcessManager : public QWidget
{
    Q_OBJECT
    
public:
    ProcessManager(QWidget *parent = 0);
    ~ProcessManager();
    
    DImageButton *onlyGuiButton;
    DImageButton *onlyMeButton;
    DImageButton *allProcessButton;
                                  
public slots:
    void updateStatus(QList<ListItem*> items);
    void popupMenu(QPoint pos, QList<ListItem*> items);
    
    void killProcesses();
    void stopProcesses();
    void resumeProcesses();
    
    void handleSearch(QString searchContent);
    
private:
    ProcessView *processView;
    QMenu *rightMenu;
    QAction *killAction;
    QAction *pauseAction;
    QAction *resumeAction;
    
    QList<ListItem*> *actionItems;
    
    QLabel *statusLabel;
};

#endif
