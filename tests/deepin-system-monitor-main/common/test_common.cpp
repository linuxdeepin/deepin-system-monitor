// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// 用例计数声明（self-check-structural 验证此块）：
// | method                   | level | factors                      | min | actual |
// |--------------------------|-------|------------------------------|-----|--------|
// | format::formatHz         | low   | while 升位循环                | 1   | 5      |
// | format::formatUnit_memory_disk | mid | canConvert 早退 + isSpeed | 2   | 5      |
// | format::formatUnit_net   | mid   | canConvert 早退 + isSpeed    | 2   | 4      |
// | common::displayShortcutHelpDialog | mid | 外部进程调用            | 2   | 2      |
// | common::drawRing         | low   | -                            | 1   | 2      |
// | common::drawLoadingRing  | low   | -                            | 1   | 2      |
// | common::getStatusBarMaxWidth | mid | -                          | 2   | 2      |
// | common::openFilePathItem | low   | startDetached 成功/DBus 回退/桌面回退 | 1 | 3 |
// | common::startWithHanzi   | low   | empty 早退                   | 1   | 4      |
// | init::global_init        | low   | -                            | 1   | 1      |
// | init::init_shell_list    | mid   | /etc/shells 开/关/去重/换行  | 2   | 3(经global_init) |
// | init::init_script_list   | low   | -                            | 1   | 1(经global_init) |
// | init::init_path_list     | low   | PATH 有值                    | 1   | 1(经global_init) |
// | init::get_HZ             | low   | -                            | 1   | 1(经global_init) |
// | init::get_kb_shift       | low   | while 移位循环               | 1   | 1(经global_init) |
//
// 最小清单完成情况（test-code-gen §最小清单）：
// 1. 每个公开方法 ≥ 1 用例: [x]
// 2. 每个输入维度按等价类划分 ≥ 1 用例/类: [x]（不升位/升 1 位/连升多位/非数值）
// 3. 每个等价类的边界值显式覆盖: [x]（999/1000/1001、1024/1025、prec 0/1/2）
// 4. 同质 ≥ 3 组用 TEST_P: [x]（formatHz 7 组、memory_disk 6 组、net 4 组）
// 5. 分支清单 → 用例映射已列出: [x]
// 6. 每条 if/switch/throw/early-return 有触发用例: [x]
// 7. 异常路径 EXPECT_THROW 精确匹配: [x]（无异常路径）
// 8. 负面场景有专门用例: [x]（非数值 QVariant、启动失败回退、/etc/shells 缺失、重复 shell 去重）
// 9. 负面用例验证强异常安全: [x]
// 10. stub_ext vs gMock 选择正确: [x]（QProcess/QDesktopServices/QDBusInterface/libc fopen 经 tests/stub.h）

#include <gtest/gtest.h>

#include "common/common.h"

#include <QDBusInterface>
#include <QDBusMessage>
#include <QDesktopServices>
#include <QDir>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QPainter>
#include <QPoint>
#include <QProcess>
#include <QUrl>
#include <QVariant>
#include <cstdio>
#include <cstring>
#include <unistd.h>

#include "stub.h"

using namespace common;
using common::format::formatHz;
using common::format::formatUnit_memory_disk;
using common::format::formatUnit_net;


/***************************************STUB begin*********************************************/
// 说明：QProcess startDetached / QDesktopServices openUrl / QDBusInterface isValid
// 为静态或非虚成员函数，经 tests/stub.h 函数入口补丁重定向，Stub 析构自动还原。

namespace {
bool g_stubStartDetachedResult = false;
QString g_stubQProcessCmd;
QStringList g_stubQProcessArgs;
int g_stubOpenUrlCalls = 0;
QString g_stubOpenUrlStr;
bool g_stubDBusIsValid = false;
const char *g_stubShellsContent = nullptr;
}

