#ifndef PROCESSITEM_H
#define PROCESSITEM_H

#include "list_item.h"

class ProcessItem : public ListItem
{
    Q_OBJECT
    
public:
    ProcessItem(QString procPath);
    
    void render(int column, QRect rect, QPainter *painter);
    void renderSelection(QRect rect, QPainter *painter);
    
private:
    QString processIconPath;
    QString processName;
    int processCPU;
    int processMemory;
    int processUploadBandwidth;
    int processDownloadBandwidth;
    int processPid;
};

#endif
