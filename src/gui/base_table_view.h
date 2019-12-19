#ifndef BASE_TABLE_VIEW_H
#define BASE_TABLE_VIEW_H

#include <DTreeView>

#include "base_header_view.h"
#include "base_item_delegate.h"

DWIDGET_USE_NAMESPACE

class BaseTableView : public DTreeView
{
    Q_OBJECT

public:
    BaseTableView(DWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    //    void drawRow(QPainter *painter, const QStyleOptionViewItem &options,
    //                 const QModelIndex &index) const override;

private:
    BaseItemDelegate *m_itemDelegate {nullptr};
    BaseHeaderView *m_headerView {nullptr};
};

#endif  // BASE_TABLE_VIEW_H