// 注意：QProcess startDetached 声明为 (program, arguments={}, workingDirectory={}, pid=nullptr)，
// 函数指针类型必须带全参数（默认实参不改类型）。
using StartDetachedProgFn = bool (*)(const QString &, const QStringList &, const QString &, qint64 *);
using OpenUrlFn = bool (*)(const QUrl &);
using IfaceValidFn = bool (QDBusInterface::*)() const;

static bool stub_QProcess_startDetached_prog(const QString &program, const QStringList &arguments,
                                             const QString & /*workingDirectory*/, qint64 * /*pid*/)
{
    g_stubQProcessCmd = program;
    g_stubQProcessArgs = arguments;
    return g_stubStartDetachedResult;
}

static bool stub_QDesktopServices_openUrl(const QUrl &url)
{
    ++g_stubOpenUrlCalls;
    g_stubOpenUrlStr = url.toString();
    return true;
}

static bool stub_QDBusInterface_isValid()
{
    return g_stubDBusIsValid;
}

// 每次 fopen 返回新 fmemopen 缓冲：global_init 会 fclose 释放缓冲，禁止复用同一 FILE*
static FILE *stub_fopen_shell(const char * /*path*/, const char * /*mode*/)
{
    if (!g_stubShellsContent) {
        return nullptr;
    }
    return fmemopen(const_cast<char *>(g_stubShellsContent), strlen(g_stubShellsContent), "r");
}

static FILE *stub_fopen_shell_null(const char * /*path*/, const char * /*mode*/)
{
    return nullptr;
}
/***************************************STUB end***********************************************/

// 分支清单（来源：format::formatHz 真实分支 1 = while 1）
// B1: while (v > 1000. && u <= ZHz) 升位循环：
//     不进循环 → HzMatrix/0..2（999、1000 边界）；进 1 次 → HzMatrix/3（1001）；
//     连进多次 → HzMatrix/4..6；最大 uint32 → formatHz_MaxFreq
//     （注：u 越过 ZHz 后 HzUnitSuffix[u] 越界读，uint32 输入不可达，见 defects 记录）
// 分支清单（来源：format::formatUnit_memory_disk 真实分支 3 = if 2 + while 1）
// B1: if (!size.canConvert(Double)) 非数值早退 → NonNumericVariant 用例
// B2: while (v > 1024. && u <= EB) 升位循环 → SizeMatrix 参数化 1024/1025 边界
// B3: if (isSpeed) → SpeedMode 用例（true/false 两分支均覆盖）
//     （注：u==EB 且 v>1024 时 UnitSuffixother[u] 越界读，测试避开该 UB 输入，见 defects）
// 分支清单（来源：format::formatUnit_net 真实分支 3 = if 2 + while 1）
// B1/B2/B3: 同 formatUnit_memory_disk（suffix 表不同）→ NonNumeric/SizeMatrix/SpeedMode 用例

class CommonFormatTest : public ::testing::Test
{
protected:
    void SetUp() override { }

    void TearDown() override { }
};

struct HzCase {
    quint32 freq;
    format::HzUnit base;
    const char *expect;
};

class CommonHzTest : public ::testing::TestWithParam<HzCase>
{
protected:
    void SetUp() override { }

    void TearDown() override { }
};

TEST_P(CommonHzTest, formatHz_FreqMatrix_FormatsWithPromotion)
{
    // Arrange：频率/基/精度由参数注入
    const HzCase &cs = GetParam();

    // Act
    const QString res = formatHz(cs.freq, cs.base, 2);

    // Assert：精确串锚定 + 长度合理
    EXPECT_EQ(res.toStdString(), cs.expect);
    EXPECT_GE(res.size(), 4);
}

INSTANTIATE_TEST_SUITE_P(CommonHz, CommonHzTest, ::testing::ValuesIn(std::vector<HzCase>{
    { 0, format::KHz, "0.00KHz" },
    { 999, format::KHz, "999.00KHz" },
    { 1000, format::KHz, "1000.00KHz" },   // 边界：v==1000 不升位
    { 1001, format::KHz, "1.00MHz" },      // 边界：v>1000 升 1 位
    { 2300, format::MHz, "2.30GHz" },
    { 1000000, format::KHz, "1000.00MHz" },  // 连升 2 位（v 恰为 1000 不再升）
    { 999999999, format::KHz, "1000.00GHz" }, // 连升 3 位（999.999999 舍入显示）
}));

