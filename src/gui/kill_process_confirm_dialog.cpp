#include <DApplication>
#include <DHiDPIHelper>
#include <QMessageBox>

#include "kill_process_confirm_dialog.h"
#include "utils.h"

KillProcessConfirmDialog::KillProcessConfirmDialog(QWidget *parent)
    : DDialog(parent)
{
    // TODO: change icon
    setIcon(DHiDPIHelper::loadNxPixmap(Utils::getQrcPath("logo.svg")), QSize(32, 32));
    //    setIcon(QIcon::fromTheme("deepin-system-monitor.svg"));

    connect(this, &KillProcessConfirmDialog::buttonClicked, this,
            &KillProcessConfirmDialog::onButtonClicked);
}

void KillProcessConfirmDialog::onButtonClicked(int index, const QString &text)
{
    Q_UNUSED(text);

    if (index == 1) {
        setResult(QMessageBox::Ok);
    } else {
        setResult(QMessageBox::Cancel);
    }
}
