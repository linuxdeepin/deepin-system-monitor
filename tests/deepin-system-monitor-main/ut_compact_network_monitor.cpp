// Copyright (C) 2019-2026 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "compact_network_monitor.h"

//dependence
#include "system/net_info.h"
#include "system/device_db.h"
#include "system/system_monitor.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

//qt
#include <QPainterPath>
#include <QMouseEvent>
#include <QSignalSpy>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <DApplicationHelper>
DWIDGET_USE_NAMESPACE
using ThemeHelper = DApplicationHelper;
#else
#include <DGuiApplicationHelper>
DGUI_USE_NAMESPACE
using ThemeHelper = DGuiApplicationHelper;
#endif

using namespace core::system;

/***************************************STUB begin*********************************************/
namespace {
// 与 ut_network_monitor 相同策略：打桩隔离 DeviceDB/NetInfo 真实数据源
static qreal s_recvBps = 0.0;
static qreal s_sentBps = 0.0;
static qulonglong s_totalRecvBytes = 0;
static qulonglong s_totalSentBytes = 0;

static SystemMonitor *stub_SystemMonitor_instance()
{
    return nullptr;  // 阻止构造重型 SystemMonitor 单例
}

static DeviceDB *stub_DeviceDB_instance()
{
    return nullptr;  // netInfo() 已打桩，不会解引用 this
}

static NetInfo *stub_DeviceDB_netInfo(DeviceDB *)
{
    static NetInfo fakeNetInfo;
    return &fakeNetInfo;
}

static qreal stub_NetInfo_recvBps(NetInfo *)
{
    return s_recvBps;
}

static qreal stub_NetInfo_sentBps(NetInfo *)
{
    return s_sentBps;
}

static qulonglong stub_NetInfo_totalRecvBytes(NetInfo *)
{
    return s_totalRecvBytes;
}

static qulonglong stub_NetInfo_totalSentBytes(NetInfo *)
{
    return s_totalSentBytes;
}
}  // namespace
/***************************************STUB end***********************************************/

static const int kPointsNumber = 30;  // compact_network_monitor.cpp 内 const int pointsNumber = 30

// 分支清单（源自 compact_network_monitor.cpp）:
// 1. ctor: 固定宽 280、固定高 150、双速率列表各 pointsNumber+1 项
// 2. changeTheme: 无分支（themeType 被 Q_UNUSED），统一取调色板设置 textColor/summaryColor/m_frameColor(alphaF=0.3)
// 3. changeFont: contentFont=Medium/-1ptF、subContentFont=-1ptF
// 4. updateStatus: 追加速率 -> size>pointsNumber+1 时 pop_front -> 构建 download/uploadPath -> update()
// 5. getPainterPathByData: for i<dataCount-1 -> y=renderMaxHeight*v/maxVlaue
// 6. mouseReleaseEvent: LeftButton -> emit clicked("MSG_NET") | 其它按键不发射
// 7. mouseMoveEvent: 直接返回（不发射、不崩溃）
// 8. paintEvent: 完整绘制路径（grab 触发）

class CompactNetworkMonitorTest : public ::testing::Test
{
public:
    CompactNetworkMonitorTest() : m_tester(nullptr) {}

    virtual void SetUp() override
    {
        // Arrange: 全量打桩，隔离 SystemMonitor/DeviceDB 真实数据源
        m_stub.set(ADDR(SystemMonitor, instance), stub_SystemMonitor_instance);
        m_stub.set(ADDR(DeviceDB, instance), stub_DeviceDB_instance);
        m_stub.set(ADDR(DeviceDB, netInfo), stub_DeviceDB_netInfo);
        m_stub.set(ADDR(NetInfo, recvBps), stub_NetInfo_recvBps);
        m_stub.set(ADDR(NetInfo, sentBps), stub_NetInfo_sentBps);
        m_stub.set(ADDR(NetInfo, totalRecvBytes), stub_NetInfo_totalRecvBytes);
        m_stub.set(ADDR(NetInfo, totalSentBytes), stub_NetInfo_totalSentBytes);

        s_recvBps = 0.0;
        s_sentBps = 0.0;
        s_totalRecvBytes = 0;
        s_totalSentBytes = 0;

        // Act
        m_tester = new CompactNetworkMonitor();
        // Assert: 构造不崩溃
        ASSERT_NE(m_tester, nullptr);
    }

