#ifndef BASE_DETAIL_ITEM_DELEGATE_H
#define BASE_DETAIL_ITEM_DELEGATE_H

#include <QStyledItemDelegate>
class BaseDetailItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit BaseDetailItemDelegate(QObject *parent = nullptr);

    virtual ~BaseDetailItemDelegate();
public:
    bool helpEvent(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index);
};

#endif // BASE_DETAIL_ITEM_DELEGATE_H
