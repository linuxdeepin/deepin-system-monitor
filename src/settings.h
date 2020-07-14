/*
* Copyright (C) 2011 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      Wang Yong <wangyong@deepin.com>
* Maintainer:  maojj <maojunjie@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>
#include <QSettings>

#include <mutex>
#include <thread>

const QString kSettingKeyDisplayMode = {"display_mode"};
const QString kSettingKeyThemeType = {"theme_type"};
const QString kSettingKeyWindowWidth = {"window_width"};
const QString kSettingKeyWindowHeight = {"window_height"};
const QString kSettingKeyProcessTabIndex = {"process_tab_index"};
const QString kSettingKeyProcessAttributeDialogWidth = {"process_attribute_dialog_width"};
const QString kSettingKeyProcessAttributeDialogHeight = {"process_attribute_dialog_height"};

class QSettings;

class Settings : public QObject
{
    Q_OBJECT

public:
    inline static Settings *instance()
    {
        Settings *sin = m_instance.load();
        if (!sin) {
            std::lock_guard<std::mutex> lock(m_mutex);
            sin = m_instance.load();
            if (!sin) {
                sin = new Settings();
                m_instance.store(sin);
            }
        }
        return sin;
    }

    static QString configPath();
    QVariant getOption(const QString &key);
    void init();
    void setOption(const QString &key, const QVariant &value);
    inline void flush() { m_settings->sync(); }

private:
    Settings(QObject *parent = nullptr);
    ~Settings();
    Q_DISABLE_COPY(Settings)

    QSettings *m_settings;
    QString m_groupName;

    static std::atomic<Settings *> m_instance;
    static std::mutex m_mutex;
};

#endif  // SETTINGS_H
