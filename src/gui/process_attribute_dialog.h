#ifndef PROCESS_ATTRIBUTE_DIALOG_H
#define PROCESS_ATTRIBUTE_DIALOG_H

#include <DFrame>
#include <DLabel>
#include <DMainWindow>
#include <DShadowLine>
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

    DWidget *m_frame {nullptr};
    DShadowLine *m_tbShadow {nullptr};

    DLabel *m_procNameLabel {nullptr};
    DLabel *m_procNameText {nullptr};
    DLabel *m_procCmdLabel {nullptr};
    DLabel *m_procCmdText {nullptr};
    DLabel *m_procStartLabel {nullptr};
    DLabel *m_procStartText {nullptr};

    int m_maxLabelWidth {0};
    int m_maxTextWidth {0};
    int m_margin {0};

    pid_t m_pid;
};

#endif  // PROCESS_ATTRIBUTE_DIALOG_H
