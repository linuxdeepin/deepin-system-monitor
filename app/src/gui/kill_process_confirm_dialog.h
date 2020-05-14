#ifndef KILL_PROCESS_CONFIRM_DIALOG_H
#define KILL_PROCESS_CONFIRM_DIALOG_H

#include <DDialog>
#include <DWidget>

DWIDGET_USE_NAMESPACE

class KillProcessConfirmDialog : public DDialog
{
    Q_OBJECT

public:
    KillProcessConfirmDialog(QWidget *parent = nullptr);

    inline int result() const { return m_result; }
    inline void setResult(int r) { m_result = r; }

public Q_SLOTS:
    void onButtonClicked(int index, const QString &text);

private:
    int m_result {0};
};

#endif  // KILL_PROCESS_CONFIRM_DIALOG_H
