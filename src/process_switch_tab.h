#ifndef PROCESSSWITCHTAB_H
#define PROCESSSWITCHTAB_H

#include <QWidget>
#include <QMouseEvent>

class ProcessSwitchTab : public QWidget 
{
    Q_OBJECT
    
public:
    ProcessSwitchTab(QWidget *parent = 0);
    
    void paintEvent(QPaintEvent *);
    void mouseMoveEvent(QMouseEvent *mouseEvent);
    void mousePressEvent(QMouseEvent *mouseEvent);
    
signals:
    void activeTab(int index);
    
protected:
    virtual void leaveEvent(QEvent * event);
    
private:
    QImage onlyGuiNormalImage;
    QImage onlyGuiHoverImage;
    QImage onlyGuiActiveImage;
    QImage onlyMeNormalImage;
    QImage onlyMeHoverImage;
    QImage onlyMeActiveImage;
    QImage allProcessNormalImage;
    QImage allProcessHoverImage;
    QImage allProcessActiveImage;
    
    int width = 26;
    int height = 24;
    
    int activeIndex = 0;
    int hoverIndex = -1;
};

#endif 
