/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      maojj <maojunjie@uniontech.com>
* Maintainer:  maojj <maojunjie@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "process_attribute_dialog.h"

#include "settings.h"
#include "utils.h"

#include <DApplication>
#include <DApplicationHelper>
#include <DFontSizeManager>
#include <DFrame>
#include <DLabel>
#include <DTextBrowser>
#include <DTitlebar>

#include <QAbstractTextDocumentLayout>
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QTextBlock>
#include <QVBoxLayout>
#include <QtMath>
#include <QScrollBar>

// prefered text width
static const int kPreferedTextWidth = 200;
// default icon size
static const int kAppIconSize = 80;
// spacing between label & content
static const int kIconToNameSpace = 10;
// spacing between display name & grid layout
static const int kNameToGridSpace = 16;

// constructor
ProcessAttributeDialog::ProcessAttributeDialog(pid_t pid,
                                               const QString &procName,
                                               const QString &displayName,
                                               const QString &cmdline,
                                               const QIcon &icon,
                                               time_t startTime,
                                               QWidget *parent)
    : DMainWindow(parent)
    , m_procName(procName)
    , m_displayName(displayName)
    , m_cmdline(cmdline)
    , m_startTime(startTime)
    , m_icon(icon)
    , m_pid(pid)
{
    // get backup settings instance
    m_settings = Settings::instance();

    // initialize ui components
    initUI();

    // install event filters for dialog itself & text contents
    installEventFilter(this);
    if (m_procNameLabel) {
        m_procNameLabel->installEventFilter(this);
    }
    if (m_procNameText) {
        m_procNameText->installEventFilter(this);
    }
    if (m_procCmdLabel) {
        m_procCmdLabel->installEventFilter(this);
    }
    if (m_procCmdText) {
        m_procCmdText->installEventFilter(this);
    }
    if (m_procStartLabel) {
        m_procStartLabel->installEventFilter(this);
    }
    if (m_procStartText) {
        m_procStartText->installEventFilter(this);
    }
}

