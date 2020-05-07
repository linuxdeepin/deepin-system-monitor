#ifndef SYS_CONFIG_VIEW_H
#define SYS_CONFIG_VIEW_H

#include <DFrame>
#include <DScrollArea>
#include <DLabel>
#include <DCheckBox>
#include <DComboBox>
#include <DLineEdit>
#include <DSwitchButton>
#include <DPushButton>
#include <QStandardItem>

#include "gui/control/property_label.h"
#include "model/system_info_model.h"

DWIDGET_USE_NAMESPACE

class SysConfigView : public DFrame
{
    Q_OBJECT

public:
    SysConfigView(QWidget *parent = nullptr);

    bool eventFilter(QObject *obj, QEvent *event);

    inline void setModel(QSharedPointer<SystemInfoModel> model)
    {
        if (model) {
            m_model = model;
            Q_EMIT modelReset();
        }
    }

Q_SIGNALS:
    void modelReset();

protected:
    void initUI();
    void initConnections();

private:
    void reset();

private:
    DScrollArea *m_scroll       {};
    QWidget     *m_scrollarea   {};

    QSharedPointer<SystemInfoModel> m_model     {};

    DCheckBox       *m_enableDropCacheOption    {};
    PropertyLabel   *m_host                     {};
    DLineEdit       *m_hostEdit                 {};
    PropertyLabel   *m_logLevel                 {};
    DComboBox       *m_logLevelCmbo             {};
    PropertyLabel   *m_dropCache                {};
    DComboBox       *m_dropCacheCmbo            {};
    PropertyLabel   *m_enableLaptopMode         {};
    DSwitchButton   *m_enableLaptopModeSwch     {};
    PropertyLabel   *m_enableCoreDump           {};
    DSwitchButton   *m_enableCoreDumpSwch       {};

    DPushButton     *m_cancel                   {};
    DPushButton     *m_commit                   {};
};

#endif // SYS_CONFIG_VIEW_H
