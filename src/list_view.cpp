#include "list_view.h"
#include "utils.h"
#include <QDebug>
#include <QApplication>
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
    lastSelectItem = NULL;

    mouseAtScrollArea = false;
    scrollbarDefaultWidth = 3;
    scrollbarDragWidth = 8;
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

void ListView::addSelections(QList<ListItem*> items, bool recordLastSelection) {
    selectionItems->append(items);

    if (recordLastSelection) {
        lastSelectItem = selectionItems->last();
    }
}

void ListView::clearSelections(bool clearLastSelection) {
    selectionItems->clear();

    if (clearLastSelection) {
        lastSelectItem = NULL;
    }
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
    } else if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

        handleButtonPressEvent(mouseEvent);
    } else if (event->type() == QEvent::MouseMove) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

        bool atScrollArea = isMouseAtScrollArea(mouseEvent->x());

        if (atScrollArea != mouseAtScrollArea) {
            mouseAtScrollArea = atScrollArea;
            repaint();
        }
    }

    return false;
}

bool ListView::isMouseAtScrollArea(int x) {
    return (x > rect().x() + rect().width() - scrollbarDragWidth) && (x < rect().x() + rect().width());
}

void ListView::handleKeyPressEvent(QKeyEvent *keyEvent) {
    if (keyEvent->key() == Qt::Key_Home) {
        if (keyEvent->modifiers() == Qt::ControlModifier) {
            scrollHome();
        } else if (keyEvent->modifiers() == Qt::ShiftModifier) {
            shiftSelectHome();
        }else {
            pressHome();
        }
    } else if (keyEvent->key() == Qt::Key_End) {
        if (keyEvent->modifiers() == Qt::ControlModifier) {
            scrollEnd();
        } else if (keyEvent->modifiers() == Qt::ShiftModifier) {
            shiftSelectEnd();
        } else {
            pressEnd();
        }
    } else if (keyEvent->key() == Qt::Key_Up) {
        if (keyEvent->modifiers() == Qt::ShiftModifier) {
            shiftSelectUp();
        } else {
            pressUp();
        }
    } else if (keyEvent->key() == Qt::Key_Down) {
        if (keyEvent->modifiers() == Qt::ShiftModifier) {
            shiftSelectDown();
        } else {
            pressDown();
        }
    } else if (keyEvent->key() == Qt::Key_PageUp) {
        if (keyEvent->modifiers() == Qt::ControlModifier) {
            scrollPageUp();
        } else if (keyEvent->modifiers() == Qt::ShiftModifier) {
            shiftSelectPageUp();
        } else {
            pressPageUp();
        }
    } else if (keyEvent->key() == Qt::Key_PageDown) {
        if (keyEvent->modifiers() == Qt::ControlModifier) {
            scrollPageDown();
        } else if (keyEvent->modifiers() == Qt::ShiftModifier) {
            shiftSelectPageDown();
        } else {
            pressPageDown();
        }
    } else if (keyEvent->key() == Qt::Key_A) {
        if (keyEvent->modifiers() == Qt::ControlModifier) {
            selectAll();
        }
    }
}

void ListView::selectAll() {
    clearSelections();

    addSelections(*listItems);

    renderOffset = 0;

    repaint();
}

void ListView::handleButtonPressEvent(QMouseEvent *mouseEvent) {
    bool atScrollArea = isMouseAtScrollArea(mouseEvent->x());

    if (atScrollArea) {
        int barHeight = getScrollbarHeight();
        
        renderOffset = adjustRenderOffset((mouseEvent->y() - barHeight / 2 - titleHeight) / (rect().height() - titleHeight * 1.0) * listItems->count() * rowHeight);
        repaint();
    } else {
        int pressItemIndex = (renderOffset + mouseEvent->y() - titleHeight) / rowHeight;

        if (pressItemIndex < listItems->count()) {
            if (mouseEvent->modifiers() == Qt::ControlModifier) {
                ListItem *item = (*listItems)[pressItemIndex];

                if (selectionItems->contains(item)) {
                    selectionItems->removeOne(item);
                } else {
                    QList<ListItem*> items = QList<ListItem*>();
                    items << item;
                    addSelections(items);
                }
            } else if ((mouseEvent->modifiers() == Qt::ShiftModifier) && !selectionItems->empty()) {
                int lastSelectionIndex = listItems->indexOf(lastSelectItem);
                int selectionStartIndex = std::min(pressItemIndex, lastSelectionIndex);
                int selectionEndIndex = std::max(pressItemIndex, lastSelectionIndex);

                shiftSelect(selectionStartIndex, selectionEndIndex);
            } else {
                clearSelections();

                QList<ListItem*> items = QList<ListItem*>();
                items << (*listItems)[pressItemIndex];
                addSelections(items);
            }

            repaint();
        }
    }
}

