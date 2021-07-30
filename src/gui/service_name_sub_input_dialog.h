#ifndef SERVICE_NAME_SUB_INPUT_DIALOG_H
#define SERVICE_NAME_SUB_INPUT_DIALOG_H

#include <DDialog>
#include <DLineEdit>
#include <DWidget>

DWIDGET_USE_NAMESPACE

class ServiceNameSubInputDialog : public DDialog
{
    Q_OBJECT

public:
    ServiceNameSubInputDialog(DWidget *parent = nullptr);

    QString getServiceSubName() const { return m_name; }
    void setServiceSubName(const QString &name) { m_name = name; }

    int result() const { return m_result; }
    void setResult(int r) { m_result = r; }

public Q_SLOTS:
    void onButtonClicked(int index, const QString &text);

protected:
    void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;

private:
    QString m_name {};
    int m_result {0};
    DLineEdit *m_nameLineEdit;

    //确认按钮翻译名称
    QString m_okButtonTranslateName {};
    //取消按钮翻译名称
    QString m_cancelButtonTranslateName {};
};

#endif  // SERVICE_NAME_SUB_INPUT_DIALOG_H
