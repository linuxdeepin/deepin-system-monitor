#ifndef PROCESS_ATTRIBUTE_DIALOG_H
#define PROCESS_ATTRIBUTE_DIALOG_H

#include <DMainWindow>
#include <DWidget>

DWIDGET_USE_NAMESPACE

class Settings;
class QHBoxLayout;
class QVBoxLayout;
class QGridLayout;

class ProcessAttributeDialog : public DMainWindow
{
    Q_OBJECT

public:
    explicit ProcessAttributeDialog(pid_t pid, QWidget *parent = nullptr);

private:
    void initUI();

protected:
    void resizeEvent(QResizeEvent *event);
    void closeEvent(QCloseEvent *event);
    bool eventFilter(QObject *obj, QEvent *event);

private:
    Settings *m_settings;

    pid_t m_pid;
};

#endif  // PROCESS_ATTRIBUTE_DIALOG_H
