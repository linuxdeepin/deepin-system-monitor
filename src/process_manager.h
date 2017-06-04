#ifndef PROCESSMANAGER_H
#define PROCESSMANAGER_H

#include "ddialog.h"
#include "process_switch_tab.h"
#include "process_view.h"
#include <QLabel>
#include <QMap>
#include <QMenu>
#include <QPixmap>
#include <QString>
#include <QWidget>
#include <proc/readproc.h>

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
    void dialogButtonClicked(int index, QString buttonText);
    void handleSearch(QString searchContent);
    void killProcesses();
    void openProcessDirectory();
    void popupMenu(QPoint pos, QList<ListItem*> items);
    void resumeProcesses();
    void showAttributes();
    void showKillProcessDialog();
    void stopProcesses();
    void updateProcessNumber(int guiProcessNumber, int systemProcessNumber);
    void updateStatus(QList<ListItem*> items);
    
private:
    DDialog *killProcessDialog;
    ProcessSwitchTab *processSwitchTab;
    ProcessView *processView;
    QAction *attributesAction;
    QAction *killAction;
    QAction *openDirectoryAction;
    QAction *pauseAction;
    QAction *resumeAction;
    QLabel *statusLabel;
    QList<int> *actionPids;
    QMenu *rightMenu;
};

#endif
