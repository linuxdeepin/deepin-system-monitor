#include <QVBoxLayout>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>

#include <DApplication>
#include <DApplicationHelper>
#include <DFontSizeManager>

#include "hw_info_view.h"

HWInfoView::HWInfoView(QWidget *parent)
    : DFrame(parent)
{
    initUI();
    initConnections();

    installEventFilter(this);
}

bool HWInfoView::eventFilter(QObject *obj, QEvent *event)
{
    return DFrame::eventFilter(obj, event);
}

void HWInfoView::initUI()
{
    auto *style = dynamic_cast<DStyle *>(DApplication::style());
    auto margin = style->pixelMetric(DStyle::PM_ContentsMargins);
    auto spacing = style->pixelMetric(DStyle::PM_ContentsSpacing);

    setAutoFillBackground(false);
    setFrameRounded(true);
    setLineWidth(0);

    setContentsMargins(0, 0, 0, 0);

    m_hwButton = new DPushButton(DApplication::translate("System.Info.Dialog", "Device Specs"));
    m_tipLabel = new DLabel(DApplication::translate("System.Info.Dialog",
                                                    "Use Device Manager to view the information of "
                                                    "all hardware devices installed on this computer."));
    m_tipLabel->setWordWrap(true);
    DFontSizeManager::instance()->bind(m_tipLabel, DFontSizeManager::T6, QFont::Medium);
    auto pa = DApplicationHelper::instance()->palette(m_tipLabel);
    pa.setColor(DPalette::Text, pa.color(DPalette::TextTips));
    m_tipLabel->setPalette(pa);
    m_tipLabel->adjustSize();

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(spacing);
    layout->setContentsMargins(margin * 4, margin * 2, margin * 4, margin * 2);

    layout->addWidget(m_hwButton);
    layout->addWidget(m_tipLabel);

    layout->addStretch(1);
    setLayout(layout);
}

void HWInfoView::initConnections()
{
    connect(m_hwButton, &QAbstractButton::clicked, this, [ = ]() {
        // check if deepin-devicemanager been installed
#define PATH_EXEC_DEVMGR    "/usr/bin/deepin-devicemanager"
        if (!QFileInfo::exists(PATH_EXEC_DEVMGR)) {
            auto title = QApplication::translate("System.Info.Dialog", "Device Manager binary not found");
            auto text = QApplication::translate("System.Info.Dialog", "Device Manager app not installed or corrupt. Please (re)install the app from App Store or with apt tools");
            QMessageBox::information(this, title, text);
            return;
        } else {
            QProcess::startDetached(PATH_EXEC_DEVMGR);
        }
    });
}