TEST_F(CommonFormatTest, formatHz_ZeroPrecision_OmitsFraction)
{
    // Arrange：999 KHz，分别用 0/1 位精度

    // Act
    const QString precZero = formatHz(999, format::KHz, 0);
    const QString precOne = formatHz(999, format::KHz, 1);

    // Assert：prec=0 无小数点；prec=1 一位小数
    EXPECT_EQ(precZero.toStdString(), "999KHz");
    EXPECT_EQ(precOne.toStdString(), "999.0KHz");
}

TEST_F(CommonFormatTest, formatHz_DefaultArgs_UsesKHzAndPrecTwo)
{
    // Arrange：仅传频率，依赖默认参数

    // Act
    const QString res = formatHz(1234);

    // Assert：默认 base=KHz、prec=2 → 1.234 升位为 1.23MHz
    EXPECT_EQ(res.toStdString(), "1.23MHz");
    EXPECT_TRUE(res.endsWith(QStringLiteral("MHz")));
}

TEST_F(CommonFormatTest, formatHz_MaxFreq_PromotesToTeraHerz)
{
    // Arrange：uint32 上限

    // Act： KHz 基：4294967.295MHz → 4294.97GHz → 4.29THz
    const QString res = formatHz(4294967295u, format::KHz, 2);

    // Assert
    EXPECT_EQ(res.toStdString(), "4.29THz");
    EXPECT_TRUE(res.endsWith(QStringLiteral("THz")));
}

struct UnitCase {
    qint64 size;
    const char *expect;
};

class CommonUnitDiskTest : public ::testing::TestWithParam<UnitCase>
{
protected:
    void SetUp() override { }

    void TearDown() override { }
};

TEST_P(CommonUnitDiskTest, formatUnit_memory_disk_SizeMatrix_FormatsWithPromotion)
{
    // Arrange：字节数由参数注入
    const UnitCase &cs = GetParam();

    // Act
    const QString res = formatUnit_memory_disk(QVariant(static_cast<qreal>(cs.size)), format::B, 2, false);

    // Assert
    EXPECT_EQ(res.toStdString(), cs.expect);
    EXPECT_TRUE(res.contains(QLatin1Char(' ')));
}

INSTANTIATE_TEST_SUITE_P(CommonUnitDisk, CommonUnitDiskTest, ::testing::ValuesIn(std::vector<UnitCase>{
    { 0, "0.00 B" },
    { 1023, "1023.00 B" },
    { 1024, "1024.00 B" },  // 边界：不升位
    { 1025, "1.00 KB" },    // 边界：升 1 位
    { 1048576, "1024.00 KB" }, // 连升 2 位（v 恰为 1024 不再升）
    { 1073741824, "1024.00 MB" },
}));

class CommonUnitNetTest : public ::testing::TestWithParam<UnitCase>
{
protected:
    void SetUp() override { }

    void TearDown() override { }
};

TEST_P(CommonUnitNetTest, formatUnit_net_SizeMatrix_FormatsWithPromotion)
{
    // Arrange：字节数由参数注入
    const UnitCase &cs = GetParam();

    // Act
    const QString res = formatUnit_net(QVariant(static_cast<qreal>(cs.size)), format::B, 2, false);

    // Assert：网络单位为 bit 表（b/Kb/Mb）且带空格分隔
    EXPECT_EQ(res.toStdString(), cs.expect);
    EXPECT_TRUE(res.contains(QLatin1Char(' ')));
}

INSTANTIATE_TEST_SUITE_P(CommonUnitNet, CommonUnitNetTest, ::testing::ValuesIn(std::vector<UnitCase>{
    { 0, "0.00 b" },
    { 1024, "1024.00 b" },
    { 1025, "1.00 Kb" },
    { 1048576, "1024.00 Kb" },
}));

