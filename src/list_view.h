/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2017 Deepin, Inc.
 *               2011 ~ 2017 Wang Yong
 *
 * Author:     Wang Yong <wangyong@deepin.com>
 * Maintainer: Wang Yong <wangyong@deepin.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */ 

#ifndef LISTVIEW_H
#define LISTVIEW_H

#include "list_item.h"
#include <QImage>
#include <QTimer>
#include <QWidget>

typedef bool (* SortAlgorithm) (const ListItem *item1, const ListItem *item2, bool descendingSort);
typedef bool (* SearchAlgorithm) (const ListItem *item, QString searchContent);

class ListView : public QWidget
{
    Q_OBJECT
    
public:
    ListView(QWidget *parent = 0);
    ~ListView();

    // ListView interfaces.
    
    /* 
     * Set row height of ListView.
     * 
     * @height the height of row
     */
    void setRowHeight(int height);

    /*
     * Set column titles.
     * ListView won't display titlebar if not call this function when create ListView.
     * 
     * @titles a list to contains titles
     * @height height of titlebar
     */
    void setColumnTitles(QList<QString> titles, int height);
    
    /*
     * Set column hide flags.
     * At least have false in hide flags list, and hide flags count must same as titles list.
     *
     * @toggleHideFlags the hide flags to control column wether toggle show/hide.
     * @alwaysVisibleColumn the column index that column is always visible, default is -1, mean no column can always visible.
     */
    void setColumnHideFlags(QList<bool> toggleHideFlags, int alwaysVisibleColumn=-1);
    
    /*
     * Set column widths
     * If you want some column use expand space, please set width with -1
     * Only allowed one -1 to set in width list.
     * 
     * @widths the width of column, list length same as titles list
     */
    void setColumnWidths(QList<int> widths);
    
    /*
     * Set column sorting algorithms.
     * Note SortAlgorithm function type must be 'static', otherwise function pointer can't match type.
     * 
     * @algorithms a list of SortAlgorithm, SortAlgorithm is function pointer, it's type is: 'bool (*) (const ListItem *item1, const ListItem *item2, bool descendingSort)'
     * @sortColumn default sort column, -1 mean don't sort any column default
     * @descendingSort whether sort column descending, default is false
     */
    void setColumnSortingAlgorithms(QList<SortAlgorithm> *algorithms, int sortColumn=-1, bool descendingSort=false);
    
    /*
     * Set search algorithm to filter match items.
     * 
     * @algorithm the search algorithm, it's type is: 'bool (*) (const ListItem *item, QString searchContent)'
     */
    void setSearchAlgorithm(SearchAlgorithm algorithm);
    
    /*
     * Set radius to clip listview.
     * 
     * @radius the radius of clip area, default is 8 pixel.
     */
    void setClipRadius(int radius);
    
    /* 
     * Add ListItem list to ListView.
     * If user has click title to sort, sort items after add items to list. 
     * 
     * @items List of LiteItem*
     */
    void addItems(QList<ListItem*> items);
    
    /* 
     * Clear items from ListView.
     */
    void clearItems();
    
    /*
     * Add ListItem list to mark selected effect in ListView.
     * 
     * @items List of ListItem* to mark selected
     * @recordLastSelection record last selection item to make selected operation continuously, default is true
     */
    void addSelections(QList<ListItem*> items, bool recordLastSelection=true);
    
    /*
     * Clear selection items from ListView.
     * 
     * @clearLastSelection clear last selection item if option is true, default is true
     */
    void clearSelections(bool clearLastSelection=true);
    
    /*
     * Refresh all items in ListView.
     * This function is different that addItems is: it will clear items first before add new items.
     * This function will keep selection status and scroll offset when add items.
     * 
     * @items List of ListItem* to add
     */
    void refreshItems(QList<ListItem*> items);
    
    /*
     * Search
     */
    void search(QString searchContent);
    
    // ListView operations.
    void selectAllItems();
    void selectFirstItem();
    void selectLastItem();
    void selectNextItem();
    void selectPrevItem();
    