// initialize ui components
void ProcessAttributeDialog::initUI()
{
    // global DStyle instance
    auto *style = dynamic_cast<DStyle *>(DApplication::style());
    //  global app helper instance
    auto *dAppHelper = DApplicationHelper::instance();
    // global palette
    auto palette = dAppHelper->applicationPalette();

    // initialize style option
    QStyleOption option;
    option.initFrom(this);

    // content margin
    m_margin = style->pixelMetric(DStyle::PM_ContentsMargins, &option);

    // default width & height
    int width = 600, height = 480;
    // restore settings from backup config if previous size found, otherwise use default size
    if (m_settings) {
        QVariant v;
        v = m_settings->getOption(kSettingKeyProcessAttributeDialogWidth);
        if (v.isValid()) {
            width = v.toInt();
        }
        v = m_settings->getOption(kSettingKeyProcessAttributeDialogHeight);
        if (v.isValid()) {
            height = v.toInt();
        }
    }

    // background frame
    m_frame = new DWidget(this);
    m_frame->setAutoFillBackground(true);

    // resize dialog
    resize(width, height);
    // set minimum size
    setMinimumSize(600, 480);
    // set window attributes
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowModality(Qt::WindowModal);
    setWindowFlags(Qt::Dialog);
    // hide menu on titlebar
    titlebar()->setQuitMenuDisabled(true);
    titlebar()->setMenuVisible(false);
    setTitlebarShadowEnabled(false);
    // set dialog title
    titlebar()->setTitle({});

    // shadow bar instance
    m_tbShadow = new DShadowLine(m_frame);
    m_tbShadow->setFixedWidth(m_frame->width());
    m_tbShadow->setFixedHeight(10);
    m_tbShadow->move(0, 0);
    m_tbShadow->raise();
    m_tbShadow->show();

    // frame layout
    auto *vlayout = new QVBoxLayout(m_frame);
    vlayout->setContentsMargins(m_margin, 0, m_margin, 0);
    vlayout->setSpacing(m_margin);

    vlayout->addStretch(1);

    DLabel *appIcon = new DLabel(this);
    vlayout->addWidget(appIcon, 0, Qt::AlignVCenter | Qt::AlignHCenter);

    // spacing between icon & display name
    vlayout->addSpacing(kIconToNameSpace);

    // process display name label
    m_appNameLabel = new DLabel(this);
    DFontSizeManager::instance()->bind(m_appNameLabel, DFontSizeManager::T5, QFont::Bold);
    // elide display name text in the middle
    m_appNameLabel->setElideMode(Qt::ElideMiddle);
    m_appNameLabel->setAlignment(Qt::AlignCenter);
    vlayout->addWidget(m_appNameLabel, 0, Qt::AlignHCenter | Qt::AlignVCenter);

    // spacing
    vlayout->addSpacing(kNameToGridSpace);

    // background frame
    auto *wnd = new QWidget(m_frame);
    wnd->setAutoFillBackground(false);
    // content grid layout
    auto *grid = new QGridLayout(wnd);
    grid->setHorizontalSpacing(m_margin);
    grid->setVerticalSpacing(0);

    QString buf;
    buf = QString("%1:").arg(DApplication::translate("Process.Attributes.Dialog", "Name"));
    // process name label
    m_procNameLabel = new DLabel(wnd);
    m_procNameLabel->setText(buf);
    m_procNameLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    buf = QString("%1:").arg(DApplication::translate("Process.Attributes.Dialog", "Command line"));
    // process command label
    m_procCmdLabel = new DLabel(wnd);
    m_procCmdLabel->setText(buf);
    m_procCmdLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    buf = QString("%1:").arg(DApplication::translate("Process.Attributes.Dialog", "Start time"));
    // process start time label
    m_procStartLabel = new DLabel(wnd);
    m_procStartLabel->setText(buf);
    m_procStartLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // process name text
    m_procNameText = new DTextBrowser(wnd);
    m_procNameText->setFrameStyle(QFrame::NoFrame);
    m_procNameText->setFocusPolicy(Qt::NoFocus);
    m_procNameText->setFixedWidth(kPreferedTextWidth);
    m_procNameText->viewport()->setBackgroundRole(QPalette::Window);
    m_procNameText->setWordWrapMode(QTextOption::WrapAnywhere);
    m_procNameText->document()->setDocumentMargin(0);

    // process command line text
    m_procCmdText = new DTextBrowser(wnd);
    m_procCmdText->setFrameStyle(QFrame::NoFrame);
    m_procCmdText->setFocusPolicy(Qt::NoFocus);
    m_procCmdText->setFixedWidth(kPreferedTextWidth);
    m_procCmdText->viewport()->setBackgroundRole(QPalette::Window);
    m_procCmdText->setWordWrapMode(QTextOption::WrapAnywhere);
    m_procCmdText->document()->setDocumentMargin(0);

    // process start time text
    m_procStartText = new DTextBrowser(wnd);
    m_procStartText->setFrameStyle(QFrame::NoFrame);
    m_procStartText->setFocusPolicy(Qt::NoFocus);
    m_procStartText->setFixedWidth(kPreferedTextWidth);
    m_procStartText->viewport()->setBackgroundRole(QPalette::Window);
    m_procStartText->setWordWrapMode(QTextOption::WrapAnywhere);
    m_procStartText->document()->setDocumentMargin(0);

    grid->addWidget(m_procNameLabel, 0, 0, Qt::AlignTop | Qt::AlignRight);
    grid->addWidget(m_procNameText, 0, 1, Qt::AlignTop | Qt::AlignLeft);

    grid->addWidget(m_procCmdLabel, 1, 0, Qt::AlignTop | Qt::AlignRight);
    grid->addWidget(m_procCmdText, 1, 1, Qt::AlignTop | Qt::AlignLeft);

    grid->addWidget(m_procStartLabel, 2, 0, Qt::AlignTop | Qt::AlignRight);
    grid->addWidget(m_procStartText, 2, 1, Qt::AlignTop | Qt::AlignLeft);
    wnd->setLayout(grid);
    vlayout->addWidget(wnd, 0, Qt::AlignCenter);

    // spacing
    vlayout->addStretch(1);

    // fill icon & text content
    appIcon->setPixmap(m_icon.pixmap(kAppIconSize, kAppIconSize));
    m_appNameLabel->setText(m_displayName);
    m_appNameLabel->setToolTip(QString("<div>%1</div>").arg(m_displayName));
    m_procNameText->setText(m_procName);
    m_procCmdText->setText(m_cmdline);
    // format date time (ISO style)
    m_procStartText->setText(QDateTime::fromSecsSinceEpoch(qint64(m_startTime)).toString("yyyy-MM-dd hh:mm:ss"));

    m_frame->setLayout(vlayout);
    setCentralWidget(m_frame);
}