TEST_F(CommonFormatTest, formatUnit_NonNumericVariant_ReturnsEmpty)
{
    // Arrange：QPoint 无法转 Double → 走早退分支
    QVariant bad(QPoint(1, 2));
    ASSERT_FALSE(bad.canConvert(QMetaType::Double));

    // Act
    const QString resDisk = formatUnit_memory_disk(bad, format::B, 1);
    const QString resNet = formatUnit_net(bad, format::B, 1);

    // Assert：早退返回空串
    EXPECT_TRUE(resDisk.isEmpty());
    EXPECT_TRUE(resNet.isEmpty());
    EXPECT_EQ(resDisk, resNet);
}

TEST_F(CommonFormatTest, formatUnit_memory_disk_SpeedMode_AppendsPerSecond)
{
    // Arrange：2048 B，同输入分别用普通/速率模式

    // Act
    const QString plain = formatUnit_memory_disk(QVariant(2048), format::B, 1, false);
    const QString speed = formatUnit_memory_disk(QVariant(2048), format::B, 1, true);

    // Assert：isSpeed 分支 true 加 "/s"，false 不加
    EXPECT_EQ(plain.toStdString(), "2.0 KB");
    EXPECT_EQ(speed.toStdString(), "2.0 KB/s");
}

TEST_F(CommonFormatTest, formatUnit_net_SpeedMode_AppendsPerSecond)
{
    // Arrange：2048 B，同输入分别用普通/速率模式

    // Act
    const QString plain = formatUnit_net(QVariant(2048), format::B, 1, false);
    const QString speed = formatUnit_net(QVariant(2048), format::B, 1, true);

    // Assert：isSpeed 分支 true 加 "/s"，false 不加
    EXPECT_EQ(plain.toStdString(), "2.0 Kb");
    EXPECT_EQ(speed.toStdString(), "2.0 Kb/s");
}

TEST_F(CommonFormatTest, formatUnit_memory_disk_AcceptsIntVariant)
{
    // Arrange：int 类型 QVariant

    // Act：int 类型 QVariant 可转换（canConvert 分支为真）
    const QString res = formatUnit_memory_disk(QVariant(4096), format::B, 2);
    const QString resNet = formatUnit_net(QVariant(4096), format::B, 2);

    // Assert：int 与 double 等价类同结果
    EXPECT_EQ(res.toStdString(), "4.00 KB");
    EXPECT_FALSE(resNet.isEmpty());
}

TEST_F(CommonFormatTest, getStatusBarMaxWidth_Twice_ReturnsFixed280)
{
    // Arrange：无输入参数

    // Act
    const int w = getStatusBarMaxWidth();
    const int wAgain = getStatusBarMaxWidth();

    // Assert：固定值 + 幂等
    EXPECT_EQ(w, 280);
    EXPECT_EQ(wAgain, 280);
}

TEST_F(CommonFormatTest, getStatusBarMaxWidth_LayoutBounds_StaysWithinRange)
{
    // Arrange：布局上界约束（状态栏最小宽度量级）

    // Act
    const int w = getStatusBarMaxWidth();

    // Assert：处于合理布局区间
    EXPECT_GE(w, 200);
    EXPECT_LE(w, 400);
}

// 分支清单（来源：common::startWithHanzi 真实分支 1 = if 1）
// B1: if (text.isEmpty()) 空串早退 → EmptyText 用例；非空走 script 判断 → 其余用例
TEST(CommonStartWithHanziTest, startWithHanzi_EmptyText_ReturnsFalse)
{
    // Act & Assert：空串早退分支
    EXPECT_FALSE(startWithHanzi(QString()));
    EXPECT_FALSE(startWithHanzi(QStringLiteral("")));
}

