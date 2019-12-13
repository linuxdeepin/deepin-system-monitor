#include <DApplication>
#include <DHiDPIHelper>
#include <QDebug>
#include <QMessageBox>

#include "service_name_sub_input_dialog.h"
#include "utils.h"

ServiceNameSubInputDialog::ServiceNameSubInputDialog(DWidget *parent)
    : DDialog(parent)
{
    setIcon(QIcon::fromTheme("deepin-system-monitor"));

    addSpacing(10);
    m_nameLineEdit = new DLineEdit(this);
    Q_ASSERT(m_nameLineEdit);
    addContent(m_nameLineEdit);

    addButton(DApplication::translate("Service.Instance.Name.Dialog", "OK"), true);
    addButton(DApplication::translate("Service.Instance.Name.Dialog", "Cancel"));

    connect(this, &ServiceNameSubInputDialog::buttonClicked, this,
            &ServiceNameSubInputDialog::onButtonClicked);
}

void ServiceNameSubInputDialog::onButtonClicked(int index, const QString &text)
{
    Q_UNUSED(text);

    if (index == 0) {
        m_name = m_nameLineEdit->text();
        setResult(QMessageBox::Ok);
    } else {
        setResult(QMessageBox::Cancel);
    }
}

void ServiceNameSubInputDialog::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);
    m_nameLineEdit->lineEdit()->setFocus();
}