    virtual void TearDown() override
    {
        delete m_tester;
        m_tester = nullptr;
    }

protected:
    CompactNetworkMonitor *m_tester;
    Stub m_stub;
};

// ───────────────────────── ctor ─────────────────────────
TEST_F(CompactNetworkMonitorTest, Construction_DefaultInit_FixedSizeAndSpeedsListInitialized)
{
    // Arrange: SetUp 中已完成打桩（隔离重型单例）并构造控件
    // Act: 读取构造后的固定尺寸与速率列表状态
    const int fixedWidth = m_tester->width();
    const int fixedHeight = m_tester->height();
    const int dlSize = m_tester->downloadSpeeds->size();
    const int ulSize = m_tester->uploadSpeeds->size();
    const double dlFirst = m_tester->downloadSpeeds->first();
    const double ulFirst = m_tester->uploadSpeeds->first();
    // Assert: 宽度 = 280，高度 = 150
    EXPECT_EQ(fixedWidth, 280);
    EXPECT_EQ(fixedHeight, 150);
    // Assert: 两个速率列表均初始化为 pointsNumber+1 个 0 值
    EXPECT_NE(m_tester->downloadSpeeds, nullptr);
    EXPECT_NE(m_tester->uploadSpeeds, nullptr);
    EXPECT_EQ(dlSize, kPointsNumber + 1);
    EXPECT_EQ(ulSize, kPointsNumber + 1);
    EXPECT_DOUBLE_EQ(dlFirst, 0.0);
    EXPECT_DOUBLE_EQ(ulFirst, 0.0);
}

// ───────────────────────── updateStatus ─────────────────────────
TEST_F(CompactNetworkMonitorTest, UpdateStatus_WhenInvoked_ReadsNetInfoAndAppendsSpeeds)
{
    // Arrange: 打桩返回受控网络数据
    s_recvBps = 1024.0;
    s_sentBps = 2048.0;
    s_totalRecvBytes = 100000;
    s_totalSentBytes = 200000;

    // Act
    m_tester->updateStatus();

    // Assert: 私有状态被刷新为打桩值
    EXPECT_DOUBLE_EQ(m_tester->m_recvBps, 1024.0);
    EXPECT_DOUBLE_EQ(m_tester->m_sentBps, 2048.0);
    EXPECT_EQ(m_tester->m_totalRecvBytes, 100000u);
    EXPECT_EQ(m_tester->m_totalSentBytes, 200000u);
    // Assert: 列表从初始 31 项 -> 追加 1 项(32) 超界 -> pop 1 项 = 31 项
    EXPECT_EQ(m_tester->downloadSpeeds->size(), kPointsNumber + 1);
    EXPECT_EQ(m_tester->uploadSpeeds->size(), kPointsNumber + 1);
    EXPECT_DOUBLE_EQ(m_tester->downloadSpeeds->last(), 1024.0);
    EXPECT_DOUBLE_EQ(m_tester->uploadSpeeds->last(), 2048.0);
}

TEST_F(CompactNetworkMonitorTest, UpdateStatus_WhenListAtBoundary_PopsOldestEntry)
{
    // Arrange: 清空列表后填满 pointsNumber+1 个可辨识旧值（31 项，处于阈值边界）
    m_tester->downloadSpeeds->clear();
    for (int i = 0; i <= kPointsNumber; ++i)
        m_tester->downloadSpeeds->append(i * 1.0);
    const double oldestExpectedAfterPop = m_tester->downloadSpeeds->at(1);
    s_recvBps = 512.0;

    // Act: 追加后变为 32 > pointsNumber+1，应 pop 掉最老一项
    m_tester->updateStatus();

    // Assert: 窗口长度维持 pointsNumber+1，且最老一项（值为 0）已被移除
    EXPECT_EQ(m_tester->downloadSpeeds->size(), kPointsNumber + 1);
    EXPECT_DOUBLE_EQ(m_tester->downloadSpeeds->first(), oldestExpectedAfterPop);
    EXPECT_DOUBLE_EQ(m_tester->downloadSpeeds->last(), 512.0);
}

