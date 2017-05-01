#ifndef PROCESSMANAGER_H
#define PROCESSMANAGER_H

#include <QWidget>
#include "process_view.h"

class ProcessManager : public QWidget
{
    Q_OBJECT
    
public:
    ProcessManager(QWidget *parent = 0);
    
private:
    ProcessView *processView;
};

#endif
