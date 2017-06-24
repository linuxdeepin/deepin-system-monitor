/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2017 Deepin, Inc.
 *               2011 ~ 2017 Wang Yong
 *
 * Author:     Wang Yong <wangyong@deepin.com>
 * Maintainer: Wang Yong <wangyong@deepin.com>
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

#ifndef ATTRIBUTESDIALOG_H
#define ATTRIBUTESDIALOG_H 

#include "find_window_title.h"
#include <QLabel>
#include <QPaintEvent>
#include <QVBoxLayout>
#include <QWidget>
#include <ddialog.h>
#include <dwindowclosebutton.h>

DWIDGET_USE_NAMESPACE

class AttributesDialog : public Dtk::Widget::DAbstractDialog
{
    Q_OBJECT
    
public:
    AttributesDialog(QWidget *parent = 0, int pid=-1);
    ~AttributesDialog();

    int getPid();
    void paintEvent(QPaintEvent *);
    
private:
    DWindowCloseButton *closeButton;
    FindWindowTitle *findWindowTitle;
    QHBoxLayout *cmdlineLayout;
    QHBoxLayout *nameLayout;
    QHBoxLayout *startTimeLayout;
    QLabel *cmdlineLabel;
    QLabel *cmdlineTitleLabel;
    QLabel *iconLabel;
    QLabel *nameLabel;
    QLabel *nameTitleLabel;
    QLabel *startTimeLabel;
    QLabel *startTimeTitleLabel;
    QLabel *titleLabel;
    QVBoxLayout *layout;
    int pid;
};

#endif
