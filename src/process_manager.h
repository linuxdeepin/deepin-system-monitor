#ifndef PROCESSMANAGER_H
#define PROCESSMANAGER_H

#include "process_view.h"
#include <QMap>
#include <QPixmap>
#include <QString>
#include <QWidget>
#include <proc/readproc.h>

class ProcessManager : public QWidget
{
    Q_OBJECT
    
    typedef std::map<int, proc_t> storedProcType;
    
public:
    ProcessManager(QWidget *parent = 0);
                                       
public slots:
    void updateProcesses();
    
private:
    static bool sortByName(const ListItem *item1, const ListItem *item2, bool descendingSort);
    static bool sortByCPU(const ListItem *item1, const ListItem *item2, bool descendingSort);
    static bool sortByMemory(const ListItem *item1, const ListItem *item2, bool descendingSort);
    static bool sortByPid(const ListItem *item1, const ListItem *item2, bool descendingSort);
    
    ProcessView *processView;
    QMap<QString, QPixmap> *processIconCache;
    QTimer *updateTimer;
    storedProcType prevProcesses;
    unsigned long long totalCpuTime;
};

#endif
