// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "wireless.h"
#include "ddlog.h"
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDebug>
#include <linux/wireless.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace DDLog;
namespace core {
namespace system {

wireless::wireless()
    : m_bwireless(false), m_link_quality(0), m_signal_level(0),
      m_noise_level(0) {
  qCDebug(app) << "wireless object created with default constructor";
}

wireless::wireless(QByteArray ifname)
    : m_bwireless(false), m_ifname(ifname), m_link_quality(0),
      m_signal_level(0), m_noise_level(0) {
  qCDebug(app) << "wireless object created for interface:" << ifname;
  read_wireless_info();
}

wireless::~wireless() { qCDebug(app) << "wireless object destroyed"; }

QByteArray wireless::essid() { return m_essid; }

uint8_t wireless::link_quality() { return m_link_quality; }

uint8_t wireless::signal_levle() { return m_signal_level; }

uint8_t wireless::noise_level() { return m_noise_level; }

bool wireless::is_wireless() { return m_bwireless; }

bool wireless::read_wireless_info() {
  qCDebug(app) << "Reading wireless info for interface:" << m_ifname;
  if (m_ifname.isNull()) {
    qCWarning(app) << "Interface name is null, cannot read wireless info";
    m_bwireless = false;
    return false;
  }

  // 创建NetworkManager接口
  QDBusInterface nm(
      "org.freedesktop.NetworkManager", "/org/freedesktop/NetworkManager",
      "org.freedesktop.NetworkManager", QDBusConnection::systemBus());

  if (!nm.isValid()) {
    m_bwireless = false;
    return false;
  }

  // 获取所有设备
  QDBusReply<QList<QDBusObjectPath>> devices = nm.call("GetDevices");
  if (!devices.isValid()) {
    m_bwireless = false;
    return false;
  }

  // 查找匹配的无线设备
  for (const QDBusObjectPath &devicePath : devices.value()) {
    QDBusInterface device("org.freedesktop.NetworkManager", devicePath.path(),
                          "org.freedesktop.NetworkManager.Device",
                          QDBusConnection::systemBus());
    if (!device.isValid()) {
      continue;
    }

    // 获取接口名称
    auto iface = device.property("Interface").toString();
    if (iface != m_ifname) {
      continue;
    }

    // 获取无线设备特定信息
    QDBusInterface wireless("org.freedesktop.NetworkManager", devicePath.path(),
                            "org.freedesktop.NetworkManager.Device.Wireless",
                            QDBusConnection::systemBus());
    if (!wireless.isValid()) {
      continue;
    }

    // 获取当前接入点
    QDBusReply<QList<QDBusObjectPath>> activeAp =
        wireless.call("GetAccessPoints");
    if (!activeAp.isValid() || activeAp.value().isEmpty()) {
      continue;
    }

    // 获取接入点信息
    QDBusInterface ap("org.freedesktop.NetworkManager",
                      activeAp.value().first().path(),
                      "org.freedesktop.NetworkManager.AccessPoint",
                      QDBusConnection::systemBus());
    if (!ap.isValid()) {
      continue;
    }

    // 获取SSID
    auto ssid = ap.property("Ssid").toByteArray();
    m_essid = ssid;

    // 获取信号强度
    auto strength = ap.property("Strength").toUInt();
    // 将信号强度转换为质量值（0-100）
    m_link_quality = strength;
    m_signal_level = strength;
    m_bwireless = true;
    return true;
  }

  m_bwireless = false;
  return false;
}

} // namespace system
} // namespace core