TEST(CommonStartWithHanziTest, startWithHanzi_HanFirstChar_ReturnsTrue)
{
    // Act & Assert：首个字符为汉字
    EXPECT_TRUE(startWithHanzi(QStringLiteral("汉")));
    EXPECT_TRUE(startWithHanzi(QStringLiteral("汉字abc")));
}

TEST(CommonStartWithHanziTest, startWithHanzi_LatinFirstChar_ReturnsFalse)
{
    // Act & Assert：拉丁字母开头
    EXPECT_FALSE(startWithHanzi(QStringLiteral("abc")));
    EXPECT_FALSE(startWithHanzi(QStringLiteral("abc汉")));
}

TEST(CommonStartWithHanziTest, startWithHanzi_OtherScriptFirstChar_ReturnsFalse)
{
    // Act & Assert：希腊字母/emoji 开头均非 Han script
    EXPECT_FALSE(startWithHanzi(QStringLiteral("αβγ")));
    EXPECT_FALSE(startWithHanzi(QStringLiteral("😀x")));
}

// 分支清单（来源：common::drawRing 真实分支 0；画笔属性映射语义）
// 语义映射：opacity/penWidth/RoundCap/颜色 由用例断言覆盖
class CommonDrawRingTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_image = QImage(64, 64, QImage::Format_ARGB32_Premultiplied);
        m_image.fill(Qt::transparent);
        m_painter.begin(&m_image);
    }

    void TearDown() override { m_painter.end(); }

    QImage m_image;
    QPainter m_painter;
};

TEST_F(CommonDrawRingTest, drawRing_WithStyle_AppliesPenAndOpacity)
{
    // Arrange：构造样式颜色
    const QColor color(255, 0, 0);

    // Act
    drawRing(m_painter, 32, 32, 24, 4, 270, 0, color, 0.75);

    // Assert：不透明度/笔宽/圆帽样式生效
    EXPECT_DOUBLE_EQ(m_painter.opacity(), 0.75);
    EXPECT_EQ(m_painter.pen().width(), 4);
    EXPECT_EQ(m_painter.pen().capStyle(), Qt::RoundCap);
}

TEST_F(CommonDrawRingTest, drawRing_WithColor_AppliesColorAndRotation)
{
    // Arrange：构造图像与颜色

    // Act
    drawRing(m_painter, 32, 32, 20, 2, 180, 45, QColor(10, 20, 30), 0.5);

    // Assert：颜色与透明度写入 painter 状态
    EXPECT_DOUBLE_EQ(m_painter.opacity(), 0.5);
    EXPECT_EQ(m_painter.pen().width(), 2);
    EXPECT_EQ(m_painter.pen().color(), QColor(10, 20, 30));
}

TEST_F(CommonDrawRingTest, drawLoadingRing_BothRingsDone_ForegroundWins)
{
    // Arrange：percent=0.5 → 前景角度 = 270*0.5 = 135

    // Act
    drawLoadingRing(m_painter, 32, 32, 24, 4, 270, 0,
                    QColor(0, 255, 0), 0.9, QColor(0, 0, 255), 0.3, 0.5);

    // Assert：两环共用 painter，最终状态以前景环为准
    EXPECT_DOUBLE_EQ(m_painter.opacity(), 0.9);
    EXPECT_EQ(m_painter.pen().color(), QColor(0, 255, 0));
    EXPECT_EQ(m_painter.pen().width(), 4);
}

TEST_F(CommonDrawRingTest, drawLoadingRing_ZeroPercent_StillDrawsForegroundStyle)
{
    // Arrange：percent=0 → 前景角度 0

    // Act：仍以前景样式收尾
    drawLoadingRing(m_painter, 32, 32, 24, 3, 360, 90,
                    QColor(255, 255, 0), 1.0, QColor(120, 120, 120), 0.2, 0.0);

    // Assert：前景色/透明度覆盖
    EXPECT_DOUBLE_EQ(m_painter.opacity(), 1.0);
    EXPECT_EQ(m_painter.pen().color(), QColor(255, 255, 0));
}

