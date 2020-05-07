#include <QResizeEvent>
#include <QVBoxLayout>
#include <DApplication>
#include <DApplicationHelper>
#include <DLineEdit>
#include <QDebug>
#include <DPushButton>

#include "sys_config_view.h"
#include "system/system_ctl.h"

SysConfigView::SysConfigView(QWidget *parent)
    : DFrame(parent)
{
    initUI();
    initConnections();

    installEventFilter(this);
    m_scroll->installEventFilter(this);
    m_scrollarea->installEventFilter(this);
}

bool SysConfigView::eventFilter(QObject *obj, QEvent *event)
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

void SysConfigView::initUI()
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
    contentLayout->setSpacing(spacing);
    setLayout(contentLayout);

    m_scroll = new DScrollArea();
    m_scroll->setContentsMargins(0, 0, 0, 0);
    m_scroll->setLineWidth(0);
    m_scroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    contentLayout->addWidget(m_scroll, 1);

    m_scrollarea = new QWidget();
    m_scrollarea->setContentsMargins(0, 0, 0, 0);
    m_scroll->setWidget(m_scrollarea);

    auto *vlayout = new QVBoxLayout(m_scroll);
    m_scrollarea->setLayout(vlayout);

    vlayout->setSpacing(spacing);
    vlayout->setMargin(0);

    m_enableDropCacheOption = new DCheckBox(DApplication::translate("System.Info.Dialog", "Enable \"Clear system cache\" option"));
    m_enableDropCacheOption->setChecked(true);
    vlayout->addWidget(m_enableDropCacheOption);

    m_host = new PropertyLabel();
    m_host->setPropertyName(DApplication::translate("System.Info.Dialog", "Computer name"));
    m_host->setMinimumHeight(56);
    m_hostEdit = new DLineEdit();
    m_hostEdit->setMinimumWidth(210);
    m_host->addPropertyValueWidget(m_hostEdit, DPalette::Text);
    vlayout->addWidget(m_host);

    m_logLevel = new PropertyLabel();
    m_logLevel->setMinimumHeight(56);
    m_logLevel->setPropertyName(DApplication::translate("System.Info.Dialog", "Kernel log level"));
    m_logLevelCmbo = new DComboBox();
    m_logLevelCmbo->setFocusPolicy(Qt::TabFocus);
    m_logLevelCmbo->setMinimumWidth(210);
    m_logLevelCmbo->setMinimumHeight(36);
    m_logLevelCmbo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_logLevel->addPropertyValueWidget(m_logLevelCmbo, DPalette::Text);
    vlayout->addWidget(m_logLevel);

    m_dropCache = new PropertyLabel();
    m_dropCache->setMinimumHeight(56);
    m_dropCache->setPropertyName(DApplication::translate("System.Info.Dialog", "Clear system cache"));
    m_dropCacheCmbo = new DComboBox();
    m_dropCacheCmbo->setMinimumWidth(210);
    m_dropCacheCmbo->setMinimumHeight(36);
    m_dropCacheCmbo->setFocusPolicy(Qt::TabFocus);
    m_dropCacheCmbo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_dropCache->addPropertyValueWidget(m_dropCacheCmbo, DPalette::Text);
    vlayout->addWidget(m_dropCache);

    m_enableLaptopMode = new PropertyLabel();
    m_enableLaptopMode->setMinimumHeight(56);
    m_enableLaptopMode->setPropertyName(DApplication::translate("System.Info.Dialog", "Laptop mode"));
    m_enableLaptopModeSwch = new DSwitchButton();
    m_enableLaptopModeSwch->setFocusPolicy(Qt::TabFocus);
    m_enableLaptopMode->addPropertyValueWidget(m_enableLaptopModeSwch, DPalette::Text);
    vlayout->addWidget(m_enableLaptopMode);

    m_enableCoreDump = new PropertyLabel();
    m_enableCoreDump->setMinimumHeight(56);
    m_enableCoreDump->setPropertyName(DApplication::translate("System.Info.Dialog", "Enable core dump files for user-mode processes"));
    m_enableCoreDumpSwch = new DSwitchButton();
    m_enableCoreDumpSwch->setFocusPolicy(Qt::TabFocus);
    m_enableCoreDump->addPropertyValueWidget(m_enableCoreDumpSwch, DPalette::Text);
    vlayout->addWidget(m_enableCoreDump);

    vlayout->addStretch(1);

    // button
    auto *btnLayout = new QHBoxLayout();
    btnLayout->setMargin(0);
    btnLayout->setSpacing(spacing);
    m_cancel = new DPushButton(DApplication::translate("System.Info.Dialog", "Cancel"));
    m_commit = new DPushButton(DApplication::translate("System.Info.Dialog", "Save"));
    btnLayout->addWidget(m_cancel);
    btnLayout->addWidget(m_commit);
    contentLayout->addLayout(btnLayout, 0);
}

void SysConfigView::initConnections()
{
    connect(this, &SysConfigView::modelReset, this, &SysConfigView::reset);
    connect(SystemCtl::instance(), &SystemCtl::systemInfoModelReady, this, &SysConfigView::setModel);
}

void SysConfigView::reset()
{
    if (m_model) {
        m_hostEdit->setText(m_model->getHostName());
        m_logLevelCmbo->setModel(m_model->getKernelLogLevelModel());
        m_logLevelCmbo->setCurrentIndex(m_model->getKernelLogLevel());
        m_dropCacheCmbo->setModel(m_model->getDropCacheModel());
        m_dropCacheCmbo->setCurrentIndex(m_model->getClearCacheDepth());
        m_enableLaptopModeSwch->setChecked(m_model->laptopModeEnabled());
        m_enableCoreDumpSwch->setChecked(m_model->coreDumpEnabled());
    }
}
