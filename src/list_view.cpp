#include "list_view.h"
#include "utils.h"
#include <QApplication>
#include <QDebug>
#include <QEvent>
#include <QWheelEvent>
#include <QtMath>

ListView::ListView(QWidget *parent) : QWidget(parent)
{
    // Init.
    installEventFilter(this);   // add event filter
    setMouseTracking(true);    // make MouseMove can response

    scrollDistance = 0;
    renderOffset = 0;
    titleHeight = 0;

    scrollAnimationTimer = NULL;
    scrollAnimationTicker = 0;
    scrollAnimationFrames = 16;
    scrollAnimationDuration = 25;

    listItems = new QList<ListItem*>();
    selectionItems = new QList<ListItem*>();
    lastSelectItem = NULL;

    mouseAtScrollArea = false;
    mouseDragScrollbar = false;
    scrollbarDefaultWidth = 3;
    scrollbarDragWidth = 8;
    scrollbarMinHeight = 30;
    hideScrollbarDuration = 1000;

    oldRenderOffset = 0;

    hideScrollbarTimer = NULL;

    sortingAlgorithms = new QList<SortAlgorithm>();
    sortingOrderes = new QList<bool>();
}

ListView::~ListView()
{
    delete lastSelectItem;
    delete listItems;
    delete selectionItems;
    delete sortingAlgorithms;
    delete sortingOrderes;
    delete hideScrollbarTimer;
    delete scrollAnimationTimer;
}

void ListView::setRowHeight(int height)
{
    rowHeight = height;
    scrollUnit = rowHeight * 9;
}

void ListView::setColumnTitles(QList<QString> titles, int height)
{
    columnTitles = titles;
    titleHeight = height;
}

void ListView::setColumnWidths(QList<int> widths)
{
    columnWidths = widths;
}

void ListView::setColumnSortingAlgorithms(QList<SortAlgorithm> *algorithms, int sortColumn, bool descendingSort)
{
    // Add sort algorithms.
    sortingAlgorithms = algorithms;

    for (int i = 0; i < sortingAlgorithms->count(); i++) {
        sortingOrderes->append(false);
    }

    // If sort column is -1, don't sort default.
    defaultSortingColumn = sortColumn;
    defaultSortingOrder = descendingSort;
}

void ListView::addItems(QList<ListItem*> items)
{
    // Add item to list.
    listItems->append(items);

    // If user has click title to sort, sort items after add items to list.
    if (defaultSortingColumn != -1) {
        sortItemsByColumn(defaultSortingColumn, defaultSortingOrder);
    }
}

void ListView::clearItems()
{
    // NOTE:
    // We need delete items in QList before clear QList to avoid *MEMORY LEAK* .
    qDeleteAll(listItems->begin(), listItems->end());
    listItems->clear();
}

void ListView::addSelections(QList<ListItem*> items, bool recordLastSelection)
{
    // Add item to selection list.
    selectionItems->append(items);

    // Record last selection item to make selected operation continuously.
    if (recordLastSelection && selectionItems->count() > 0) {
        lastSelectItem = selectionItems->last();
    }
}

void ListView::clearSelections(bool clearLastSelection)
{
    // Clear selection list.
    selectionItems->clear();

    if (clearLastSelection) {
        lastSelectItem = NULL;
    }
}

void ListView::refreshItems(QList<ListItem*> items)
{
    // Init.
    QList<ListItem*> *newSelectionItems = new QList<ListItem*>();
    ListItem *newLastSelectionItem = NULL;

    // Save selection items and last selection item.
    for (ListItem *item:items) {
        for (ListItem *selectionItem:*selectionItems) {
            if (item->sameAs(selectionItem)) {
                newSelectionItems->append(item);
                break;
            }
        }
    }
    if (lastSelectItem != NULL) {
        for (ListItem *item:items) {
            if (item->sameAs(lastSelectItem)) {
                newLastSelectionItem = item;
                break;
            }
        }
    }

    // Update items.
    clearItems();
    listItems->append(items);

    // Sort once if default sort column hasn't init.
    if (defaultSortingColumn != -1) {
        sortItemsByColumn(defaultSortingColumn, defaultSortingOrder);
    }

    // Restore selection items and last selection item.
    clearSelections();
    addSelections(*newSelectionItems, false);
    lastSelectItem = newLastSelectionItem;

    // Keep scroll position.
    renderOffset = adjustRenderOffset(renderOffset);

    // Render.
    repaint();
}

