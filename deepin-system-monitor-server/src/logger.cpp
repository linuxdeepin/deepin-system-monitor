// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "logger.h"
#include "dtkcore_global.h"
#include "qglobal.h"
#include "ddlog.h"
#include <QLoggingCategory>
#include <QObject>

#include <DConfig>
using namespace DDLog;
DCORE_USE_NAMESPACE

MLogger::MLogger(QObject *parent)
    : QObject(parent), m_rules(""), m_config(nullptr)
{
    QByteArray logRules = qgetenv("QT_LOGGING_RULES");
    // qunsetenv 之前一定不要有任何日志打印，否则取消环境变量设置不会生效
    qunsetenv("QT_LOGGING_RULES");

    // set env
    m_rules = logRules;

    // set dconfig
    m_config = DConfig::create("deepin-system-monitor-server", "org.deepin.system-monitor.server");
    logRules = m_config->value("log_rules").toByteArray();
    appendRules(logRules);
    setRules(m_rules);

    // watch dconfig
    connect(m_config, &DConfig::valueChanged, this, [this](const QString &key) {
        qCCritical(app) << "value changed:" << key;
        if (key == "log_rules") {
            setRules(m_config->value(key).toByteArray());
        }
    });
}

MLogger::~MLogger()
{
    m_config->deleteLater();
}

void MLogger::setRules(const QString &rules)
{
    auto tmpRules = rules;
    m_rules = tmpRules.replace(";", "\n");
    QLoggingCategory::setFilterRules(m_rules);
}

void MLogger::appendRules(const QString &rules)
{
    QString tmpRules = rules;
    tmpRules = tmpRules.replace(";", "\n");
    auto tmplist = tmpRules.split('\n');
    for (int i = 0; i < tmplist.count(); i++)
        if (m_rules.contains(tmplist.at(i))) {
            tmplist.removeAt(i);
            i--;
        }
    if (tmplist.isEmpty())
        return;
    m_rules.isEmpty() ? m_rules = tmplist.join("\n")
                      : m_rules += "\n" + tmplist.join("\n");
}
