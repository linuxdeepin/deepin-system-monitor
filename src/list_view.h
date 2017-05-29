#ifndef LISTVIEW_H
#define LISTVIEW_H

#include "list_item.h"
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
     */
    void setColumnHideFlags(QList<bool> toggleHideFlags);
    
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
                           
signals:
    void rightClickItems(QPoint pos, QList<ListItem*> items);
    
private slots:
    void scrollAnimation();
    void hideScrollbar();
    
private:
    bool eventFilter(QObject *, QEvent *event);
    void keyPressEvent(QKeyEvent *keyEvent);
    void mouseMoveEvent(QMouseEvent *mouseEvent);
    void mousePressEvent(QMouseEvent *mouseEvent);
    void mouseReleaseEvent(QMouseEvent *mouseEvent);
    void wheelEvent(QWheelEvent *event);
    
    void paintEvent(QPaintEvent *);
    void paintScrollbar(QPainter *painter);
    
    void selectNextItemWithOffset(int scrollOffset);
    void selectPrevItemWithOffset(int scrollOffset);
    
    void shiftSelectItemsWithBound(int selectionStartIndex, int selectionEndIndex);
    void shiftSelectNextItemWithOffset(int scrollOffset);
    void shiftSelectPrevItemWithOffset(int scrollOffset);
                        
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
    QList<ListItem*> getSearchItems(QList<ListItem*> items);
    void sortItemsByColumn(int column, bool descendingSort);
    void startScrollAnimation();
    void startScrollbarHideTimer();
    
    ListItem *lastSelectItem;
    QList<ListItem*> *listItems;
    QList<ListItem*> *renderItems;
    QList<ListItem*> *selectionItems;
    QList<QString> columnTitles;
    QList<SortAlgorithm> *sortingAlgorithms;
    QList<bool> *sortingOrderes;
    QList<bool> columnToggleHideFlags;
    QList<bool> columnVisibles;
    QList<int> columnWidths;
    QString searchContent;
    QTimer *hideScrollbarTimer;
    QTimer *scrollAnimationTimer;
    SearchAlgorithm searchAlgorithm;
    bool defaultSortingOrder;
    bool mouseAtScrollArea;
    bool mouseDragScrollbar;
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
    int titleHeight;
};

#endif
