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
#ifndef BLOCK_DEV_SUMMARY_VIEW_WIDGET_H
#define BLOCK_DEV_SUMMARY_VIEW_WIDGET_H

#include <DTableView>
DWIDGET_USE_NAMESPACE
/**
 * @brief Block device summary view widget
 */
class DeailTableModelBlock;
class BlockDevSummaryViewWidget : public DTableView
{
    Q_OBJECT
public:
    explicit BlockDevSummaryViewWidget(QWidget *parent = nullptr);

signals:

public slots:
    void fontChanged(const QFont &font);
    void chageSummaryInfo(const QString& deviceName);
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    QFont m_font;
    DeailTableModelBlock *m_model;
};

#endif // BLOCK_DEV_SUMMARY_VIEW_WIDGET_H
