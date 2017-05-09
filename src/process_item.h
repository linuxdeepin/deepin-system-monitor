#ifndef PROCESSITEM_H
#define PROCESSITEM_H

#include "list_item.h"
#include <proc/readproc.h>
#include <QIcon>

class ProcessItem : public ListItem
{
    Q_OBJECT
    
public:
    ProcessItem(proc_t *p);
    
    void render(int column, bool isSelectItem, QRect rect, QPainter *painter);
    void renderBackground(int index, QRect rect, QPainter *painter);
    void renderSelection(QRect rect, QPainter *painter);
    
private:
    proc_t *process;
    QString path;
    QString name;
    QString user;
    int cpu;
    QString memory;
    int uploadBandwidth;
    int downloadBandwidth;
    int pid;
    QIcon icon;
    QPixmap iconPixmap;
    int iconSize;
    int padding;
};

#endif
