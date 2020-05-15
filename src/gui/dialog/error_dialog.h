#ifndef ERROR_DIALOG_H
#define ERROR_DIALOG_H

#include <DDialog>
#include <DLabel>

DWIDGET_USE_NAMESPACE

class ErrorDialog : public DDialog
{
    Q_OBJECT

public:
    static void show(QWidget *parent, const QString &errMsg, const QString &detail);

private:
    ErrorDialog(const QString &errMsg, const QString &detail, QWidget *parent = nullptr);
    void initUI();

private:
    QString m_errMsg        {};
    QString m_detail        {};
    DLabel *m_detailLabel   {};
};

#endif // ERROR_DIALOG_H
