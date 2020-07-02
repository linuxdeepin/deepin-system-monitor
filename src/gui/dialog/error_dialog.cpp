#include <QApplication>
#include <QMap>
#include <QDebug>

#include <DApplicationHelper>
#include <DFontSizeManager>

#include "error_dialog.h"
#include "gui/ui_common.h"

void ErrorDialog::show(QWidget *parent, const QString &errMsg, const QString &detail)
{
    auto *dlg = new ErrorDialog(errMsg, detail, parent);
    dlg->exec();
}

void ErrorDialog::initUI()
{
    setIcon(QIcon::fromTheme("dialog-warning"));
    setAttribute(Qt::WA_DeleteOnClose);

    m_detailLabel = new DLabel(this);
    auto pa = DApplicationHelper::instance()->palette(m_detailLabel);
    auto color = pa.color(DPalette::TextTips);
    pa.setColor(DPalette::Foreground, color);
    m_detailLabel->setPalette(pa);
    DFontSizeManager::instance()->bind(m_detailLabel, DFontSizeManager::T6, QFont::Medium);
    m_detailLabel->setMaximumWidth(360);
    m_detailLabel->setMinimumHeight(m_detailLabel->height() + 6);
    m_detailLabel->setWordWrap(true);

    setMessage(m_errMsg);
    m_detailLabel->setText(m_detail);

    addSpacing(5);

    addContent(m_detailLabel, Qt::AlignHCenter);
    addButton(QApplication::translate("Error.Dialog", "OK"), true);
}

ErrorDialog::ErrorDialog(const QString &errMsg, const QString &detail, QWidget *parent)
    : DDialog(parent), m_errMsg(errMsg), m_detail(detail)
{
    initUI();
}