void ListView::selectAllItems()
{
    // Record old render offset to control scrollbar whether display.
    oldRenderOffset = renderOffset;

    // Select all items.
    clearSelections();
    addSelections(*listItems);

    // Scroll to top.
    renderOffset = getTopRenderOffset();

    // Repaint.
    repaint();
}

void ListView::selectFirstItem()
{
    // Record old render offset to control scrollbar whether display.
    oldRenderOffset = renderOffset;

    // Select first item.
    clearSelections();

    QList<ListItem*> items = QList<ListItem*>();
    items << listItems->first();
    addSelections(items);

    // Scroll to top.
    renderOffset = getTopRenderOffset();

    // Repaint.
    repaint();
}

void ListView::selectLastItem()
{
    // Record old render offset to control scrollbar whether display.
    oldRenderOffset = renderOffset;

    // Select last item.
    clearSelections();

    QList<ListItem*> items = QList<ListItem*>();
    items << listItems->last();
    addSelections(items);

    // Scroll to bottom.
    renderOffset = getBottomRenderOffset();

    // Repaint.
    repaint();
}

void ListView::selectPrevItem()
{
    selectPrevItemWithOffset(1);
}

void ListView::selectNextItem()
{
    selectNextItemWithOffset(1);
}

void ListView::shiftSelectPageDown()
{
    shiftSelectNextItemWithOffset(getScrollAreaHeight() / rowHeight);
}

void ListView::shiftSelectPageUp()
{
    shiftSelectPrevItemWithOffset(getScrollAreaHeight() / rowHeight);
}

void ListView::shiftSelectToEnd()
{
    // Select last item if nothing selected yet.
    if (selectionItems->empty()) {
        selectLastItem();
    }
    // Select items from last selected item to last item.
    else {
        // Found last selected index and do select operation.
        int lastSelectionIndex = listItems->indexOf(lastSelectItem);
        shiftSelectItemsWithBound(lastSelectionIndex, listItems->count() - 1);

        // Scroll to bottom.
        renderOffset = getBottomRenderOffset();

        // Repaint.
        repaint();
    }
}

void ListView::shiftSelectToHome()
{
    // Select first item if nothing selected yet.
    if (selectionItems->empty()) {
        selectFirstItem();
    }
    // Select items from last selected item to first item.
    else {
        // Found last selected index and do select operation.
        int lastSelectionIndex = listItems->indexOf(lastSelectItem);
        shiftSelectItemsWithBound(0, lastSelectionIndex);

        // Scroll to top.
        renderOffset = getTopRenderOffset();

        // Repaint.
        repaint();
    }
}

void ListView::shiftSelectToNext()
{
    shiftSelectNextItemWithOffset(1);
}

void ListView::shiftSelectToPrev()
{
    shiftSelectPrevItemWithOffset(1);
}

void ListView::scrollPageDown()
{
    selectNextItemWithOffset(getScrollAreaHeight() / rowHeight);
}

void ListView::scrollPageUp()
{
    selectPrevItemWithOffset(getScrollAreaHeight() / rowHeight);
}

void ListView::ctrlScrollPageUp()
{
    renderOffset = adjustRenderOffset(renderOffset - getScrollAreaHeight());

    repaint();
}

void ListView::ctrlScrollPageDown()
{
    renderOffset = adjustRenderOffset(renderOffset + getScrollAreaHeight());

    repaint();
}

void ListView::ctrlScrollToHome()
{
    renderOffset = getTopRenderOffset();

    repaint();
}

void ListView::ctrlScrollToEnd()
{
    renderOffset = getBottomRenderOffset();

    repaint();
}

