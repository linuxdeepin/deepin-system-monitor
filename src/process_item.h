#ifndef PROCESSITEM_H
#define PROCESSITEM_H

#include "list_item.h"
#include <proc/readproc.h>

class ProcessItem : public ListItem
{
    Q_OBJECT
    
public:
    ProcessItem(proc_t *p, QMap<QString, QPixmap> *processIconCache);
    
    bool sameAs(ListItem *item);
    void drawBackground(QRect rect, QPainter *painter, int index, bool isSelect);
    void drawForeground(QRect rect, QPainter *painter, int column, bool isSelect);
    
    QString getName() const;
    int getCPU() const;
    int getMemory() const;
    int getPid() const;
    
private:
    proc_t *process;
    
    int cpu;
    int iconSize;
    int memory;
    int padding;
    int pid;
    
    QPixmap iconPixmap;
    
    QString memoryString;
    QString name;
    QString path;
    QString user;
};

#endif
