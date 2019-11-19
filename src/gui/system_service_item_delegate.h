#ifndef SYSTEM_SERVICE_ITEM_DELEGATE_H
#define SYSTEM_SERVICE_ITEM_DELEGATE_H

#include <QStyledItemDelegate>

class QModelIndex;
class QPainter;
class QStyleOptionViewItem;

class SystemServiceItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit SystemServiceItemDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QWidget *createEditor(QWidget *, const QStyleOptionViewItem &,
                          const QModelIndex &) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

protected:
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;
};

#endif  // SYSTEM_SERVICE_ITEM_DELEGATE_H