void ListView::scrollAnimation()
{
    if (scrollAnimationTicker <= scrollAnimationFrames) {
        renderOffset = adjustRenderOffset(scrollStartY + Utils::easeInOut(scrollAnimationTicker / (scrollAnimationFrames * 1.0)) * scrollDistance);

        repaint();

        scrollAnimationTicker++;
    } else {
        scrollAnimationTimer->stop();
    }
}

void ListView::hideScrollbar()
{
    // Record old render offset to control scrollbar whether display.
    oldRenderOffset = renderOffset;

    repaint();
}

bool ListView::eventFilter(QObject *, QEvent *)
{
    return false;
}

void ListView::keyPressEvent(QKeyEvent *keyEvent)
{
    if (keyEvent->key() == Qt::Key_Home) {
        if (keyEvent->modifiers() == Qt::ControlModifier) {
            ctrlScrollToHome();
        } else if (keyEvent->modifiers() == Qt::ShiftModifier) {
            shiftSelectToHome();
        }else {
            selectFirstItem();
        }
    } else if (keyEvent->key() == Qt::Key_End) {
        if (keyEvent->modifiers() == Qt::ControlModifier) {
            ctrlScrollToEnd();
        } else if (keyEvent->modifiers() == Qt::ShiftModifier) {
            shiftSelectToEnd();
        } else {
            selectLastItem();
        }
    } else if (keyEvent->key() == Qt::Key_Up) {
        if (keyEvent->modifiers() == Qt::ShiftModifier) {
            shiftSelectToPrev();
        } else {
            selectPrevItem();
        }
    } else if (keyEvent->key() == Qt::Key_Down) {
        if (keyEvent->modifiers() == Qt::ShiftModifier) {
            shiftSelectToNext();
        } else {
            selectNextItem();
        }
    } else if (keyEvent->key() == Qt::Key_PageUp) {
        if (keyEvent->modifiers() == Qt::ControlModifier) {
            ctrlScrollPageUp();
        } else if (keyEvent->modifiers() == Qt::ShiftModifier) {
            shiftSelectPageUp();
        } else {
            scrollPageUp();
        }
    } else if (keyEvent->key() == Qt::Key_PageDown) {
        if (keyEvent->modifiers() == Qt::ControlModifier) {
            ctrlScrollPageDown();
        } else if (keyEvent->modifiers() == Qt::ShiftModifier) {
            shiftSelectPageDown();
        } else {
            scrollPageDown();
        }
    } else if (keyEvent->key() == Qt::Key_A) {
        if (keyEvent->modifiers() == Qt::ControlModifier) {
            selectAllItems();
        }
    }
}

void ListView::mouseMoveEvent(QMouseEvent *mouseEvent)
{
    // Scroll if mouse drag at scrollbar.
    if (mouseDragScrollbar) {
        int barHeight = getScrollbarHeight();
        renderOffset = adjustRenderOffset((mouseEvent->y() - barHeight / 2 - titleHeight) / (getScrollAreaHeight() * 1.0) * getItemsTotalHeight());

        repaint();
    }
    // Otherwise update scrollbar status with mouse position.
    else {
        bool atScrollArea = isMouseAtScrollArea(mouseEvent->x());

        // Update scrollbar status when mouse in or out of scrollbar area.
        if (atScrollArea != mouseAtScrollArea) {
            mouseAtScrollArea = atScrollArea;
            repaint();
        }
    }
}