TEST_F(CompactNetworkMonitorTest, UpdateStatus_WhenListBelowBoundary_KeepsAllEntries)
{
    // Arrange: 列表长度(5)小于 pointsNumber+1，不触发 pop 分支
    m_tester->downloadSpeeds->clear();
    m_tester->downloadSpeeds->append(1.0);
    m_tester->downloadSpeeds->append(2.0);
    m_tester->downloadSpeeds->append(3.0);
    m_tester->downloadSpeeds->append(4.0);
    m_tester->downloadSpeeds->append(5.0);
    m_tester->uploadSpeeds->clear();
    m_tester->uploadSpeeds->append(1.0);
    m_tester->uploadSpeeds->append(2.0);
    m_tester->uploadSpeeds->append(3.0);
    m_tester->uploadSpeeds->append(4.0);
    m_tester->uploadSpeeds->append(5.0);

    // Act
    m_tester->updateStatus();

    // Assert: 全部保留，仅各自追加 1 项
    EXPECT_EQ(m_tester->downloadSpeeds->size(), 6);
    EXPECT_EQ(m_tester->uploadSpeeds->size(), 6);
}

TEST_F(CompactNetworkMonitorTest, UpdateStatus_WhenInvoked_BuildsDownloadAndUploadPaths)
{
    // Arrange
    s_recvBps = 640.0;
    s_sentBps = 320.0;

    // Act
    m_tester->updateStatus();

    // Assert: 路径由 31 个数据点生成：隐式 moveTo + 30 段 cubicTo（每段 3 元素）= 91
    EXPECT_EQ(m_tester->downloadPath.elementCount(), 1 + 3 * kPointsNumber);
    EXPECT_EQ(m_tester->uploadPath.elementCount(), 1 + 3 * kPointsNumber);
}

// ───────────────────────── getPainterPathByData ─────────────────────────
TEST_F(CompactNetworkMonitorTest, GetPainterPathByData_WithEmptyList_ReturnsEmptyPath)
{
    // Arrange: 空数据列表
    QList<double> emptyList;

    // Act
    QPainterPath path;
    m_tester->getPainterPathByData(&emptyList, path, 20.0);

    // Assert: 循环体不执行，路径为空
    EXPECT_TRUE(path.isEmpty());
    EXPECT_EQ(path.elementCount(), 0);
}

TEST_F(CompactNetworkMonitorTest, GetPainterPathByData_WithSinglePoint_ReturnsEmptyPath)
{
    // Arrange: 单点数据（dataCount-1 = 0，不进入循环）
    QList<double> single {42.0};

    // Act
    QPainterPath path;
    m_tester->getPainterPathByData(&single, path, 20.0);

    // Assert
    EXPECT_TRUE(path.isEmpty());
    EXPECT_EQ(path.elementCount(), 0);
}

TEST_F(CompactNetworkMonitorTest, GetPainterPathByData_WithFivePoints_GeneratesThirteenElements)
{
    // Arrange: 5 个点 -> 隐式 moveTo + 4 段 cubicTo
    QList<double> data {0.0, 0.25, 0.5, 0.75, 1.0};

    // Act
    QPainterPath path;
    m_tester->getPainterPathByData(&data, path, 1.0);

    // Assert: 元素数 = 1 + 3*4 = 13，隐式 moveTo 落在原点
    EXPECT_EQ(path.elementCount(), 13);
    EXPECT_DOUBLE_EQ(path.elementAt(0).x, 0.0);
}

// ───────────────────────── changeTheme ─────────────────────────
TEST_F(CompactNetworkMonitorTest, ChangeTheme_AnyType_UpdatesPaletteColors)
{
    // Arrange: changeTheme 无分支（themeType 被 Q_UNUSED），Light/Dark 结果一致
    // Act: 分别以亮色与暗色触发
    m_tester->changeTheme(ThemeHelper::LightType);
    const QColor frameAfterLight = m_tester->m_frameColor;
    m_tester->changeTheme(ThemeHelper::DarkType);

    // Assert: 两次刷新后取色一致且来自调色板，m_frameColor alphaF ≈ 0.3
    EXPECT_EQ(m_tester->m_frameColor, frameAfterLight);
    EXPECT_NEAR(m_tester->m_frameColor.alphaF(), 0.3, 1e-3);
    EXPECT_TRUE(m_tester->textColor.isValid());
    EXPECT_TRUE(m_tester->summaryColor.isValid());
}