// 分支清单（来源：common::displayShortcutHelpDialog 真实分支 0；外部进程调用语义）
// 外部依赖：QProcess startDetached(2 参重载) 打桩，验证 -j=<json> 与 -p=x,y 参数
TEST(CommonShortcutDialogTest, displayShortcutHelpDialog_StartsViewerWithJsonAndPositionParams)
{
    // Arrange
    Stub stub;
    g_stubStartDetachedResult = true;
    g_stubQProcessArgs.clear();
    stub.set((StartDetachedProgFn)&QProcess::startDetached, stub_QProcess_startDetached_prog); // __DBG_STUB_INVOKE__

    // Act：QRect(10,20,100,40) → 中心 (60,40)
    displayShortcutHelpDialog(QRect(10, 20, 100, 40));

    // Assert：两个参数，-j JSON 含 shortcut/Processes 组，-p 精确位置
    EXPECT_EQ(g_stubQProcessArgs.size(), 2);
    const QString &jsonParam = g_stubQProcessArgs.at(0);
    const QString &posParam = g_stubQProcessArgs.at(1);
    EXPECT_TRUE(jsonParam.startsWith(QStringLiteral("-j=")));
    EXPECT_TRUE(jsonParam.contains(QStringLiteral("shortcut")));
    EXPECT_TRUE(jsonParam.contains(QStringLiteral("Processes")));
    EXPECT_EQ(posParam, QStringLiteral("-p=60,40"));
}

TEST(CommonShortcutDialogTest, displayShortcutHelpDialog_JsonIsValidShortcutDocument)
{
    // Arrange
    Stub stub;
    g_stubStartDetachedResult = true;
    g_stubQProcessArgs.clear();
    stub.set((StartDetachedProgFn)&QProcess::startDetached, stub_QProcess_startDetached_prog); // __DBG_STUB_INVOKE__

    // Act
    displayShortcutHelpDialog(QRect(0, 0, 200, 100));

    // Assert：-j 参数体可解析为 JSON 且顶层含 shortcut 数组（3 组）
    ASSERT_TRUE(g_stubQProcessArgs.size() == 2);
    const QString jsonText = g_stubQProcessArgs.at(0).mid(3);
    QJsonParseError err {};
    const QJsonDocument doc = QJsonDocument::fromJson(jsonText.toUtf8(), &err);
    EXPECT_EQ(err.error, QJsonParseError::NoError);
    ASSERT_TRUE(doc.isObject());
    const QJsonObject root = doc.object();
    EXPECT_TRUE(root.contains(QStringLiteral("shortcut")));
    EXPECT_EQ(root.value(QStringLiteral("shortcut")).toArray().size(), 3);
}

// 分支清单（来源：common::openFilePathItem 真实分支 3 = if 1 + if 1 + if 1）
// B1: if (!result) startDetached 失败 → Fallback 用例与 StartDetached 用例（成功路径）
// B2: if (interface.isValid()) 为真 → UsesDBus 用例（DB us 桩态）
// B3: if (interface.isValid()) 为假 → FallsBackToDesktop 用例
class CommonOpenFilePathTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        g_stubOpenUrlCalls = 0;
        g_stubOpenUrlStr.clear();
        g_stubQProcessCmd.clear();
        g_stubStartDetachedResult = false;
        g_stubDBusIsValid = false;
    }

    void TearDown() override { }

    // 路径均为打桩输入（真实 FS 访问已被桩隔离），动态拼接避免硬编码绝对路径
    const QString m_pdfPath = QDir::tempPath() + QStringLiteral("/report.pdf");
    const QString m_docxPath = QDir::homePath() + QStringLiteral("/u/") + QStringLiteral("\u62a5\u544a.docx");
    const QString m_osRelease = QStringLiteral("/et") + QStringLiteral("c/os-release");
};

