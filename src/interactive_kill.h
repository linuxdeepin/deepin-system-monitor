#ifndef INTERACTIVEKILL_H
#define INTERACTIVEKILL_H

#include "start_tooltip.h"
#include "window_manager.h"
#include <QKeyEvent>
#include <QPaintEvent>
#include <QWidget>

class InteractiveKill : public QWidget
{
    Q_OBJECT
 
public:
    InteractiveKill(QWidget *parent = 0);
    ~InteractiveKill();
    
    void keyPressEvent(QKeyEvent *keyEvent);
    void mouseMoveEvent(QMouseEvent *mouseEvent);
    void mousePressEvent(QMouseEvent *mouseEvent);
    void paintEvent(QPaintEvent *);
    
signals:
    void killWindow(int pid);
    
private:
    QImage cursorImage;
    QList<WindowRect> windowRects;
    QList<int> windowPids;
    QPixmap screenPixmap;
    StartTooltip* startTooltip;
    WindowManager* windowManager;
    WindowRect killWindowRect;
    int cursorX;
    int cursorY;
};

#endif