// ───────────────────────── changeFont ─────────────────────────
TEST_F(CompactNetworkMonitorTest, ChangeFont_WithCustomFont_UpdatesContentAndSubContentFonts)
{
    // Arrange
    QFont font;
    font.setPointSizeF(9.0);

    // Act
    m_tester->changeFont(font);

    // Assert: content 字体 Medium 字重且 -1ptF
    EXPECT_EQ(m_tester->m_contentFont.weight(), QFont::Medium);
    EXPECT_DOUBLE_EQ(m_tester->m_contentFont.pointSizeF(), 8.0);
    // Assert: subContent 字体 -1ptF
    EXPECT_DOUBLE_EQ(m_tester->m_subContentFont.pointSizeF(), 8.0);
}

// ───────────────────────── mouseReleaseEvent ─────────────────────────
TEST_F(CompactNetworkMonitorTest, MouseReleaseEvent_LeftButton_EmitsClickedSignal)
{
    // Arrange
    QSignalSpy spy(m_tester, &CompactNetworkMonitor::clicked);
    QMouseEvent ev(QEvent::MouseButtonRelease, QPointF(10, 10),
                   Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

    // Act
    m_tester->mouseReleaseEvent(&ev);

    // Assert: 左键 -> 发射 clicked("MSG_NET")
    EXPECT_EQ(spy.count(), 1);
    ASSERT_EQ(spy.first().size(), 1);
    EXPECT_EQ(spy.first().first().toString(), QString("MSG_NET"));
}

TEST_F(CompactNetworkMonitorTest, MouseReleaseEvent_RightButton_DoesNotEmitSignal)
{
    // Arrange: 先按右键释放，再按左键释放，验证按键过滤逻辑有效
    QSignalSpy spy(m_tester, &CompactNetworkMonitor::clicked);
    QMouseEvent ev(QEvent::MouseButtonRelease, QPointF(10, 10),
                   Qt::RightButton, Qt::RightButton, Qt::NoModifier);

    // Act: 右键释放
    m_tester->mouseReleaseEvent(&ev);

    // Assert: 非左键 -> 不发射
    EXPECT_EQ(spy.count(), 0);

    // Act: 随后左键释放，验证信号连接本身有效
    QMouseEvent leftEv(QEvent::MouseButtonRelease, QPointF(10, 10),
                       Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    m_tester->mouseReleaseEvent(&leftEv);

    // Assert
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.first().first().toString(), QString("MSG_NET"));
}

// ───────────────────────── mouseMoveEvent ─────────────────────────
TEST_F(CompactNetworkMonitorTest, MouseMoveEvent_AnyMovement_IsIgnoredAndSilent)
{
    // Arrange: mouseMoveEvent 按设计屏蔽鼠标移动响应；随后以左键释放验证事件管线完好
    QSignalSpy spy(m_tester, &CompactNetworkMonitor::clicked);
    QMouseEvent ev(QEvent::MouseMove, QPointF(5, 5),
                   Qt::NoButton, Qt::LeftButton, Qt::NoModifier);

    // Act: 两次移动不应产生任何点击
    m_tester->mouseMoveEvent(&ev);
    m_tester->mouseMoveEvent(&ev);

    // Assert: 不崩溃且不产生任何点击信号
    EXPECT_EQ(spy.count(), 0);

    // Act: 左键释放
    QMouseEvent relEv(QEvent::MouseButtonRelease, QPointF(5, 5),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    m_tester->mouseReleaseEvent(&relEv);

    // Assert: 点击事件管线未被移动事件破坏
    EXPECT_EQ(spy.count(), 1);
}

// ───────────────────────── paintEvent ─────────────────────────
TEST_F(CompactNetworkMonitorTest, PaintEvent_AfterGrab_RendersPixmapWithWidgetSize)
{
    // Arrange: 注入受控数据后触发绘制
    s_recvBps = 1024.0;
    s_sentBps = 2048.0;
    s_totalRecvBytes = 12345;
    s_totalSentBytes = 6789;
    m_tester->updateStatus();

    // Act: 第一次 grab 触发 paintEvent，稳定尺寸后再次 grab
    const QPixmap firstPix = m_tester->grab();
    const QPixmap pix = m_tester->grab();

    // Assert: 渲染非空且尺寸与控件一致
    EXPECT_FALSE(firstPix.isNull());
    EXPECT_FALSE(pix.isNull());
    EXPECT_EQ(pix.size(), m_tester->size());
}
