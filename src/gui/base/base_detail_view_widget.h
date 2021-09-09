/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     leiyu <leiyu@uniontech.com>
*
* Maintainer: leiyu <leiyu@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef BASEDETAILVIEWWIDGET_H
#define BASEDETAILVIEWWIDGET_H

#include <QWidget>
#include <DCommandLinkButton>
#include <DIconButton>
#include <QVBoxLayout>

class BaseCommandLinkButton;

class BaseDetailViewWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title WRITE setTitle)
    Q_PROPERTY(QString detail READ detail WRITE setDetail)

public:
    explicit BaseDetailViewWidget(QWidget *parent = nullptr);

signals:
    void signalHideDetailClicked();

public:
    void setTitle(const QString &text);
    QString title();

    void setDetail(const QString &text);
    QString detail();

    int titleHeight();

public slots:
    void detailFontChanged(const QFont &font);

private:
    void updateWidgetGrometry();

protected:
    void resizeEvent(QResizeEvent *event);
    void paintEvent(QPaintEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

protected:
    QFont m_contentFont;
    QFont m_titleFont;

    QString m_titleText;
    QString m_detailText;

    BaseCommandLinkButton *m_detailButton;
    Dtk::Widget::DIconButton *m_arrowButton;

    QVBoxLayout *m_centralLayout;
};

#endif // BASEDETAILVIEWWIDGET_H