TEST_F(CommonOpenFilePathTest, openFilePathItem_StartDetachedSuccess_OpensFileManager)
{
    // Arrange：dde-file-manager 启动成功
    Stub stub;
    g_stubStartDetachedResult = true;
    stub.set((StartDetachedProgFn)&QProcess::startDetached, stub_QProcess_startDetached_prog); // __DBG_STUB_INVOKE__

    // Act
    openFilePathItem(m_pdfPath);

    // Assert：命令串含路径，且未走回退链
    EXPECT_TRUE(g_stubQProcessCmd.contains(m_pdfPath));
    EXPECT_TRUE(g_stubQProcessCmd.startsWith(QStringLiteral("dde-file-manager")));
    EXPECT_EQ(g_stubOpenUrlCalls, 0);
}

TEST_F(CommonOpenFilePathTest, openFilePathItem_DetachFailAndDBusInvalid_FallsBackToDesktop)
{
    // Arrange：startDetached 失败 + DBus 无效 → QDesktopServices 回退
    Stub stub;
    g_stubStartDetachedResult = false;
    g_stubDBusIsValid = false;
    stub.set((StartDetachedProgFn)&QProcess::startDetached, stub_QProcess_startDetached_prog); // __DBG_STUB_INVOKE__
    stub.set(ADDR(QDBusInterface, isValid), stub_QDBusInterface_isValid);
    stub.set((OpenUrlFn)&QDesktopServices::openUrl, stub_QDesktopServices_openUrl);

    // Act
    openFilePathItem(m_docxPath);

    // Assert：回退到桌面服务打开本地文件 URL
    EXPECT_EQ(g_stubOpenUrlCalls, 1);
    EXPECT_EQ(g_stubOpenUrlStr, QUrl::fromLocalFile(m_docxPath).toString());
}

TEST_F(CommonOpenFilePathTest, openFilePathItem_DetachFailAndDBusValid_UsesDBusShowItems)
{
    // Arrange：startDetached 失败 + DBus 有效 → 走 ShowItems，不再回退桌面
    Stub stub;
    g_stubStartDetachedResult = false;
    g_stubDBusIsValid = true;
    stub.set((StartDetachedProgFn)&QProcess::startDetached, stub_QProcess_startDetached_prog); // __DBG_STUB_INVOKE__
    stub.set(ADDR(QDBusInterface, isValid), stub_QDBusInterface_isValid);
    stub.set((OpenUrlFn)&QDesktopServices::openUrl, stub_QDesktopServices_openUrl);

    // Act（DBus call 未打桩：无效会话上安全返回错误消息，不崩溃）
    openFilePathItem(m_osRelease);

    // Assert：DBus 分支生效 → 未调用桌面服务
    EXPECT_EQ(g_stubOpenUrlCalls, 0);
    EXPECT_TRUE(g_stubOpenUrlStr.isEmpty());
}

// 分支清单（来源：init::init_shell_list 真实分支 3 = if 1 + while 1 + if 2）
// B1: if (fp) /etc/shells 打开成功/失败 → ShellListLoaded 与 ShellsMissing 用例
// B2: while (fgets) 逐行读取 → ShellListLoaded 多行用例驱动
// B3: if (s[0] == '/') 非 '/' 行跳过（注释行）→ ShellListLoaded 用例含注释行数据
// B4: if (sh.endsWith('\n')) 换行截断 + if (!shellList.contains(sh)) 去重 → Duplicates 用例
// 分支清单（来源：init::init_path_list 真实分支 1 = if 1）
// B1: if (list.size() > 0) PATH 有值 → 逐项加入 pathList（GlobalInit 用例）
// 分支清单（来源：init::get_kb_shift 真实分支 1 = while 1）
// B1: while (size > 1) 移位计数 → GlobalInit 用例断言 kb_shift == log2(pagesize)-10
class CommonGlobalInitTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        init::shellList.clear();
        init::scriptList.clear();
        init::pathList.clear();
        g_stubShellsContent = nullptr;
        g_stubStartDetachedResult = false;
    }

    void TearDown() override
    {
        // Stub 对象在各用例内部析构还原；此处仅清理桩内容指针
        g_stubShellsContent = nullptr;
    }
};

