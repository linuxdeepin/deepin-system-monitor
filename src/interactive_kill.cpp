#include "interactive_kill.h"
#include "utils.h"

#include <QDebug>
#include <QApplication>

InteractiveKill::InteractiveKill(QWidget *parent) : QWidget(parent)
{
    cursorImage = QImage(Utils::getQrcPath("kill_cursor.png"));
    cursorX = -1;
    cursorY = -1;
    
    // Add Qt::WindowDoesNotAcceptFocus make window not accept focus forcely, avoid conflict with dde hot-corner.
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_DeleteOnClose, true);
    setMouseTracking(true);     // make MouseMove can response
    installEventFilter(this);   // add event filter

    windowManager = new WindowManager();
    QList<xcb_window_t> windows = windowManager->getWindows();

    for (int i = 0; i < windows.length(); i++) {
        windowRects.append(windowManager->adjustRectInScreenArea(windowManager->getWindowRect(windows[i])));
        windowPids.append(windowManager->getWindowPid(windows[i]));
    }
    
    startTooltip = new StartTooltip();
    startTooltip->setWindowManager(windowManager);
    startTooltip->show();
    
    showFullScreen();
}

InteractiveKill::~InteractiveKill()
{
    delete startTooltip;
}

void InteractiveKill::mousePressEvent(QMouseEvent *mouseEvent)
{
    QList<xcb_window_t> windows = windowManager->getWindows();

    for (int i = 0; i < windowRects.length(); i++) {
        WindowRect rect = windowRects[i];
        
        if (mouseEvent->x() >= rect.x && mouseEvent->x() <= rect.x + rect.width &&
            mouseEvent->y() >= rect.y && mouseEvent->y() <= rect.y + rect.height) {
            killWindow(windowPids[i]);
            
            break;
        }
    }
    
    close();
}

void InteractiveKill::mouseMoveEvent(QMouseEvent *mouseEvent)
{
    if (startTooltip != NULL) {
        delete startTooltip;
        startTooltip = NULL;
        
        QApplication::setOverrideCursor(Qt::BlankCursor);
    }
    
    cursorX = mouseEvent->x();
    cursorY = mouseEvent->y();
    
    repaint();
}

void InteractiveKill::paintEvent(QPaintEvent *)
{
    if (cursorX >=0 && cursorY >= 0) {
        QPainter painter(this);
        painter.drawImage(QPoint(cursorX, cursorY), cursorImage);
    }
}

