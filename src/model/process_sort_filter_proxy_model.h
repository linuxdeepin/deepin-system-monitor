#ifndef PROCESS_SORT_FILTER_PROXY_MODEL_H
#define PROCESS_SORT_FILTER_PROXY_MODEL_H

#include <QSortFilterProxyModel>

class ProcessSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit ProcessSortFilterProxyModel(QObject *parent = nullptr);

    void setSortFilterString(const QString &search);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool filterAcceptsColumn(int column, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    QString m_search {};
    QString m_hanwords {};
    QString m_capwords {};
};

#endif  // PROCESS_SORT_FILTER_PROXY_MODEL_H
