#include <DApplication>
#include <DHiDPIHelper>
#include <QDebug>
#include <QMessageBox>
#include <QPushButton>

#include "service_name_sub_input_dialog.h"
#include "utils.h"

// define max service name limit at 30 charactor
#define MAX_SERVICE_NAME 30

ServiceNameSubInputDialog::ServiceNameSubInputDialog(DWidget *parent)
    : DDialog(parent)
{
    setIcon(QIcon::fromTheme("dialog-question"));

    addSpacing(10);
    m_nameLineEdit = new DLineEdit(this);
    Q_ASSERT(m_nameLineEdit);
    addContent(m_nameLineEdit);
    // set max service name
    m_nameLineEdit->lineEdit()->setMaxLength(MAX_SERVICE_NAME);

    m_okButtonTranslateName = DApplication::translate("Service.Instance.Name.Dialog", "OK");
    m_cancelButtonTranslateName = DApplication::translate("Service.Instance.Name.Dialog", "Cancel");
    addButton(m_okButtonTranslateName, true);
    addButton(m_cancelButtonTranslateName);
    // default the button OK is not enabled
    getButton(0)->setEnabled(false);

    connect(this, &ServiceNameSubInputDialog::buttonClicked, this,
            &ServiceNameSubInputDialog::onButtonClicked);

    // set alert when the text length reach the max length and set the button enable if the text length is not zero
    connect(m_nameLineEdit, &DLineEdit::textChanged, this, [=]() {
        if (m_nameLineEdit->text().length() >= MAX_SERVICE_NAME) {
            m_nameLineEdit->setAlert(true);
        }
        auto okBtn = dynamic_cast<QPushButton*>(getButton(getButtonIndexByText(m_okButtonTranslateName)));
        if (m_nameLineEdit->text().length() == 0) {
            okBtn->setEnabled(false);
            m_nameLineEdit->setAlert(false);
        } else {
            okBtn->setEnabled(true);
        }
    });
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
