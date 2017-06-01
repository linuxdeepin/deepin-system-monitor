#ifndef PROCESSMANAGER_H
#define PROCESSMANAGER_H

#include "ddialog.h"
#include "process_view.h"
#include <QMap>
#include <QLabel>
#include <QPixmap>
#include <QString>
#include <QWidget>
#include <proc/readproc.h>
#include <QMenu>
#include "process_switch_tab.h"

DWIDGET_USE_NAMESPACE

class ProcessManager : public QWidget
{
    Q_OBJECT
    
public:
    ProcessManager(QWidget *parent = 0);
    ~ProcessManager();
                     
signals:
    void activeTab(int index);
    
public slots:
    void updateStatus(QList<ListItem*> items);
    void popupMenu(QPoint pos, QList<ListItem*> items);
    
    void killProcesses();
    void stopProcesses();
    void resumeProcesses();
    void openProcessDirectory();
    
    void handleSearch(QString searchContent);
    
    void showKillProcessDialog();
    
    void dialogButtonClicked(int index, QString buttonText);
    
    void updateProcessNumber(int guiProcessNumber, int systemProcessNumber);
    
private:
    ProcessView *processView;
    QMenu *rightMenu;
    QAction *killAction;
    QAction *pauseAction;
    QAction *resumeAction;
    QAction *openDirectoryAction;
    
    QList<int> *actionPids;
    
    QLabel *statusLabel;
    
    DDialog *killProcessDialog;
    
    ProcessSwitchTab *processSwitchTab;
};

#endif
