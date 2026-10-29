// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// DBusAyatanaInterface
// (deepin-system-monitor-plugin-popup/dbus/dbusayatanainterface.cpp)单元测试
//
// 外部依赖打桩:
//  - QFile::open:desktop 文件读取路径固定为 DEEPIN_AI_ASSISTANT_PATH,
//    测试不依赖真实文件。open 为虚函数(Itanium ABI 下成员函数指针编码为
//    虚表索引而非真实地址,无法直接 Stub::set),改由 AddrAny 解析
//    libQt6Core.so 的 dynsym 得到真实实现地址后打桩;
//  - QTextStream::atEnd/readLine:非虚函数,注入 desktop 文件行内容;
//  - dbus 调用不打桩:私有总线上不存在 org.ayatana.bamf 服务,call("Name")
//    立即收到 ServiceUnknown 错误应答,reply.value() 为空串(确定性分支)。

#include <gtest/gtest.h>

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QFile>
#include <QIODevice>
#include <QSignalSpy>
#include <QStringList>
#include <QTextStream>

#include <map>
#include <string>

#include "dbusayatanainterface.h"
#include "../../tests/addr_any.h"
#include "../../tests/stub.h"

namespace {

typedef bool (*QFileOpenPtr)(QFile *, QIODevice::OpenMode);

// QFile::open 真实实现地址(虚函数,需经 ELF 符号表解析)
char *g_qfileOpenAddr = nullptr;

bool resolveQFileOpenAddr()
{
    AddrAny any("libQt6Core.so");
    std::map<std::string, void *> result;
    any.get_global_func_addr_dynsym("QFile::open(QFlags", result);
    for (const auto &it : result) {
        if (it.first == "QFile::open(QFlags<QIODeviceBase::OpenModeFlag>)") {
            g_qfileOpenAddr = static_cast<char *>(it.second);
            return true;
        }
    }
    return false;
}

// ---- knob ----
bool g_fileOpenOk = true;         // QFile::open 返回值
QStringList g_desktopLines;       // 注入的 desktop 文件行
int g_lineCursor = 0;             // 行游标

bool stub_QFile_open(QFile *self, QIODevice::OpenMode mode)
{
    Q_UNUSED(self)
    Q_UNUSED(mode)
    return g_fileOpenOk;
}

bool stub_QTextStream_atEnd(const QTextStream *self)
{
    Q_UNUSED(self)
    // 语义与 QTextStream 一致:所有行已读尽时为 true(游标越过末尾)
    return g_lineCursor >= g_desktopLines.count();
}

QString stub_QTextStream_readLine(QTextStream *self, qint64 maxlen)
{
    Q_UNUSED(self)
    Q_UNUSED(maxlen)
    if (g_lineCursor >= g_desktopLines.count())
        return QString();
    return g_desktopLines.at(g_lineCursor++);
}

}  // namespace

class UT_DBusAyatana : public ::testing::Test
{
protected:
    void SetUp() override
    {
        g_fileOpenOk = true;
        g_desktopLines.clear();
        g_lineCursor = 0;
        if (g_qfileOpenAddr == nullptr) {
            ASSERT_TRUE(resolveQFileOpenAddr());
        }
        m_stub.set((QFileOpenPtr)g_qfileOpenAddr, stub_QFile_open);
        m_stub.set((bool (QTextStream::*)() const)ADDR(QTextStream, atEnd), stub_QTextStream_atEnd);
        m_stub.set((QString (QTextStream::*)(qint64))ADDR(QTextStream, readLine), stub_QTextStream_readLine);
    }
    void TearDown() override
    {
        g_fileOpenOk = true;
        g_desktopLines.clear();
        g_lineCursor = 0;
    }
    Stub m_stub;
};

// 用例名称: getinstance_sameInstance
// 测试类型: 正常场景
// 覆盖函数: DBusAyatanaInterface::getInstance(原子单例双检锁)+ 构造 + init
// 操作步骤: 多次调用 getInstance 比对指针
// 预期结果: 每次返回同一实例,构造与 init(DBus connect)无崩溃
TEST_F(UT_DBusAyatana, getinstance_sameInstance)
{
    // Act
    DBusAyatanaInterface *first = DBusAyatanaInterface::getInstance();
    DBusAyatanaInterface *second = DBusAyatanaInterface::getInstance();
    // Assert
    EXPECT_NE(first, nullptr);
    EXPECT_EQ(first, second);
}

// 用例名称: slot_emptyPath_noSignal
// 测试类型: 边界场景
// 覆盖函数: slotActiveApplicationChanged(路径为空提前返回分支)
// 操作步骤: 传入空路径调用槽
// 预期结果: 不发出 sigSendCloseWidget
TEST_F(UT_DBusAyatana, slot_emptyPath_noSignal)
{
    // Arrange
    DBusAyatanaInterface *iface = DBusAyatanaInterface::getInstance();
    QSignalSpy spy(iface, &DBusAyatanaInterface::sigSendCloseWidget);
    // Act
    iface->slotActiveApplicationChanged(QString(), QStringLiteral("app"));
    // Assert
    EXPECT_EQ(spy.count(), 0);
}

