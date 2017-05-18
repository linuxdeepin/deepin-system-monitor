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
    
public:
    ProcessManager(QWidget *parent = 0);
    ~ProcessManager();
    
public slots:
    void updateStatus(QList<ListItem*> items);
                                       
private:
    ProcessView *processView;
};

#endif
