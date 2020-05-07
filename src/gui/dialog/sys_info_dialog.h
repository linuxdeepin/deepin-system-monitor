#ifndef SYS_INFO_DIALOG_H
#define SYS_INFO_DIALOG_H

#include <DMainWindow>
#include <DListView>
#include <DStackedWidget>
#include <DShadowLine>

#include "sys_info_view.h"
#include "hw_info_view.h"
#include "sys_config_view.h"

DWIDGET_USE_NAMESPACE

class SysInfoDialog : public DMainWindow
{
    Q_OBJECT

public:
    enum ActionType {
        kShowSysInfo,
        kShowHWInfo,
        kChangeSysCfg
    };
    Q_ENUM(ActionType)

    const int kValueRole = Qt::UserRole + 1;

public:
    SysInfoDialog(QWidget *parent = nullptr);

    bool eventFilter(QObject *obj, QEvent *event) override;

protected:
    void initUI();
    void initConnections();

    virtual void resizeEvent(QResizeEvent *event) override;
    virtual void showEvent(QShowEvent *event) override;

private:
    void populateCmdList();

private:
    DListView       *m_cmdListView      {};
    DStackedWidget  *m_views            {};
    SysInfoView     *m_sysInfoView      {};
    HWInfoView      *m_hwInfoView       {};
    SysConfigView   *m_sysCfgView       {};

    DShadowLine     *m_shadow           {};
};

#endif // SYS_INFO_DIALOG_H
