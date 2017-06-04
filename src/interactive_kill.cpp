#include "interactive_kill.h"
#include "utils.h"

#include <QDebug>
#include <QScreen>
#include <QApplication>

InteractiveKill::InteractiveKill(QWidget *parent) : QWidget(parent)
{
    cursorImage = QImage(Utils::getQrcPath("kill_cursor.png"));
    cursorX = -1;
    cursorY = -1;

    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
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
    
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        screenPixmap = screen->grabWindow(0);
    }

    showFullScreen();
}

InteractiveKill::~InteractiveKill()
{
    delete startTooltip;
}

void InteractiveKill::mousePressEvent(QMouseEvent *mouseEvent)
{
    if (startTooltip != NULL) {
        delete startTooltip;
        startTooltip = NULL;
    }

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
    QApplication::setOverrideCursor(Qt::BlankCursor);

    cursorX = mouseEvent->x();
    cursorY = mouseEvent->y();
    
    for (int i = 0; i < windowRects.length(); i++) {
        WindowRect rect = windowRects[i];

        if (mouseEvent->x() >= rect.x && mouseEvent->x() <= rect.x + rect.width &&
            mouseEvent->y() >= rect.y && mouseEvent->y() <= rect.y + rect.height) {
            killWindowRect = rect;

            break;
        }
    }

    repaint();
}

void InteractiveKill::keyPressEvent(QKeyEvent *keyEvent)
{
    if (keyEvent->key() == Qt::Key_Escape) {
        close();
    }
}

void InteractiveKill::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.drawPixmap(QPoint(0, 0), screenPixmap);

    if (cursorX >=0 && cursorY >= 0) {
        QPainterPath path;
        path.addRect(QRectF(killWindowRect.x, killWindowRect.y, killWindowRect.width, killWindowRect.height));
        painter.setOpacity(0.5);
        painter.fillPath(path, QColor("#ff0000"));
        
        painter.setOpacity(1);
        painter.drawImage(QPoint(cursorX, cursorY), cursorImage);
    }
}
