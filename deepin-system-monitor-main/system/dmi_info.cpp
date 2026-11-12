// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dmi_info.h"

#include "ddlog.h"

#include <QByteArray>
#include <QDebug>
#include <QFile>

#include <cstring>

namespace {

// SMBIOS 结构句柄类型
constexpr uchar kTypeSystemInfo = 1;     // Type 1: System Information
constexpr uchar kTypeProcessorInfo = 4;  // Type 4: Processor Information
constexpr uchar kTypeCacheInfo = 7;      // Type 7: Cache Information
constexpr uchar kTypeOemStrings = 11;    // Type 11: OEM Strings

constexpr const char *kDevMem = "/dev/mem";

inline quint16 readWord(const uchar *p)
{
    return static_cast<quint16>(p[0] | (p[1] << 8));
}

inline quint32 readDWord(const uchar *p)
{
    return static_cast<quint32>(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
}

inline quint64 readQWord(const uchar *p)
{
    return static_cast<quint64>(readDWord(p)) | (static_cast<quint64>(readDWord(p + 4)) << 32);
}

bool checksum(const uchar *buf, uchar len)
{
    uchar sum = 0;
    for (int i = 0; i < len; ++i)
        sum += buf[i];
    return sum == 0;
}

/**
 * @brief 读取 \a index 号字符串（结构 \a hdr 起始、格式化区长度 \a hdrLen，
 *          字符串区不得越过表尾 \a end），index 为 0 或越界返回空串。
 *          与 dmidecode 一致，非可打印 ASCII 字符替换为 '.'。
 */
QString dmiString(const uchar *hdr, uchar hdrLen, uchar index, const uchar *end)
{
    if (index == 0)
        return QString();

    const uchar *bp = hdr + hdrLen;
    while (bp < end && index > 1) {
        if (*bp == '\0')  // 字符串数量不足
            return QString();
        while (bp < end && *bp != '\0')
            ++bp;
        if (bp >= end)
            return QString();
        ++bp;  // 跳过字符串结尾的 NUL
        --index;
    }
    if (bp >= end || *bp == '\0')
        return QString();

    const uchar *e = bp;
    while (e < end && *e != '\0')
        ++e;
    QString str = QString::fromLatin1(reinterpret_cast<const char *>(bp), static_cast<int>(e - bp));
    for (QChar &ch : str) {
        if (ch.unicode() < 32 || ch.unicode() >= 127)
            ch = QLatin1Char('.');
    }
    return str;
}

/// SMBIOS 缓存容量字段转 KB（16 位格式：bit15 置位时单位为 64KB）
quint64 cacheSizeKB(quint16 code)
{
    if (code & 0x8000u)
        return static_cast<quint64>(code & 0x7FFFu) * 64;
    return code;
}

/// SMBIOS 缓存容量字段转 KB（32 位格式：bit31 置位时单位为 64KB）
quint64 cacheSizeKB(quint32 code)
{
    if (code & 0x80000000u)
        return static_cast<quint64>(code & 0x7FFFFFFFu) * 64;
    return code;
}

/// 读取整个文件，失败返回空数组
QByteArray readFile(const char *path, qint64 maxSize = 16 * 1024 * 1024)
{
    QFile file(QString::fromLatin1(path));
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return file.read(maxSize);
}

}  // namespace

// sysfs 根目录（单测可重定向）
QString DmiInfo::s_sysfsRoot = QStringLiteral("/sys");

DmiInfo *DmiInfo::instance()
{
    static DmiInfo info;
    return &info;
}

DmiInfo::DmiInfo()
{
    load();
}

void DmiInfo::load()
{
    if (loadFromSysfs())
        return;
    qCDebug(DDLog::app) << "Failed to load DMI table from sysfs, fallback to EFI/devmem";
    loadFromEfiOrMemory();
}

void DmiInfo::reset()
{
    m_valid = false;
    m_manufacturer.clear();
    m_productName.clear();
    m_currentCpuMHz = 0;
    m_maxCpuMHz = 0;
    m_caches.clear();
    m_oemStrings.clear();
}

void DmiInfo::reloadForTest(const QString &sysfsRoot)
{
    DmiInfo *info = instance();
    s_sysfsRoot = sysfsRoot;
    info->reset();
    info->load();
}

bool DmiInfo::loadFromSysfs()
{
    const QByteArray table = readFile(qPrintable(s_sysfsRoot + QStringLiteral("/firmware/dmi/tables/DMI")));
    if (table.isEmpty())
        return false;

    parseTable(reinterpret_cast<const uchar *>(table.constData()), table.size());
    m_valid = true;
    return true;
}

bool DmiInfo::loadFromEfiOrMemory()
{
    // 与 dmidecode 一致：先尝试 EFI systab 中记录的地址，再扫描 /dev/mem
    qint64 addr = -1;
    bool smbios3 = false;

    const QByteArray systab = readFile(qPrintable(s_sysfsRoot + QStringLiteral("/firmware/efi/systab")));
    if (!systab.isEmpty()) {
        const QList<QByteArray> lines = systab.split('\n');
        for (const QByteArray &line : lines) {
            const int eq = line.indexOf('=');
            if (eq <= 0)
                continue;
            const QByteArray key = line.left(eq);
            if (key == "SMBIOS3" || key == "SMBIOS") {
                addr = strtoull(line.mid(eq + 1).constData(), nullptr, 0);
                smbios3 = (key == "SMBIOS3");
                break;
            }
        }
    }

    QFile devMem(QString::fromLatin1(kDevMem));
    if (!devMem.open(QIODevice::ReadOnly))
        return false;

    if (addr >= 0) {
        // EFI 模式：从记录的物理地址读取入口点
        if (!devMem.seek(addr))
            return false;
        QByteArray entry = devMem.read(0x20);
        if (entry.size() >= 0x18 && qstrncmp(entry.constData(), smbios3 ? "_SM3_" : "_SM_", 5) == 0)
            parseEntryPoints(reinterpret_cast<const uchar *>(entry.constData()), entry.size());
        return m_valid;
    }

#if defined(__i386__) || defined(__x86_64__)
    // 内存扫描模式（x86）：在 0xF0000~0xFFFFF 中查找入口点
    if (!devMem.seek(0xF0000))
        return false;
    const QByteArray bios = devMem.read(0x10000);
    if (bios.size() < 0x20)
        return false;
    const uchar *buf = reinterpret_cast<const uchar *>(bios.constData());
    const qint64 size = bios.size();

    // 先查找 64 位入口点
    for (qint64 off = 0; off <= size - 0x18; off += 16) {
        if (qstrncmp(bios.constData() + off, "_SM3_", 5) == 0) {
            parseEntryPoints(buf + off, 0x20);
            if (m_valid)
                return true;
        }
    }
    // 再查找 32 位入口点
    for (qint64 off = 0; off <= size - 0x20; off += 16) {
        if (qstrncmp(bios.constData() + off, "_SM_", 4) == 0 || qstrncmp(bios.constData() + off, "_DMI_", 5) == 0) {
            parseEntryPoints(buf + off, 0x20);
            if (m_valid)
                return true;
        }
    }
#endif
    return m_valid;
}

void DmiInfo::parseEntryPoints(const uchar *buf, qint64 size)
{
    Q_UNUSED(size);
    if (qstrncmp(reinterpret_cast<const char *>(buf), "_SM3_", 5) == 0) {
        // SMBIOS 3.0 64 位入口点
        if (buf[0x06] > 0x20 || !checksum(buf, buf[0x06]))
            return;
        const quint64 addr = readQWord(buf + 0x10);
        const quint32 maxLen = readDWord(buf + 0x0C);
        QFile devMem(QString::fromLatin1(kDevMem));
        if (!devMem.open(QIODevice::ReadOnly) || !devMem.seek(static_cast<qint64>(addr)))
            return;
        const QByteArray table = devMem.read(qMin<qint64>(maxLen ? maxLen : 0x10000, 64 * 1024 * 1024));
        if (table.isEmpty())
            return;
        parseTable(reinterpret_cast<const uchar *>(table.constData()), table.size());
        m_valid = true;
    } else if (qstrncmp(reinterpret_cast<const char *>(buf), "_SM_", 4) == 0
               && qstrncmp(reinterpret_cast<const char *>(buf) + 0x10, "_DMI_", 5) == 0) {
        // SMBIOS 2.x 32 位入口点
        if (buf[0x05] > 0x20 || !checksum(buf, buf[0x05]) || !checksum(buf + 0x10, 0x0F))
            return;
        const quint32 addr = readDWord(buf + 0x18);
        const quint32 len = readWord(buf + 0x16);
        QFile devMem(QString::fromLatin1(kDevMem));
        if (!devMem.open(QIODevice::ReadOnly) || !devMem.seek(static_cast<qint64>(addr)))
            return;
        const QByteArray table = devMem.read(len);
        if (table.isEmpty())
            return;
        parseTable(reinterpret_cast<const uchar *>(table.constData()), table.size());
        m_valid = true;
    } else if (qstrncmp(reinterpret_cast<const char *>(buf), "_DMI_", 5) == 0) {
        // Legacy DMI 入口点
        if (!checksum(buf, 0x0F))
            return;
        const quint32 addr = readDWord(buf + 0x08);
        const quint32 len = readWord(buf + 0x06);
        QFile devMem(QString::fromLatin1(kDevMem));
        if (!devMem.open(QIODevice::ReadOnly) || !devMem.seek(static_cast<qint64>(addr)))
            return;
        const QByteArray table = devMem.read(len);
        if (table.isEmpty())
            return;
        parseTable(reinterpret_cast<const uchar *>(table.constData()), table.size());
        m_valid = true;
    }
}

void DmiInfo::parseTable(const uchar *buf, qint64 len)
{
    const uchar *end = buf + len;
    const uchar *data = buf;

    while (data + 4 <= end) {
        const uchar type = data[0];
        const uchar hdrLen = data[1];
        // 结构头不完整则无法可靠定位下一个结构，直接终止
        if (hdrLen < 4)
            break;

        // 字符串区以连续两个 NUL 结束
        const uchar *next = data + hdrLen;
        while (next + 1 < end && (next[0] != '\0' || next[1] != '\0'))
            ++next;
        next += 2;
        if (next > end)
            break;

        switch (type) {
        case kTypeSystemInfo: {
            if (hdrLen >= 0x06) {
                m_manufacturer = dmiString(data, hdrLen, data[0x04], end);
                m_productName = dmiString(data, hdrLen, data[0x05], end);
            }
            break;
        }
        case kTypeProcessorInfo: {
            // 只取第一个处理器结构的频率信息
            // SMBIOS 规范：0x10 为 Max Speed，0x12 为 Current Speed（单位 MHz）
            if (hdrLen >= 0x14 && m_maxCpuMHz == 0) {
                m_maxCpuMHz = readWord(data + 0x10);
                m_currentCpuMHz = readWord(data + 0x12);
            }
            break;
        }
        case kTypeCacheInfo: {
            CacheInfo info;
            if (hdrLen >= 0x05)
                info.socketDesignation = dmiString(data, hdrLen, data[0x04], end);
            if (hdrLen >= 0x0F) {
                // SMBIOS 3.1+ 使用 32 位容量字段，否则使用 16 位字段
                if (hdrLen >= 0x17)
                    info.maximumSizeKB = cacheSizeKB(readDWord(data + 0x13));
                else
                    info.maximumSizeKB = cacheSizeKB(readWord(data + 0x07));
                if (hdrLen >= 0x1B)
                    info.installedSizeKB = cacheSizeKB(readDWord(data + 0x17));
                else
                    info.installedSizeKB = cacheSizeKB(readWord(data + 0x09));
            }
            m_caches.append(info);
            break;
        }
        case kTypeOemStrings: {
            if (hdrLen >= 0x05) {
                const uchar count = data[0x04];
                for (uchar i = 1; i <= count; ++i)
                    m_oemStrings.append(dmiString(data, hdrLen, i, end));
            }
            break;
        }
        default:
            break;
        }

        data = next;
    }
}

QString DmiInfo::oemString(int index) const
{
    if (index <= 0 || index > m_oemStrings.size())
        return QString();
    return m_oemStrings.at(index - 1);
}