// resize event handler
void ProcessAttributeDialog::resizeEvent(QResizeEvent *event)
{
    DMainWindow::resizeEvent(event);

    // backup size when size changed
    if (m_settings) {
        m_settings->setOption(kSettingKeyProcessAttributeDialogWidth, width());
        m_settings->setOption(kSettingKeyProcessAttributeDialogHeight, height());
    }

    // raise shadow widget on top
    if (m_frame) {
        m_tbShadow->setFixedWidth(m_frame->size().width());
        m_tbShadow->raise();
        m_tbShadow->show();
    }

    if (m_procCmdText) {
        QFontMetrics fm(m_procCmdText->font());
        // recalculate command line text browser prefered height
        m_cmdh = m_frame->height()
                 - (kAppIconSize + kIconToNameSpace + m_appNameLabel->height() + kNameToGridSpace + m_margin * 2)
                 - m_procNameText->height() - m_procStartText->height();
        // line height of single lined plain text
        int singleline = fm.size(Qt::TextSingleLine, m_procCmdText->toPlainText()).height();
        // more tweaking
        m_cmdh = (qFloor(m_cmdh * 1.0 / singleline) - 1) * singleline;

        // show vertical scrollbar when document size larger than text browser's viewport size
        if (m_cmdh > m_procCmdText->document()->size().height()) {
            m_cmdh = int(m_procCmdText->document()->size().height());
        }
        m_procCmdText->setFixedHeight(m_cmdh);
        m_procCmdText->verticalScrollBar()->setValue(0);
    }
}

// close event handler
void ProcessAttributeDialog::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);

    // backup size settings
    if (m_settings) {
        m_settings->setOption(kSettingKeyProcessAttributeDialogWidth, width());
        m_settings->setOption(kSettingKeyProcessAttributeDialogHeight, height());
    }
}

// event filters
bool ProcessAttributeDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Show) {
        // show event
        if (obj == m_procNameLabel || obj == m_procCmdLabel || obj == m_procStartLabel) {
            auto *ctl = dynamic_cast<DLabel *>(obj);
            QFontMetrics fm(ctl->font());
            // calculate max label width
            int max = fm.size(Qt::TextSingleLine, ctl->text()).width();
            ctl->setFixedWidth(max);
            ctl->setFixedHeight(ctl->height());
        } else if (obj == m_procNameText || obj == m_procStartText || obj == m_procCmdText) {
            auto *ctl = dynamic_cast<DTextBrowser *>(obj);
            QFontMetrics fm(ctl->font());
            // calculate max content width
            int max = fm.size(Qt::TextSingleLine, ctl->toPlainText()).width();
            if (max > kPreferedTextWidth) {
                max = kPreferedTextWidth;
            }
            ctl->setFixedWidth(max);
            ctl->setMaximumHeight(int(ctl->document()->size().height()));
            // fix text cursor not at the first line issue
            auto textCur = ctl->textCursor();
            textCur.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor, 1);
            ctl->setTextCursor(textCur);
        }
    } else if (event->type() == QEvent::Leave) {
        // mouse leave event
        if (obj == m_procNameText) {
            if (m_procNameText->textCursor().hasSelection()) {
                // set focus if has any selected text
                m_procNameText->setFocus();

                // clear command line text selection if any
                auto cur = m_procCmdText->textCursor();
                cur.clearSelection();
                m_procCmdText->setTextCursor(cur);

                // clear start time text selection if any
                cur = m_procStartText->textCursor();
                cur.clearSelection();
                m_procStartText->setTextCursor(cur);
            }
        } else if (obj == m_procCmdText) {
            if (m_procCmdText->textCursor().hasSelection()) {
                // set focus if has any selected text
                m_procCmdText->setFocus();

                // clear name text selection if any
                auto cur = m_procNameText->textCursor();
                cur.clearSelection();
                m_procNameText->setTextCursor(cur);

                // clear start time text selection if any
                cur = m_procStartText->textCursor();
                cur.clearSelection();
                m_procStartText->setTextCursor(cur);
            }
        } else if (obj == m_procStartText) {
            if (m_procStartText->textCursor().hasSelection()) {
                // set focus if has any selected text
                m_procStartText->setFocus();

                // clear command line text selection if any
                auto cur = m_procCmdText->textCursor();
                cur.clearSelection();
                m_procCmdText->setTextCursor(cur);

                // clear name text selection if any
                cur = m_procNameText->textCursor();
                cur.clearSelection();
                m_procNameText->setTextCursor(cur);
            }
        }
    } else if (event->type() == QEvent::KeyPress) {
        // key press event
        if (obj == this) {
            auto *kev = dynamic_cast<QKeyEvent *>(event);
            // handle ESC key press event
            if (kev->matches(QKeySequence::Cancel)) {
                close();
                return true;
            } else {
                return false;
            }
        }
    } else if (event->type() == QEvent::MouseButtonPress) {
        // mouse button press event
        auto *ev = dynamic_cast<QMouseEvent *>(event);
        if (!(ev->button() & Qt::RightButton)) {
            auto cur = m_procNameText->textCursor();
            cur.clearSelection();
            m_procNameText->setTextCursor(cur);

            cur = m_procCmdText->textCursor();
            cur.clearSelection();
            m_procCmdText->setTextCursor(cur);

            cur = m_procStartText->textCursor();
            cur.clearSelection();
            m_procStartText->setTextCursor(cur);
        }
    }

    return DMainWindow::eventFilter(obj, event);
}
