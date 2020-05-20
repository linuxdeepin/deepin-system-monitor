#include <QEvent>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <DApplication>
#include <DApplicationHelper>
#include <DLineEdit>
#include <QDebug>
#include <DPushButton>

#include "sys_info_view.h"
#include "utils.h"
#include "system/system_ctl.h"

SysInfoView::SysInfoView(QWidget *parent)
    : DFrame(parent)
{
    initUI();
    initConnections();

    installEventFilter(this);
    m_scroll->installEventFilter(this);
    m_scrollarea->installEventFilter(this);
}

void SysInfoView::initUI()
{
    auto *style = dynamic_cast<DStyle *>(DApplication::style());
    auto margin = style->pixelMetric(DStyle::PM_ContentsMargins);
    auto spacing = style->pixelMetric(DStyle::PM_ContentsSpacing);

    setAutoFillBackground(false);
    setFrameRounded(true);
    setLineWidth(0);

    setContentsMargins(0, 0, 0, 0);
    auto *contentLayout = new QVBoxLayout(this);
    contentLayout->setMargin(margin);
    contentLayout->setSpacing(0);
    setLayout(contentLayout);

    m_scroll = new DScrollArea();
    m_scroll->setContentsMargins(0, 0, 0, 0);
    m_scroll->setLineWidth(0);
    m_scroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    contentLayout->addWidget(m_scroll, 0);

    m_scrollarea = new QWidget();
    m_scrollarea->setContentsMargins(0, 0, 0, 0);
    m_scroll->setWidget(m_scrollarea);

    auto *vlayout = new QVBoxLayout(m_scroll);
    m_scrollarea->setLayout(vlayout);

    vlayout->setSpacing(spacing);
    vlayout->setMargin(0);

    // host name
    m_hostName = new PropertyLabel();
    m_hostName->setPropertyName(DApplication::translate("System.Info.Dialog", "Computer name"));
    m_hostName->setMinimumHeight(36);
    m_hostValue = new DLabel();
    m_hostValue->setElideMode(Qt::ElideMiddle);
    m_hostName->addPropertyValueWidget(m_hostValue, DPalette::TextTips);
    vlayout->addWidget(m_hostName);

    // edition
    m_versionName = new PropertyLabel();
    m_versionName->setPropertyName(DApplication::translate("System.Info.Dialog", "Edition"));
    m_versionValue = new DLabel();
    m_versionName->addPropertyValueWidget(m_versionValue, DPalette::TextTips);
    vlayout->addWidget(m_versionName);

    // type
    m_typeName = new PropertyLabel();
    m_typeName->setPropertyName(DApplication::translate("System.Info.Dialog", "Type"));
    m_typeValue = new DLabel();
    m_typeName->addPropertyValueWidget(m_typeValue, DPalette::TextTips);
    vlayout->addWidget(m_typeName);

    // kernel
    m_kernelName = new PropertyLabel();
    m_kernelName->setPropertyName(DApplication::translate("System.Info.Dialog", "Kernel"));
    m_kernelValue = new DLabel();
    m_kernelValue->setElideMode(Qt::ElideMiddle);
    m_kernelName->addPropertyValueWidget(m_kernelValue, DPalette::TextTips);
    vlayout->addWidget(m_kernelName);

    // processor
    m_processorName = new PropertyLabel();
    m_processorName->setPropertyName(DApplication::translate("System.Info.Dialog", "Processor"));
    m_proccessorValue = new DLabel();
    m_proccessorValue->setElideMode(Qt::ElideMiddle);
    m_processorName->addPropertyValueWidget(m_proccessorValue, DPalette::TextTips);
    vlayout->addWidget(m_processorName);

    // memory
    m_memoryName = new PropertyLabel();
    m_memoryName->setPropertyName(DApplication::translate("System.Info.Dialog", "Memory"));
    m_memoryValue = new DLabel();
    m_memoryName->addPropertyValueWidget(m_memoryValue, DPalette::TextTips);
    vlayout->addWidget(m_memoryName);

    // disk
    m_diskName = new PropertyLabel();
    m_diskName->setPropertyName(DApplication::translate("System.Info.Dialog", "Disk"));
    m_diskValue = new DLabel();
    m_diskName->addPropertyValueWidget(m_diskValue, DPalette::TextTips);
    vlayout->addWidget(m_diskName);

    vlayout->addStretch(1);
}

void SysInfoView::initConnections()
{
    connect(this, &SysInfoView::modelReset, this, &SysInfoView::reset);
    connect(SystemCtl::instance(), &SystemCtl::sysInfoModelUpdated, this, &SysInfoView::setModel);
}

void SysInfoView::reset()
{
    if (m_model) {
        m_hostValue->setText(m_model->getHostName());
        m_hostValue->setToolTip(m_model->getHostName());
        m_versionValue->setText(m_model->getOSVersion());
        m_versionValue->setToolTip(m_model->getOSVersion());
        m_typeValue->setText(m_model->getOSType());
        m_typeValue->setToolTip(m_model->getOSType());
        m_kernelValue->setText(m_model->getKernelVersion());
        m_kernelValue->setToolTip(m_model->getKernelVersion());
        m_proccessorValue->setText(m_model->getCPUModel());
        m_proccessorValue->setToolTip(m_model->getCPUModel());
        m_memoryValue->setText(DApplication::translate("System.Info.Dialog", "%1 (%2 available)")
                               .arg(m_model->getInstalledMemory())
                               .arg(m_model->getAvailMemory()));
        m_memoryValue->setToolTip(m_memoryValue->text());
        m_diskValue->setText(m_model->getDiskSize());
        m_diskValue->setToolTip(m_model->getDiskSize());

        connect(m_model.data(), &SystemInfoModel::hostNameChanged, this, [ = ](const QString & hostName) {
            m_hostValue->setText(hostName);
        });
    }
}

bool SysInfoView::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Resize) {
        auto *rev = dynamic_cast<QResizeEvent *>(event);
        if (obj == this) {
            m_scroll->resize(rev->size());
            auto margin_sum = (layout()->margin() + m_scrollarea->layout()->margin()) * 2 + 2;
            m_scrollarea->resize(m_scroll->size().width() - margin_sum,
                                 m_scroll->size().height() - margin_sum);
        }
    }
    return DFrame::eventFilter(obj, event);
}