void ListView::mousePressEvent(QMouseEvent *mouseEvent)
{
    bool atTitleArea = isMouseAtTitleArea(mouseEvent->y());
    bool atScrollArea = isMouseAtScrollArea(mouseEvent->x());

    // Sort items with column's sorting algorithms when click on title area.
    if (atTitleArea) {
        if (sortingAlgorithms->count() != 0 && sortingAlgorithms->count() == columnTitles.count() && sortingOrderes->count() == columnTitles.count()) {
            // Calcuate title widths;
            QList<int> renderWidths = getRenderWidths();

            int columnCounter = 0;
            int columnRenderX = 0;
            for (int renderWidth:renderWidths) {
                if (mouseEvent->x() > columnRenderX && mouseEvent->x() < columnRenderX + renderWidth) {
                    // If switch other column, default order is from top to bottom.
                    if (columnCounter != defaultSortingColumn) {
                        (*sortingOrderes)[columnCounter] = true;
                    }
                    // If user click same column, just switch reverse order.
                    else {
                        (*sortingOrderes)[columnCounter] = !(*sortingOrderes)[columnCounter];
                    }

                    defaultSortingColumn = columnCounter;
                    defaultSortingOrder = (*sortingOrderes)[columnCounter];

                    sortItemsByColumn(columnCounter, (*sortingOrderes)[columnCounter]);

                    repaint();
                    break;
                }

                columnRenderX += renderWidth;
                columnCounter++;
            }
        }
    }
    // Scroll when click on scrollbar area.
    else if (atScrollArea) {
        int barHeight = getScrollbarHeight();
        int barY = getScrollbarY();

        // Flag mouseDragScrollbar when click on scrollbar.
        if (mouseEvent->y() > barY && mouseEvent->y() < barY + barHeight) {
            mouseDragScrollbar = true;
        }
        // Scroll if click out of scrollbar area.
        else {
            renderOffset = adjustRenderOffset((mouseEvent->y() - barHeight / 2 - titleHeight) / (getScrollAreaHeight() * 1.0) * getItemsTotalHeight());
            repaint();
        }
    }
    // Select items.
    else {
        int pressItemIndex = (renderOffset + mouseEvent->y() - titleHeight) / rowHeight;

        if (mouseEvent->button() == Qt::LeftButton) {
            if (pressItemIndex < listItems->count()) {
                // Scattered selection of items when press ctrl modifier.
                if (mouseEvent->modifiers() == Qt::ControlModifier) {
                    ListItem *item = (*listItems)[pressItemIndex];

                    if (selectionItems->contains(item)) {
                        selectionItems->removeOne(item);
                    } else {
                        QList<ListItem*> items = QList<ListItem*>();
                        items << item;
                        addSelections(items);
                    }
                }
                // Continuous selection of items when press shift modifier.
                else if ((mouseEvent->modifiers() == Qt::ShiftModifier) && !selectionItems->empty()) {
                    int lastSelectionIndex = listItems->indexOf(lastSelectItem);
                    int selectionStartIndex = std::min(pressItemIndex, lastSelectionIndex);
                    int selectionEndIndex = std::max(pressItemIndex, lastSelectionIndex);

                    shiftSelectItemsWithBound(selectionStartIndex, selectionEndIndex);
                }
                // Just select item under mouse if user not press any modifier.
                else {
                    clearSelections();

                    QList<ListItem*> items = QList<ListItem*>();
                    items << (*listItems)[pressItemIndex];
                    addSelections(items);
                }

                repaint();
            }
        } else if (mouseEvent->button() == Qt::RightButton) {
            ListItem *pressItem = (*listItems)[pressItemIndex];
            bool pressInSelectionArea = false;
            
            for (ListItem *item : *selectionItems) {
                if (item == pressItem) {
                    pressInSelectionArea = true;
                    
                    break;
                }
            }
            
            if (!pressInSelectionArea) {
                clearSelections();
                
                QList<ListItem*> items = QList<ListItem*>();
                items << (*listItems)[pressItemIndex];
                addSelections(items);
                
                repaint();
            }
            
            rightClickItems(mouseEvent->pos(), *selectionItems);
        }
    }
}

void ListView::mouseReleaseEvent(QMouseEvent *)
{
    if (mouseDragScrollbar) {
        // Reset mouseDragScrollbar.
        mouseDragScrollbar = false;

        repaint();
    }
}