void ListView::pressHome() {
    clearSelections();

    QList<ListItem*> items = QList<ListItem*>();
    items << listItems->first();
    addSelections(items);

    renderOffset = 0;

    repaint();
}

void ListView::pressEnd() {
    clearSelections();

    QList<ListItem*> items = QList<ListItem*>();
    items << listItems->last();
    addSelections(items);

    renderOffset = listItems->count() * rowHeight - rect().height() + titleHeight;

    repaint();
}

void ListView::selectScrollUp(int scrollOffset) {
    if (selectionItems->empty()) {
        pressHome();
    } else {
        int firstIndex = listItems->count();
        for (ListItem *item:*selectionItems) {
            int index = listItems->indexOf(item);
            if (index < firstIndex) {
                firstIndex = index;
            }
        }

        if (firstIndex != -1) {
            firstIndex = std::max(0, firstIndex - scrollOffset);

            clearSelections();

            QList<ListItem*> items = QList<ListItem*>();
            items << (*listItems)[firstIndex];

            addSelections(items);

            int itemIndex = firstIndex - 1;
            int itemOffset = adjustRenderOffset(itemIndex * rowHeight + titleHeight);
            if ((renderOffset / rowHeight) > itemIndex) {
                renderOffset = itemOffset;
            }

            repaint();
        }
    }
}

void ListView::selectScrollDown(int scrollOffset) {
    if (selectionItems->empty()) {
        pressHome();
    } else {
        int lastIndex = 0;
        for (ListItem *item:*selectionItems) {
            int index = listItems->indexOf(item);
            if (index > lastIndex) {
                lastIndex = index;
            }
        }

        if (lastIndex != -1) {
            lastIndex = std::min(listItems->count() - 1, lastIndex + scrollOffset);

            clearSelections(false);

            QList<ListItem*> items = QList<ListItem*>();
            items << (*listItems)[lastIndex];

            addSelections(items);

            int itemIndex = lastIndex + 1;
            int itemOffset = adjustRenderOffset(itemIndex * rowHeight - rect().height() + titleHeight);
            if (((renderOffset + rect().height() - titleHeight) / rowHeight) < itemIndex) {
                renderOffset = itemOffset;
            }

            repaint();
        }
    }
}

void ListView::pressUp() {
    selectScrollUp(1);
}

void ListView::pressDown() {
    selectScrollDown(1);
}

void ListView::pressPageUp() {
    selectScrollUp((rect().height() - titleHeight) / rowHeight);
}

void ListView::pressPageDown() {
    selectScrollDown((rect().height() - titleHeight) / rowHeight);
}

void ListView::scrollPageUp() {
    renderOffset = adjustRenderOffset(renderOffset - (rect().height() - titleHeight));

    repaint();
}

void ListView::scrollPageDown() {
    renderOffset = adjustRenderOffset(renderOffset + (rect().height() - titleHeight));

    repaint();
}

void ListView::scrollHome() {
    renderOffset = 0;

    repaint();
}

void ListView::scrollEnd() {
    renderOffset = listItems->count() * rowHeight - rect().height() + titleHeight;

    repaint();
}

void ListView::shiftSelect(int selectionStartIndex, int selectionEndIndex) {
    // Note: Shift operation always selection bound from last selection index to current index.
    // So we don't need *clear* lastSelectionIndex for keep shift + button is right logic.
    clearSelections(false);
    QList<ListItem*> items = QList<ListItem*>();
    int index = 0;
    for (ListItem *item:*listItems) {
        if (index >= selectionStartIndex && index <= selectionEndIndex) {
            items << item;
        }

        index++;
    }
    // Note: Shift operation always selection bound from last selection index to current index.
    // So we don't need *record* lastSelectionIndex for keep shift + button is right logic.
    addSelections(items, false);
}

void ListView::shiftSelectHome() {
    if (selectionItems->empty()) {
        pressHome();
    } else {
        int lastSelectionIndex = listItems->indexOf(lastSelectItem);
        shiftSelect(0, lastSelectionIndex);

        renderOffset = 0;

        repaint();
    }
}

void ListView::shiftSelectEnd() {
    if (selectionItems->empty()) {
        pressEnd();
    } else {
        int lastSelectionIndex = listItems->indexOf(lastSelectItem);
        shiftSelect(lastSelectionIndex, listItems->count() - 1);

        renderOffset = listItems->count() * rowHeight - rect().height() + titleHeight;

        repaint();
    }
}

