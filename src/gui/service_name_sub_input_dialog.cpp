#include <DApplication>
#include <DHiDPIHelper>
#include <QDebug>
#include <QMessageBox>

#include "service_name_sub_input_dialog.h"
#include "utils.h"

ServiceNameSubInputDialog::ServiceNameSubInputDialog(DWidget *parent)
    : DDialog(parent)
{
    //    setFixedSize(600, 300);
    // TODO: change icon
    setIconPixmap(DHiDPIHelper::loadNxPixmap(Utils::getQrcPath("logo.svg")));

    m_nameLineEdit = new DLineEdit(this);
    Q_ASSERT(m_nameLineEdit);
    addContent(m_nameLineEdit);

    addButton(DApplication::translate("Service.Instance.Name.Dialog", "OK"), true);
    addButton(DApplication::translate("Service.Instance.Name.Dialog", "Cancel"));

    connect(this, &ServiceNameSubInputDialog::buttonClicked, this,
            &ServiceNameSubInputDialog::onButtonCliecked);

    setFocus(Qt::PopupFocusReason);
    setFocusProxy(m_nameLineEdit);
}

void ServiceNameSubInputDialog::onButtonCliecked(int index, const QString &text)
{
    qDebug() << "Button clicked:" << text;
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
    m_nameLineEdit->setFocus();
}
