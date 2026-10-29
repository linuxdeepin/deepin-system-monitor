// Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "model_manager.h"
#include "ddlog.h"

using namespace DDLog;

Q_GLOBAL_STATIC(ModelManager, theInstance)
ModelManager::ModelManager(QObject *parent)
    : QObject(parent)
{
    qCDebug(app) << "ModelManager constructor";
}

ModelManager *ModelManager::instance()
{
    // qCDebug(app) << "ModelManager::instance()";
    return theInstance();
}
