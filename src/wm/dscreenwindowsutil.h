/**
 * Copyright (C) 2017 Deepin Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 **/

#ifndef DSCREENWINDOWSUTIL_H
#define DSCREENWINDOWSUTIL_H

#include <QObject>
#include <QScreen>

#include  <DObject>

#include "dwindowmanager.h"
#include "dtkwm_global.h"

DWM_BEGIN_NAMESPACE

class DScreenWindowsUtilPrivate;
class DScreenWindowsUtil : public QObject ,
        public DTK_CORE_NAMESPACE::DObject
{
    Q_OBJECT
public:
    static DScreenWindowsUtil *instance(QPoint pos);

    ~DScreenWindowsUtil();

    int getScreenNum() const;
    QRect backgroundRect() const;
    WId rootWindowId() const;
    QScreen* primaryScreen() const;
    QList<QRect> windowsRect() const;
    QStringList windowsName() const;
    bool isPrimayScreen() const;

protected:
    DScreenWindowsUtil(QPoint pos, QObject *parent = 0);

private:
    D_DECLARE_PRIVATE(DScreenWindowsUtil)
};

DWM_END_NAMESPACE

#endif // DSCREENWINDOWSUTIL_H
