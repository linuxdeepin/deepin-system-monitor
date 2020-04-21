#ifndef SYSTEM_SERVICE_SORT_FILTER_PROXY_MODEL_H
#define SYSTEM_SERVICE_SORT_FILTER_PROXY_MODEL_H

#include <QSortFilterProxyModel>

class SystemServiceSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    SystemServiceSortFilterProxyModel(QObject *parent = nullptr);
    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool filterAcceptsColumn(int column, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
};

#endif  // SYSTEM_SERVICE_SORT_FILTER_PROXY_MODEL_H
