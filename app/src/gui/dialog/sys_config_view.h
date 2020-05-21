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
#include <DSuggestButton>
#include <QStandardItem>
#include <DSpinner>

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

    inline bool contentModified()
    {
        return (m_hostEdit->text() != m_model->getHostName()
                || m_logLevelCmbo->currentIndex() != m_model->getKernelLogLevel()
                || m_dropCacheCmbo->currentIndex() != m_model->getClearCacheDepth()
                || m_enableSplashModeSwch->isChecked() != m_model->splashModeEnabled()
                || m_enableCoreDumpSwch->isChecked() != m_model->coreDumpEnabled());
    }

Q_SIGNALS:
    void modelReset();
    void editCancelled();
    void progressChanged(bool running = false);

protected:
    void initUI();
    void initConnections();

private:
    void reset();
    bool validateHostname(const QString host);

private:
    DScrollArea *m_scroll       {};
    QWidget     *m_scrollarea   {};

    QSharedPointer<SystemInfoModel> m_model     {};

    PropertyLabel   *m_host                     {};
    DLineEdit       *m_hostEdit                 {};
    PropertyLabel   *m_logLevel                 {};
    DComboBox       *m_logLevelCmbo             {};
    PropertyLabel   *m_dropCache                {};
    DComboBox       *m_dropCacheCmbo            {};
    PropertyLabel   *m_enableSplashMode         {};
    DSwitchButton   *m_enableSplashModeSwch     {};
    PropertyLabel   *m_enableCoreDump           {};
    DSwitchButton   *m_enableCoreDumpSwch       {};

    DPushButton     *m_cancel                   {};
    DSuggestButton  *m_commit                   {};

    bool            m_onProgress                {false};
};

#endif // SYS_CONFIG_VIEW_H
