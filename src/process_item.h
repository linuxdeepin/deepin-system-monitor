#ifndef PROCESSITEM_H
#define PROCESSITEM_H

#include "list_item.h"
#include <proc/readproc.h>

class ProcessItem : public ListItem
{
    Q_OBJECT
    
public:
    ProcessItem(proc_t *p);
    
    void render(int column, QRect rect, QPainter &painter);
    void renderSelection(QRect rect, QPainter &painter);
    
private:
    proc_t *process;
    QString path;
    QString name;
    QString user;
    int cpu;
    int memory;
    int uploadBandwidth;
    int downloadBandwidth;
    int pid;
};

#endif
