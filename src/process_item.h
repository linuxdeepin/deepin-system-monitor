#ifndef PROCESSITEM_H
#define PROCESSITEM_H

#include "list_item.h"
#include <proc/readproc.h>

class ProcessItem : public ListItem
{
    Q_OBJECT
    
public:
    ProcessItem(proc_t *p);
    
    void render(QRect rect, QPainter *painter, int column, bool isSelect);
    void renderBackground(QRect rect, QPainter *painter, int index, bool isSelect);
    
    int pid;
    QString name;
    int cpu;
    int memory;
    
private:
    proc_t *process;
    QString path;
    QString user;
    int uploadBandwidth;
    int downloadBandwidth;
    QPixmap iconPixmap;
    int iconSize;
    int padding;
    QString memoryString;
};

#endif