void ListView::shiftSelectScrollUp(int scrollOffset) {
    if (selectionItems->empty()) {
        pressHome();
    } else {
        int firstIndex = listItems->count();
        int lastIndex = 0;
        for (ListItem *item:*selectionItems) {
            int index = listItems->indexOf(item);

            if (index < firstIndex) {
                firstIndex = index;
            }

            if (index > lastIndex) {
                lastIndex = index;
            }
        }

        if (firstIndex != -1) {
            int lastSelectionIndex = listItems->indexOf(lastSelectItem);
            int selectionStartIndex, selectionEndIndex;

            if (lastIndex == lastSelectionIndex) {
                selectionStartIndex = std::max(0, firstIndex - scrollOffset);
                selectionEndIndex = lastSelectionIndex;
            } else {
                selectionStartIndex = firstIndex;
                selectionEndIndex = std::max(0, lastIndex - scrollOffset);
            }

            shiftSelect(selectionStartIndex, selectionEndIndex);

            renderOffset = adjustRenderOffset((selectionStartIndex - 1) * rowHeight + titleHeight);

            repaint();
        }
    }
}

void ListView::shiftSelectScrollDown(int scrollOffset) {
    if (selectionItems->empty()) {
        pressHome();
    } else {
        int firstIndex = listItems->count();
        int lastIndex = 0;
        for (ListItem *item:*selectionItems) {
            int index = listItems->indexOf(item);

            if (index < firstIndex) {
                firstIndex = index;
            }

            if (index > lastIndex) {
                lastIndex = index;
            }
        }

        if (firstIndex != -1) {
            int lastSelectionIndex = listItems->indexOf(lastSelectItem);
            int selectionStartIndex, selectionEndIndex;

            if (firstIndex == lastSelectionIndex) {
                selectionStartIndex = firstIndex;
                selectionEndIndex = std::min(listItems->count() - 1, lastIndex + scrollOffset);
            } else {
                selectionStartIndex = std::min(listItems->count() - 1, firstIndex + scrollOffset);
                selectionEndIndex = lastIndex;
            }

            shiftSelect(selectionStartIndex, selectionEndIndex);

            renderOffset = adjustRenderOffset((selectionEndIndex + 1) * rowHeight + titleHeight - rect().height());

            repaint();
        }
    }
}

void ListView::shiftSelectUp() {
    shiftSelectScrollUp(1);
}

void ListView::shiftSelectDown() {
    shiftSelectScrollDown(1);
}

void ListView::shiftSelectPageUp() {
    shiftSelectScrollUp((rect().height() - titleHeight) / rowHeight);
}

void ListView::shiftSelectPageDown() {
    shiftSelectScrollDown((rect().height() - titleHeight) / rowHeight);
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
            bool isSelect = selectionItems->contains(item);
            item->renderBackground(QRect(0, renderY + rowCounter * rowHeight - renderOffset, rect().width(), rowHeight), &painter, rowCounter, isSelect);

            int columnCounter = 0;
            int columnRenderX = 0;
            for (int renderWidth:renderWidths) {
                item->render(QRect(columnRenderX, renderY + rowCounter * rowHeight - renderOffset, renderWidth, rowHeight), &painter, columnCounter, isSelect);

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

    painter.setClipRect(QRectF(rect()));

    // Draw scrollbar.
    paintScrollbar(&painter);
}

void ListView::paintScrollbar(QPainter *painter) {
    if (listItems->count() * rowHeight > rect().height() - titleHeight) {
        qreal barOpacitry = mouseAtScrollArea ? 0.5 : 0.1;
        int barWidth = mouseAtScrollArea ? scrollbarDragWidth : scrollbarDefaultWidth;
        int barRadius = 5;

        int barY = static_cast<int>((renderOffset / (listItems->count() * rowHeight * 1.0)) * (rect().height() - titleHeight) + titleHeight);
        int barHeight = getScrollbarHeight();

        painter->setOpacity(barOpacitry);
        QPainterPath path;
        path.addRoundedRect(QRectF(rect().x() + rect().width() - barWidth, barY + barRadius, barWidth, barHeight - barRadius * 2), barRadius, barRadius);
        painter->fillPath(path, QColor("#F5F5F5"));
    }
}

int ListView::getScrollbarHeight() {
    return std::max(static_cast<int>((rect().height() - titleHeight) / (listItems->count() * rowHeight * 1.0) * rect().height()), 30);
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
