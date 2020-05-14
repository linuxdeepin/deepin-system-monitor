#ifndef SYS_INFO_VIEW_H
#define SYS_INFO_VIEW_H

#include <DFrame>
#include <DScrollArea>
#include <DLabel>

#include "gui/control/property_label.h"
#include "model/system_info_model.h"

DWIDGET_USE_NAMESPACE

class SysInfoView : public DFrame
{
    Q_OBJECT

public:
    SysInfoView(QWidget *parent = nullptr);

    bool eventFilter(QObject *obj, QEvent *event);

    inline void setModel(QSharedPointer<SystemInfoModel> model)
    {
        if (model) {
            m_model = model;
            Q_EMIT modelReset();
        }
    }

protected:
    void initUI();
    void initConnections();

private:
    void reset();

Q_SIGNALS:
    void modelReset();

private:
    DScrollArea *m_scroll       {};
    QWidget     *m_scrollarea   {};

    QSharedPointer<SystemInfoModel> m_model    {};

    PropertyLabel   *m_hostName         {};
    DLabel          *m_hostValue        {};
    PropertyLabel   *m_versionName      {};
    DLabel          *m_versionValue     {};
    PropertyLabel   *m_typeName         {};
    DLabel          *m_typeValue        {};
    PropertyLabel   *m_kernelName       {};
    DLabel          *m_kernelValue      {};
    PropertyLabel   *m_processorName    {};
    DLabel          *m_proccessorValue  {};
    PropertyLabel   *m_memoryName       {};
    DLabel          *m_memoryValue      {};
    PropertyLabel   *m_diskName         {};
    DLabel          *m_diskValue        {};
};

#endif // SYS_INFO_VIEW_H
