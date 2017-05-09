#include "list_view.h"
#include "utils.h"
#include <QDebug>
#include <QEvent>
#include <QWheelEvent>
#include <QtMath>

ListView::ListView(int height, QWidget *parent) : QWidget(parent)
{
    installEventFilter(this);  // add event filter
    setMouseTracking(true);   // make MouseMove can response

    rowHeight = height;
    scrollUnit = rowHeight * 9;
    scrollDistance = 0;
    renderOffset = 0;
    titleHeight = 0;

    renderTimer = NULL;
    renderTicker = 0;
    animationFrames = 16;
    animationDuration = 25;

    listItems = new QList<ListItem*>();
    selectionItems = new QList<ListItem*>();
}

void ListView::addItems(QList<ListItem*> items) {
    QPainter painter(this);

    listItems->append(items);

    repaint();
}

void ListView::clearItems() {
    listItems->clear();

    repaint();
}

void ListView::addSelections(QList<ListItem*> items) {
    selectionItems->append(items);
}

void ListView::clearSelections() {
    selectionItems->clear();
}

void ListView::setTitles(QList<QString> titles, int height) {
    titleNames = titles;
    titleHeight = height;
}

void ListView::setSortAlgorithm() {

}

void ListView::setColumnWidths(QList<int> widths) {
    columnWidths = widths;
}

bool ListView::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

        handleKeyPressEvent(keyEvent);
    }

    return false;
}

void ListView::handleKeyPressEvent(QKeyEvent *keyEvent) {
    if (keyEvent->key() == Qt::Key_Home) {
        selectFirst();
    } else if (keyEvent->key() == Qt::Key_End) {
        selectLast();
    }
}

void ListView::selectFirst() {
    clearSelections();
    
    QList<ListItem*> items = QList<ListItem*>();
    items << listItems->first();
    addSelections(items);
    
    renderOffset = 0;

    repaint();
}

void ListView::selectLast() {
    clearSelections();
    
    QList<ListItem*> items = QList<ListItem*>();
    items << listItems->last();
    addSelections(items);

    renderOffset = listItems->count() * rowHeight - rect().height() + titleHeight;

    repaint();
}

int ListView::adjustRenderOffset(int offset) {
    return std::max(0, std::min(offset, listItems->count() * rowHeight - rect().height() + titleHeight));
}

void ListView::wheelEvent(QWheelEvent *event)
{
    if (event->orientation() == Qt::Vertical) {
        int scrollStep = event->angleDelta().y() / 120;

        int newRenderOffset = renderOffset - scrollStep * scrollUnit;
        newRenderOffset = adjustRenderOffset(newRenderOffset);

        if (newRenderOffset != renderOffset) {
            startScroll(newRenderOffset - renderOffset);
        }
    }

    event->accept();
}

void ListView::startScroll(int scrollOffset) {
    if (renderTimer == NULL || !renderTimer->isActive()) {
        scrollStartY = renderOffset;
        scrollDistance = scrollOffset;

        renderTicker = 0;
        renderTimer = new QTimer();
        connect(renderTimer, SIGNAL(timeout()), this, SLOT(renderAnimation()));
        renderTimer->start(animationDuration);
    }
}

void ListView::renderAnimation() {
    if (renderTicker <= animationFrames) {

        renderOffset = scrollStartY + Utils::easeInOut(renderTicker / (animationFrames * 1.0)) * scrollDistance;
        renderOffset = adjustRenderOffset(renderOffset);

        repaint();

        renderTicker++;
    } else {
        renderTimer->stop();
    }
}

void ListView::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Calcuate title widths;
    QList<int> renderWidths = calcuateRenderWidths();

    // Draw title.
    int renderY = 0;
    int renderHeight = 0;
    if (titleHeight > 0) {
        int columnCounter = 0;
        int columnRenderX = 0;
        for (int renderWidth:renderWidths) {
            painter.setOpacity(1);
            Utils::setFontSize(painter, 11);
            painter.setPen(QPen(QColor("#666666")));
            painter.drawText(QRect(columnRenderX, 0, renderWidth, titleHeight), Qt::AlignCenter, titleNames[columnCounter]);

            columnRenderX += renderWidth;
            columnCounter++;
        }

        renderY += titleHeight;
        renderHeight += titleHeight;
    }

    // Draw context.
    painter.setClipRect(QRectF(rect().x(), rect().y() + rowHeight, rect().width(), rect().height() - rowHeight));

    int rowCounter = 0;
    for (ListItem *item:*listItems) {
        if (rowCounter > ((renderOffset - rowHeight) / rowHeight)) {
            item->renderBackground(rowCounter, QRect(0, renderY + rowCounter * rowHeight - renderOffset, rect().width(), rowHeight), &painter);

            if (selectionItems->contains(item)) {
                item->renderSelection(QRect(0, renderY + rowCounter * rowHeight - renderOffset, rect().width(), rowHeight), &painter);
            }

            int columnCounter = 0;
            int columnRenderX = 0;
            for (int renderWidth:renderWidths) {
                item->render(columnCounter, QRect(columnRenderX, renderY + rowCounter * rowHeight - renderOffset, renderWidth, rowHeight), &painter);

                columnRenderX += renderWidth;
                columnCounter++;
            }

            renderHeight += rowHeight;

            if (renderHeight > rect().height()) {
                break;
            }
        }

        rowCounter++;
    }
}

QList<int> ListView::calcuateRenderWidths() {
    QList<int> renderWidths;
    if (columnWidths.contains(-1)) {
        for (int columnWidth:columnWidths) {
            if (columnWidth != -1) {
                renderWidths << columnWidth;
            } else {
                int totalWidthOfOtherColumns = 0;
                for (auto w:columnWidths) {
                    if (w != -1) {
                        totalWidthOfOtherColumns += w;
                    }
                }

                renderWidths << rect().width() - totalWidthOfOtherColumns;
            }
        }
    } else {
        renderWidths = columnWidths;
    }

    return renderWidths;
}
