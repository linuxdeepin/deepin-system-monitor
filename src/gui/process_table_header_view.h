#ifndef PROCESS_TABLE_HEADER_VIEW_H
#define PROCESS_TABLE_HEADER_VIEW_H

#include <DHeaderView>
#include <DStyle>

DWIDGET_USE_NAMESPACE

class ProcessTableHeaderView : public DHeaderView
{
    Q_OBJECT

public:
    explicit ProcessTableHeaderView(Qt::Orientation orientation, QWidget *parent = nullptr);

    QSize sizeHint() const override;

    inline int getSpacing() const { return m_spacing; }
    inline void setSpacing(int spacing) { m_spacing = spacing; }

protected:
    void paintEvent(QPaintEvent *e) override;
    virtual void paintSection(QPainter *painter, const QRect &rect,
                              int logicalIndex) const override;

private:
    int m_spacing {1};
};

#endif  // PROCESS_TABLE_HEADER_VIEW_H
