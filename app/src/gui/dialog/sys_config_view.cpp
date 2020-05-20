#include <QDebug>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QProcess>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMessageBox>
#include <QToolTip>

#include <DApplication>
#include <DApplicationHelper>
#include <DLineEdit>
#include <DPushButton>
#include <DListView>

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

    m_enableSplashMode = new PropertyLabel();
    m_enableSplashMode->setMinimumHeight(56);
    m_enableSplashMode->setPropertyName(DApplication::translate("System.Info.Dialog", "Enable splash mode"));
    m_enableSplashModeSwch = new DSwitchButton();
    m_enableSplashModeSwch->setFocusPolicy(Qt::TabFocus);
    m_enableSplashMode->addPropertyValueWidget(m_enableSplashModeSwch, DPalette::Text);
    vlayout->addWidget(m_enableSplashMode);

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
    m_cancel->setEnabled(false);
    m_commit = new DPushButton(DApplication::translate("System.Info.Dialog", "Save"));
    m_commit->setEnabled(false);
    auto pa = DApplicationHelper::instance()->palette(m_commit);
    pa.setBrush(DPalette::Normal, DPalette::Light, pa.color(DPalette::Normal, DPalette::LightLively));
    pa.setBrush(DPalette::Normal, DPalette::Dark, pa.color(DPalette::Normal, DPalette::DarkLively));
    pa.setBrush(DPalette::Normal, DPalette::ButtonText, pa.color(DPalette::Normal, DPalette::TextLively));
    m_commit->setPalette(pa);
    btnLayout->addWidget(m_cancel);
    btnLayout->addWidget(m_commit);
    contentLayout->addLayout(btnLayout, 0);
}

