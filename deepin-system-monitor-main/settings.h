/*
* Copyright (C) 2011 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      leiyu <maojunjie@uniontech.com>
* Maintainer:  leiyu <maojunjie@uniontech.com>
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
#include <QVariant>

const QString kSettingKeyDisplayMode = {"display_mode"};
const QString kSettingKeyWindowWidth = {"window_width"};
const QString kSettingKeyWindowHeight = {"window_height"};
const QString kSettingKeyProcessTabIndex = {"process_tab_index"};
const QString kSettingKeyProcessAttributeDialogWidth = {"process_attribute_dialog_width"};
const QString kSettingKeyProcessAttributeDialogHeight = {"process_attribute_dialog_height"};
const QString kSettingKeyTimePeriod = {"time_period"};

class QSettings;
class Settings
{
public:
    explicit Settings();
    ~Settings();

    static Settings *instance();
    void flush();

    QVariant getOption(const QString &key, const QVariant &defaultValue = QVariant());
    void setOption(const QString &key, const QVariant &value);

private:
    QSettings *m_settings;
};

#endif  // SETTINGS_H