void ListView::wheelEvent(QWheelEvent *event)
{
    if (event->orientation() == Qt::Vertical) {
        // Record old render offset to control scrollbar whether display.
        oldRenderOffset = renderOffset;

        int scrollStep = event->angleDelta().y() / 120;
        int newRenderOffset = renderOffset - scrollStep * scrollUnit;
        newRenderOffset = adjustRenderOffset(newRenderOffset);

        if (newRenderOffset != renderOffset) {
            // If timer is inactive, start scroll timer.
            if (scrollAnimationTimer == NULL || !scrollAnimationTimer->isActive()) {
                scrollStartY = renderOffset;
                scrollDistance = newRenderOffset - renderOffset;

                startScrollAnimation();
            }
            // If timer is active, just add scroll offset make scroll faster and *smooth*.
            else {
                scrollDistance -= scrollStep * rowHeight;
            }
        }
    }

    event->accept();
}

void ListView::paintEvent(QPaintEvent *)
{
    // Init.
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Calcuate title widths;
    QList<int> renderWidths = getRenderWidths();

    // Draw title.
    int renderY = 0;
    int renderHeight = 0;
    if (titleHeight > 0) {
        int columnCounter = 0;
        int columnRenderX = 0;
        for (int renderWidth:renderWidths) {
            painter.setOpacity(1);
            Utils::setFontSize(painter, 10);
            painter.setPen(QPen(QColor("#666666")));
            painter.drawText(QRect(columnRenderX, 0, renderWidth, titleHeight), Qt::AlignCenter, columnTitles[columnCounter]);

            columnRenderX += renderWidth;
            columnCounter++;
        }

        renderY += titleHeight;
        renderHeight += titleHeight;
    }

    // Draw context.
    int rowCounter = 0;
    for (ListItem *item:*listItems) {
        if (rowCounter > ((renderOffset - rowHeight) / rowHeight)) {
            // Clip item rect.
            painter.setClipRect(QRect(0, renderY + rowCounter * rowHeight - renderOffset, rect().width(), rowHeight));
            painter.setClipRect(QRectF(rect().x(), rect().y() + titleHeight, rect().width(), getScrollAreaHeight()));

            // Draw item backround.
            bool isSelect = selectionItems->contains(item);
            item->drawBackground(QRect(0, renderY + rowCounter * rowHeight - renderOffset, rect().width(), rowHeight), &painter, rowCounter, isSelect);

            // Draw item foreground.
            int columnCounter = 0;
            int columnRenderX = 0;
            for (int renderWidth:renderWidths) {
                item->drawForeground(QRect(columnRenderX, renderY + rowCounter * rowHeight - renderOffset, renderWidth, rowHeight), &painter, columnCounter, rowCounter, isSelect);

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

    // Draw scrollbar.
    if (mouseAtScrollArea) {
        paintScrollbar(&painter);
    } else if (oldRenderOffset != renderOffset) {
        paintScrollbar(&painter);

        startScrollbarHideTimer();
    }
}

void ListView::paintScrollbar(QPainter *painter)
{
    if (getItemsTotalHeight() > getScrollAreaHeight()) {
        // Init scrollbar opacity with scrollbar status.
        qreal barOpacitry = 0;
        if (mouseDragScrollbar) {
            barOpacitry = 0.8;
        } else {
            if (mouseAtScrollArea) {
                barOpacitry = 0.5;
            } else {
                barOpacitry = 0.1;
            }
        }

        int barWidth = mouseAtScrollArea ? scrollbarDragWidth : scrollbarDefaultWidth;
        int barRadius = 5;

        int barY = getScrollbarY();
        int barHeight = getScrollbarHeight();

        painter->setOpacity(barOpacitry);
        QPainterPath path;
        path.addRoundedRect(QRectF(rect().x() + rect().width() - barWidth, barY + barRadius, barWidth, barHeight - barRadius * 2), barRadius, barRadius);
        painter->fillPath(path, QColor("#F5F5F5"));
    }
}

void ListView::selectNextItemWithOffset(int scrollOffset)
{
    // Record old render offset to control scrollbar whether display.
    oldRenderOffset = renderOffset;

    if (selectionItems->empty()) {
        selectFirstItem();
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
            if (((renderOffset + getScrollAreaHeight()) / rowHeight) < itemIndex) {
                renderOffset = itemOffset;
            }

            repaint();
        }
    }
}

void ListView::selectPrevItemWithOffset(int scrollOffset)
{
    // Record old render offset to control scrollbar whether display.
    oldRenderOffset = renderOffset;

    if (selectionItems->empty()) {
        selectFirstItem();
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

void ListView::shiftSelectItemsWithBound(int selectionStartIndex, int selectionEndIndex)
{
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

void ListView::shiftSelectPrevItemWithOffset(int scrollOffset)
{
    // Record old render offset to control scrollbar whether display.
    oldRenderOffset = renderOffset;

    if (selectionItems->empty()) {
        selectFirstItem();
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

            shiftSelectItemsWithBound(selectionStartIndex, selectionEndIndex);

            renderOffset = adjustRenderOffset((selectionStartIndex - 1) * rowHeight + titleHeight);

            repaint();
        }
    }
}

void ListView::shiftSelectNextItemWithOffset(int scrollOffset)
{
    // Record old render offset to control scrollbar whether display.
    oldRenderOffset = renderOffset;

    if (selectionItems->empty()) {
        selectFirstItem();
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

            shiftSelectItemsWithBound(selectionStartIndex, selectionEndIndex);

            renderOffset = adjustRenderOffset((selectionEndIndex + 1) * rowHeight + titleHeight - rect().height());

            repaint();
        }
    }
}

QList<int> ListView::getRenderWidths()
{
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

bool ListView::isMouseAtScrollArea(int x)
{
    return (x > rect().x() + rect().width() - scrollbarDragWidth) && (x < rect().x() + rect().width());
}

bool ListView::isMouseAtTitleArea(int y)
{
    return (y > rect().y() && y < rect().y() + titleHeight);
}

int ListView::adjustRenderOffset(int offset)
{
    return std::max(0, std::min(offset, getBottomRenderOffset()));
}

int ListView::getItemsTotalHeight()
{
    return listItems->count() * rowHeight;
}

int ListView::getScrollAreaHeight()
{
    return rect().height() - titleHeight;
}

int ListView::getScrollbarY()
{
    return static_cast<int>((renderOffset / (getItemsTotalHeight() * 1.0)) * getScrollAreaHeight() + titleHeight);
}

int ListView::getScrollbarHeight()
{
    return std::max(static_cast<int>(getScrollAreaHeight() / (getItemsTotalHeight() * 1.0) * rect().height()), scrollbarMinHeight);
}

int ListView::getTopRenderOffset()
{
    return 0;
}

int ListView::getBottomRenderOffset()
{
    return getItemsTotalHeight() - rect().height() + titleHeight;
}

void ListView::sortItemsByColumn(int column, bool descendingSort)
{
    if (sortingAlgorithms->count() != 0 && sortingAlgorithms->count() == columnTitles.count() && sortingOrderes->count() == columnTitles.count()) {
        qSort(listItems->begin(), listItems->end(), [&](const ListItem *item1, const ListItem *item2) {
                return (*sortingAlgorithms)[column](item1, item2, descendingSort);
            });
    }
}

void ListView::startScrollAnimation()
{
    if (scrollAnimationTimer == NULL || !scrollAnimationTimer->isActive()) {
        scrollAnimationTicker = 0;
        scrollAnimationTimer = new QTimer();
        connect(scrollAnimationTimer, SIGNAL(timeout()), this, SLOT(scrollAnimation()));
        scrollAnimationTimer->start(scrollAnimationDuration);
    }
}

void ListView::startScrollbarHideTimer()
{
    if (hideScrollbarTimer != NULL) {
        hideScrollbarTimer->stop();
    }

    hideScrollbarTimer = new QTimer();
    connect(hideScrollbarTimer, SIGNAL(timeout()), this, SLOT(hideScrollbar()));
    hideScrollbarTimer->start(hideScrollbarDuration);
}
