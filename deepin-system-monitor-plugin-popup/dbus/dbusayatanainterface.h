/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     yukuan  <yukuan@uniontech.com>
*
* Maintainer: yukuan  <yukuan@uniontech.com>
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
#ifndef DBUSAYATANAINTERFACE_H
#define DBUSAYATANAINTERFACE_H

#include <QObject>

#include <mutex>

class QDBusInterface;

class DBusAyatanaInterface: public QObject
{
    Q_OBJECT
public:
    inline static DBusAyatanaInterface *getInstance()
    {
        // 利用原子变量解决，单例模式造成的内存泄露
        DBusAyatanaInterface *sin = s_Instance.load();

        if (!sin) {
            // std::lock_guard 自动加锁解锁
            std::lock_guard<std::mutex> lock(m_mutex);
            sin = s_Instance.load();

            if (!sin) {
                sin = new DBusAyatanaInterface();
                s_Instance.store(sin);
            }
        }

        return sin;
    }

protected:
    DBusAyatanaInterface(QObject *parent = nullptr);

public slots:
    //!
    //! \brief slotActiveApplicationChanged 连接org.ayatana.bamf信号，判断窗口的激活状态
    //! \param path 路径名称
    //! \param name 名称
    //!
    void slotActiveApplicationChanged(QString path, QString name);

signals:
    //!
    //! \brief sigSendCloseWidget 发送关闭窗口信号
    //!
    void sigSendCloseWidget();
private:
    /**
     * @brief init:初始化DBus
     */
    void init();

private:
    static std::atomic<DBusAyatanaInterface *> s_Instance;
    static std::mutex m_mutex;
};


#endif // DBUSAYATANAINTERFACE_H
