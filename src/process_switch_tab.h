#ifndef PROCESSSWITCHTAB_H
#define PROCESSSWITCHTAB_H

#include <QMouseEvent>
#include <QWidget>

class ProcessSwitchTab : public QWidget 
{
    Q_OBJECT
    
public:
    ProcessSwitchTab(QWidget *parent = 0);
    
    void mouseMoveEvent(QMouseEvent *mouseEvent);
    void mousePressEvent(QMouseEvent *mouseEvent);
    void paintEvent(QPaintEvent *);
    
signals:
    void activeTab(int index);
    
protected:
    virtual void leaveEvent(QEvent * event);
    
private:
    QImage allProcessActiveImage;
    QImage allProcessHoverImage;
    QImage allProcessNormalImage;
    QImage onlyGuiActiveImage;
    QImage onlyGuiHoverImage;
    QImage onlyGuiNormalImage;
    QImage onlyMeActiveImage;
    QImage onlyMeHoverImage;
    QImage onlyMeNormalImage;
    int activeIndex = 0;
    int height = 24;
    int hoverIndex = -1;
    int width = 26;
};

#endif 