    void shiftSelectPageDown();
    void shiftSelectPageUp();
    void shiftSelectToEnd();
    void shiftSelectToHome();
    void shiftSelectToNext();
    void shiftSelectToPrev();
    
    void scrollPageDown();
    void scrollPageUp();
    
    void ctrlScrollPageDown();
    void ctrlScrollPageUp();
    void ctrlScrollToEnd();
    void ctrlScrollToHome();
    
protected:
    virtual void leaveEvent(QEvent * event);
    
    QString titleColor = "#000000";
    QString titleLineColor = "#000000";
    
    QString titleAreaColor = "#ffffff";
    double titleAreaOpacity = 0.02;
    
    QString backgroundColor = "#ffffff";
    double backgroundOpacity = 0.03;
    
    QString frameColor = "#000000";
    double frameOpacity = 0.1;
    
    QString searchColor = "#000000";
                           
signals:
    void rightClickItems(QPoint pos, QList<ListItem*> items);
    void columnToggleStatus(int index, bool visible, QList<bool> columnVisibles);
    
private slots:
    void scrollAnimation();
    void hideScrollbar();
    
private:
    bool eventFilter(QObject *, QEvent *event);
    void keyPressEvent(QKeyEvent *keyEvent);
    void mouseMoveEvent(QMouseEvent *mouseEvent);
    void mousePressEvent(QMouseEvent *mouseEvent);
    void mouseReleaseEvent(QMouseEvent *mouseEvent);
    void paintEvent(QPaintEvent *);
    void paintScrollbar(QPainter *painter);
    void selectNextItemWithOffset(int scrollOffset);
    void selectPrevItemWithOffset(int scrollOffset);
    void shiftSelectItemsWithBound(int selectionStartIndex, int selectionEndIndex);
    void shiftSelectNextItemWithOffset(int scrollOffset);
    void shiftSelectPrevItemWithOffset(int scrollOffset);
    void wheelEvent(QWheelEvent *event);
                        
    QList<ListItem*> getSearchItems(QList<ListItem*> items);
    QList<int> getRenderWidths();
    bool isMouseAtScrollArea(int x);
    bool isMouseAtTitleArea(int y);
    int adjustRenderOffset(int offset);
    int getBottomRenderOffset();
    int getItemsTotalHeight();
    int getScrollAreaHeight();
    int getScrollbarHeight();
    int getScrollbarY();
    int getTopRenderOffset();
    void sortItemsByColumn(int column, bool descendingSort);
    void startScrollAnimation();
    void startScrollbarHideTimer();
    
    ListItem *lastSelectItem;
    QImage arrowDownImage;
    QImage arrowUpImage;
    QList<ListItem*> *listItems;
    QList<ListItem*> *renderItems;
    QList<ListItem*> *selectionItems;
    QList<QString> columnTitles;
    QList<SortAlgorithm> *sortingAlgorithms;
    QList<bool> *sortingOrderes;
    QList<bool> columnVisibles;
    QList<int> columnWidths;
    QString searchContent;
    QTimer *hideScrollbarTimer;
    QTimer *scrollAnimationTimer;
    SearchAlgorithm searchAlgorithm;
    bool defaultSortingOrder;
    bool mouseAtScrollArea;
    bool mouseDragScrollbar;
    int alwaysVisibleColumn;
    int clipRadius;
    int defaultSortingColumn;
    int hideScrollbarDuration;
    int oldRenderOffset;
    int renderOffset;
    int rowHeight;
    int scrollAnimationDuration;
    int scrollAnimationFrames;
    int scrollAnimationTicker;
    int scrollDistance;
    int scrollStartY;
    int scrollUnit;
    int scrollbarDefaultWidth;
    int scrollbarDragWidth;
    int scrollbarMinHeight;
    int scrollbarPadding;
    int titleArrowPadding;
    int titleHeight;
    int titlePadding;
};

#endif
