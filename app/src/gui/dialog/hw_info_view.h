#ifndef HW_INFO_VIEW_H
#define HW_INFO_VIEW_H

#include <DFrame>
#include <DLabel>
#include <DPushButton>

DWIDGET_USE_NAMESPACE

class HWInfoView : public DFrame
{
    Q_OBJECT

public:
    HWInfoView(QWidget *parent = nullptr);

    bool eventFilter(QObject *obj, QEvent *event);

public Q_SLOTS:
    void clearFocus();

protected:
    void initUI();
    void initConnections();

private:
    DPushButton *m_hwButton     {};
    DLabel      *m_tipLabel     {};
};

#endif // HW_INFO_VIEW_H
