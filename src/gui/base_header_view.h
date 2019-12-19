#ifndef BASE_HEADER_VIEW_H
#define BASE_HEADER_VIEW_H

#include <DHeaderView>
#include <DStyle>

DWIDGET_USE_NAMESPACE

class BaseHeaderView : public DHeaderView
{
    Q_OBJECT

public:
    explicit BaseHeaderView(Qt::Orientation orientation, QWidget *parent = nullptr);

    QSize sizeHint() const override;
    int sectionSizeHint(int logicalIndex) const;

    inline int getSpacing() const { return m_spacing; }
    inline void setSpacing(int spacing) { m_spacing = spacing; }

protected:
    void paintEvent(QPaintEvent *e) override;
    virtual void paintSection(QPainter *painter, const QRect &rect,
                              int logicalIndex) const override;

private:
    int m_spacing {1};
};

#endif  // BASE_HEADER_VIEW_H