void SysConfigView::initConnections()
{
    connect(this, &SysConfigView::modelReset, this, &SysConfigView::reset);
    connect(SystemCtl::instance(), &SystemCtl::sysInfoModelUpdated, this, &SysConfigView::setModel);
    connect(m_cancel, &QAbstractButton::clicked, this, [ = ]() {
        Q_EMIT editCancelled();
    });
    connect(this, &SysConfigView::progressChanged, this, [ = ](bool running) {
        if (running) {
            m_hostEdit->setEnabled(false);
            m_logLevelCmbo->setEnabled(false);
            m_dropCacheCmbo->setEnabled(false);
            m_enableSplashModeSwch->setEnabled(false);
            m_enableCoreDumpSwch->setEnabled(false);

            m_cancel->setEnabled(false);
            m_commit->setEnabled(false);

            m_onProgress = true;
        } else {
            m_hostEdit->setEnabled(true);
            m_logLevelCmbo->setEnabled(true);
            m_dropCacheCmbo->setEnabled(true);
            m_enableSplashModeSwch->setEnabled(true);
            m_enableCoreDumpSwch->setEnabled(true);

            if (contentModified()) {
                m_cancel->setEnabled(true);
                m_commit->setEnabled(true);
            } else {
                m_cancel->setEnabled(false);
                m_commit->setEnabled(false);
            }

            m_onProgress = false;
        }
    });
    connect(m_commit, &QAbstractButton::clicked, this, [ = ]() {
        auto *proc = new QProcess();
        QJsonObject obj {};
        QStringList params;
        params << "/usr/bin/deepin-system-monitor-syscfg-tool";
        if (m_hostEdit->text() != m_model->getHostName()) {
            if (!validateHostname(m_hostEdit->text())) {
                // DLineEdit showAlertMessage kinda buggy right now!!
                auto pa = QToolTip::palette();
                auto color = DApplicationHelper::instance()->palette(this).color(DPalette::Current, DPalette::TextWarning);
                pa.setBrush(QPalette::ToolTipText, color);
                QToolTip::setPalette(pa);
                QToolTip::showText(mapToGlobal({m_hostEdit->x(), m_hostEdit->y() + m_hostEdit->height()}),
                                   DApplication::translate("System.Info.Dialog",
                                                           "Host name can only contain 2 ~ 63 characters, \n"
                                                           "can only contain alpha, numeric, '.' or '-', and \n"
                                                           "must starts and ends with alpha or numeric"));
                return;
            }
            obj.insert("Hostname", m_hostEdit->text());
        }
        if (m_logLevelCmbo->currentIndex() != m_model->getKernelLogLevel()) {
            obj.insert("KernelLogLevel", QString::number(m_logLevelCmbo->currentIndex()));
        }
        if (m_dropCacheCmbo->currentIndex() != m_model->getClearCacheDepth()) {
            obj.insert("DropCache", QString::number(m_dropCacheCmbo->currentIndex()));
        }
        if (m_enableSplashModeSwch->isChecked() != m_model->splashModeEnabled()) {
            obj.insert("EnableSplashMode", m_enableSplashModeSwch->isChecked());
        }
        if (m_enableCoreDumpSwch->isChecked() != m_model->coreDumpEnabled()) {
            auto title = DApplication::translate("System.Info.Dialog", "Core dump state changed");
            auto msg = DApplication::translate("System.Info.Dialog", "Core dump state change only take effect after relogin");
            QMessageBox::information(this, title, msg);
            obj.insert("EnableCoreDump", m_enableCoreDumpSwch->isChecked());
        }
        QJsonDocument doc(obj);
        params << doc.toBinaryData().toBase64();
        proc->start("/usr/bin/pkexec", params);
        Q_EMIT progressChanged(true);
        connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [ = ](int exitCode, QProcess::ExitStatus exitStatus) {
            Q_UNUSED(exitCode)
            Q_UNUSED(exitStatus);
            if (exitCode == 0) {
                SystemCtl::instance()->getSystemInfo();
            }
            Q_EMIT progressChanged(false);
            proc->deleteLater();
        });
    });


    connect(m_hostEdit, &DLineEdit::textChanged, this, [ = ](const QString & text) {
        if (text != m_model->getHostName()) {
            m_commit->setEnabled(true);
            m_cancel->setEnabled(true);
        } else {
            m_commit->setEnabled(!m_onProgress && contentModified());
            m_cancel->setEnabled(!m_onProgress && contentModified());
        }
    });
    connect(m_logLevelCmbo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [ = ](int index) {
        if (index != m_model->getKernelLogLevel()) {
            m_commit->setEnabled(true);
            m_cancel->setEnabled(true);
        } else {
            m_commit->setEnabled(!m_onProgress && contentModified());
            m_cancel->setEnabled(!m_onProgress && contentModified());
        }
    });
    connect(m_dropCacheCmbo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [ = ](int index) {
        if (index != m_model->getClearCacheDepth()) {
            m_commit->setEnabled(true);
            m_cancel->setEnabled(true);
        } else {
            m_commit->setEnabled(!m_onProgress && contentModified());
            m_cancel->setEnabled(!m_onProgress && contentModified());
        }
    });
    connect(m_enableSplashModeSwch, &DSwitchButton::checkedChanged, this, [ = ](bool checked) {
        if (checked != m_model->splashModeEnabled()) {
            m_commit->setEnabled(true);
            m_cancel->setEnabled(true);
        } else {
            m_commit->setEnabled(!m_onProgress && contentModified());
            m_cancel->setEnabled(!m_onProgress && contentModified());
        }
    });
    connect(m_enableCoreDumpSwch, &DSwitchButton::checkedChanged, this, [ = ](bool checked) {
        if (checked != m_model->coreDumpEnabled()) {
            m_commit->setEnabled(true);
            m_cancel->setEnabled(true);
        } else {
            m_commit->setEnabled(!m_onProgress && contentModified());
            m_cancel->setEnabled(!m_onProgress && contentModified());
        }
    });
    connect(this, &SysConfigView::editCancelled, this, [ = ]() {
        reset();
    });
}

void SysConfigView::reset()
{
    if (m_model) {
        m_hostEdit->setText(m_model->getHostName());
        m_logLevelCmbo->setModel(m_model->getKernelLogLevelModel());
        m_logLevelCmbo->setCurrentIndex(m_model->getKernelLogLevel());
        m_dropCacheCmbo->setModel(m_model->getDropCacheModel());
        m_dropCacheCmbo->setCurrentIndex(m_model->getClearCacheDepth());
        m_enableSplashModeSwch->setChecked(m_model->splashModeEnabled());
        m_enableCoreDumpSwch->setChecked(m_model->coreDumpEnabled());
        m_cancel->setEnabled(false);
        m_commit->setEnabled(false);
    }
}

bool SysConfigView::validateHostname(const QString host)
{
    if (host.length() < 2 || host.length() > 63) {
        return false;
    }
    QRegularExpression regex("^[\\d\\w]{1}[a-zA-Z0-9\\.\\-]*[\\d\\w]{1}$", QRegularExpression::CaseInsensitiveOption);
    auto match = regex.match(host);
    if (match.hasMatch()) {
        return true;
    }
    return false;
}