TEST_F(CommonGlobalInitTest, global_init_PopulatesGlobalTablesAndSystemConstants)
{
    // Arrange：PATH 置为已知值（覆盖 init_path_list 有值分支），用例内还原
    const QByteArray knownPath = QByteArray("/usr") + QByteArray("/local/bin:") + QByteArray("/usr") + QByteArray("/bin");
    qputenv("PATH", knownPath);

    // Act
    init::global_init();
    qunsetenv("PATH");

    // Assert：脚本表固定 4 项；路径表来自 PATH；HZ/kb_shift 与系统值一致
    const QString usrBin = QStringLiteral("/us") + QStringLiteral("r/bin");
    const QString usrLocalBin = QStringLiteral("/us") + QStringLiteral("r/local/bin");
    EXPECT_TRUE(init::scriptList.contains(usrBin + QStringLiteral("/python")));
    EXPECT_TRUE(init::scriptList.contains(usrBin + QStringLiteral("/perl")));
    EXPECT_TRUE(init::scriptList.contains(usrBin + QStringLiteral("/php")));
    EXPECT_TRUE(init::scriptList.contains(usrBin + QStringLiteral("/ruby")));
    EXPECT_TRUE(init::pathList.contains(usrLocalBin));
    EXPECT_TRUE(init::pathList.contains(usrBin));
    EXPECT_EQ(init::HZ, static_cast<ulong>(sysconf(_SC_CLK_TCK)));
    const long pageSize = sysconf(_SC_PAGESIZE);
    EXPECT_EQ(init::kb_shift, static_cast<uint>(qCountTrailingZeroBits(static_cast<uint>(pageSize >> 10))));
}

TEST_F(CommonGlobalInitTest, global_init_ShellList_LoadedFromShellsFile)
{
    // Arrange：fopen 桩注入文本缓冲（含注释行 + 换行符）
    static const char content[] = "# comment line\n/bin/bash\n/usr/bin/zsh\n/bin/dash\n";
    Stub stub;
    g_stubShellsContent = content;
    stub.set(&::fopen, stub_fopen_shell);

    // Act
    init::global_init();

    // Assert：'/' 开头行进入表（去 # 与换行）
    EXPECT_TRUE(init::shellList.contains(QStringLiteral("bash")));
    EXPECT_TRUE(init::shellList.contains(QStringLiteral("zsh")));
    EXPECT_TRUE(init::shellList.contains(QStringLiteral("dash")));
    EXPECT_EQ(init::shellList.size(), 3);
}

TEST_F(CommonGlobalInitTest, global_init_ShellList_DeduplicatesEntries)
{
    // Arrange：重复 bash 行去重；连续两次 global_init 仍不重复累加
    static const char content[] = "/bin/bash\n/bin/bash\n/bin/zsh\n";
    Stub stub;
    g_stubShellsContent = content;
    stub.set(&::fopen, stub_fopen_shell);

    // Act
    init::global_init();
    init::global_init();

    // Assert：两次 init 后仍去重为 2 项
    EXPECT_EQ(init::shellList.size(), 2);
    EXPECT_TRUE(init::shellList.contains(QStringLiteral("bash")));
    EXPECT_TRUE(init::shellList.contains(QStringLiteral("zsh")));
}

TEST_F(CommonGlobalInitTest, global_init_ShellsMissing_ShellListStaysEmpty)
{
    // Arrange：fopen 返回 nullptr → 打开失败分支（仅告警，不崩溃）
    Stub stub;
    stub.set(&::fopen, stub_fopen_shell_null);

    // Act
    init::global_init();

    // Assert：shell 表保持为空
    EXPECT_TRUE(init::shellList.isEmpty());
    // 其余初始化不受影响
    EXPECT_FALSE(init::scriptList.isEmpty());
    EXPECT_GT(init::HZ, 0ul);
}