// 用例名称: slot_fileOpenFailed_noSignal
// 测试类型: 异常场景
// 覆盖函数: slotActiveApplicationChanged(文件打开失败告警 + 提前返回分支)
// 操作步骤: 注入 open 失败,传入非空路径
// 预期结果: 不发出 sigSendCloseWidget
TEST_F(UT_DBusAyatana, slot_fileOpenFailed_noSignal)
{
    // Arrange
    DBusAyatanaInterface *iface = DBusAyatanaInterface::getInstance();
    QSignalSpy spy(iface, &DBusAyatanaInterface::sigSendCloseWidget);
    g_fileOpenOk = false;
    // Act
    iface->slotActiveApplicationChanged(QStringLiteral("/org/ayatana/bamf/window/1"),
                                        QStringLiteral("app"));
    // Assert
    EXPECT_EQ(spy.count(), 0);
}

// 用例名称: slot_emptyDesktopFile_noSignal
// 测试类型: 边界场景
// 覆盖函数: slotActiveApplicationChanged(文件打开成功但内容为空,循环跳过分支)
// 操作步骤: 注入空行列表,非空路径调用
// 预期结果: 不发出 sigSendCloseWidget
TEST_F(UT_DBusAyatana, slot_emptyDesktopFile_noSignal)
{
    // Arrange
    DBusAyatanaInterface *iface = DBusAyatanaInterface::getInstance();
    QSignalSpy spy(iface, &DBusAyatanaInterface::sigSendCloseWidget);
    // Act
    iface->slotActiveApplicationChanged(QStringLiteral("/org/ayatana/bamf/window/1"),
                                        QStringLiteral("app"));
    // Assert
    EXPECT_EQ(spy.count(), 0);
}

// 用例名称: slot_replyEmpty_matchesEmptyName_emits
// 测试类型: 正常场景
// 覆盖函数: slotActiveApplicationChanged(行解析 + Name/GenericName 匹配 + 发信号分支)
// 操作步骤: 注入含空值 Name= 行的 desktop 内容(私有总线无 ayatana 服务,
//           reply.value() 为空串),非空路径调用
// 预期结果: 空串命中发出一次 sigSendCloseWidget
TEST_F(UT_DBusAyatana, slot_replyEmpty_matchesEmptyName_emits)
{
    // Arrange
    DBusAyatanaInterface *iface = DBusAyatanaInterface::getInstance();
    QSignalSpy spy(iface, &DBusAyatanaInterface::sigSendCloseWidget);
    g_desktopLines = QStringList { QStringLiteral("[Desktop Entry]"),
                                   QStringLiteral("Name="),
                                   QStringLiteral("GenericName=AI助手"),
                                   QStringLiteral("Icon=ai-assistant") };
    // Act
    iface->slotActiveApplicationChanged(QStringLiteral("/org/ayatana/bamf/window/1"),
                                        QStringLiteral("app"));
    // Assert
    EXPECT_EQ(spy.count(), 1);
}

// 用例名称: slot_noMatchingName_noSignal
// 测试类型: 正常场景
// 覆盖函数: slotActiveApplicationChanged(名称均不匹配分支)
// 操作步骤: 注入非空名称行,reply.value() 为空串不匹配
// 预期结果: 不发出 sigSendCloseWidget
TEST_F(UT_DBusAyatana, slot_noMatchingName_noSignal)
{
    // Arrange
    DBusAyatanaInterface *iface = DBusAyatanaInterface::getInstance();
    QSignalSpy spy(iface, &DBusAyatanaInterface::sigSendCloseWidget);
    g_desktopLines = QStringList { QStringLiteral("Name=AI助手"),
                                   QStringLiteral("GenericName=智能助手"),
                                   QStringLiteral("Icon=ai-assistant") };
    // Act
    iface->slotActiveApplicationChanged(QStringLiteral("/org/ayatana/bamf/window/1"),
                                        QStringLiteral("app"));
    // Assert
    EXPECT_EQ(spy.count(), 0);
}

// 用例名称: slot_localizedName_parsedByContains
// 测试类型: 边界场景
// 覆盖函数: slotActiveApplicationChanged(contains 判定含本地化键 Name[zh_CN] 分支)
// 操作步骤: 注入 Name[zh_CN] 行,验证 contains("Name") 命中后仍不匹配空 reply
// 预期结果: 不发出 sigSendCloseWidget(名称非空,与空 reply 不等)
TEST_F(UT_DBusAyatana, slot_localizedName_parsedByContains)
{
    // Arrange
    DBusAyatanaInterface *iface = DBusAyatanaInterface::getInstance();
    QSignalSpy spy(iface, &DBusAyatanaInterface::sigSendCloseWidget);
    g_desktopLines = QStringList { QStringLiteral("Name[zh_CN]=AI助手"),
                                   QStringLiteral("Name=AI") };
    // Act
    iface->slotActiveApplicationChanged(QStringLiteral("/org/ayatana/bamf/window/1"),
                                        QStringLiteral("app"));
    // Assert: 解析出的名称("AI助手"/"AI")均与空 reply 不匹配
    EXPECT_EQ(spy.count(), 0);
}

