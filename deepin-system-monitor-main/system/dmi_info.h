// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DMI_INFO_H
#define DMI_INFO_H

#include <QList>
#include <QString>

#include <QtGlobal>

/**
 * @brief 轻量级 SMBIOS/DMI 表解析器
 *
 * 仅解析 deepin-system-monitor 实际需要的少量信息（CPU 频率、缓存容量、
 * 产品名称、OEM 字符串），用于替代内置的 dmidecode 三方库。
 *
 * 数据来源（按优先级）：
 *  1. sysfs: /sys/firmware/dmi/tables/{smbios_entry_point,DMI}
 *  2. EFI:   /sys/firmware/efi/systab 中记录的物理地址 + /dev/mem
 *  3. /dev/mem 0xF0000~0xFFFFF 内存扫描（仅 x86，需要 root 权限）
 *
 * 后两种方式与 dmidecode 命令行工具的回退行为一致，均需要 root 权限，
 * 普通用户运行时读取失败则所有接口返回空值/0，与原先调用
 * dmidecode 命令失败时的表现一致。
 */
class DmiInfo
{
public:
    struct CacheInfo {
        QString socketDesignation;    ///< 缓存标识，如 "L1 Cache"
        quint64 maximumSizeKB = 0;    ///< 最大容量（KB），0 表示未提供
        quint64 installedSizeKB = 0;  ///< 已安装容量（KB），0 表示未提供
    };

    static DmiInfo *instance();

    /// 仅供单元测试使用：替换 sysfs 根目录（默认 "/sys"）并强制重新加载表
    static void reloadForTest(const QString &sysfsRoot);

    /// 表是否成功加载
    bool isValid() const { return m_valid; }

    /// Type 1 (System Information) 厂商名称
    QString systemManufacturer() const { return m_manufacturer; }
    /// Type 1 (System Information) 产品名称，等价于 `dmidecode -s system-product-name`
    QString systemProductName() const { return m_productName; }

    /// Type 11 (OEM Strings) 第 \a index 个字符串（索引从 1 开始），不存在返回空串
    QString oemString(int index) const;

    /// Type 4 (Processor Information) 当前频率（MHz），0 表示未知
    quint16 currentCpuSpeedMHz() const { return m_currentCpuMHz; }
    /// Type 4 (Processor Information) 最大频率（MHz），0 表示未知
    quint16 maxCpuSpeedMHz() const { return m_maxCpuMHz; }

    /// Type 7 (Cache Information) 缓存列表
    const QList<CacheInfo> &caches() const { return m_caches; }

private:
    DmiInfo();
    ~DmiInfo() = default;
    DmiInfo(const DmiInfo &) = delete;
    DmiInfo &operator=(const DmiInfo &) = delete;

    void load();
    bool loadFromSysfs();
    bool loadFromEfiOrMemory();
    void parseEntryPoints(const uchar *buf, qint64 size);
    void parseTable(const uchar *buf, qint64 len);
    void reset();

    static QString s_sysfsRoot;

    bool m_valid = false;
    QString m_manufacturer;
    QString m_productName;
    quint16 m_currentCpuMHz = 0;
    quint16 m_maxCpuMHz = 0;
    QList<CacheInfo> m_caches;
    QList<QString> m_oemStrings;
};

#endif  // DMI_INFO_H
