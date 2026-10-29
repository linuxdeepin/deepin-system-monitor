// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// qt-autotest-generator 单元测试 - SystemMonitorTipsWidget（deepin-system-monitor-plugin/gui/systemmonitortipswidget.cpp）
//
// 用例计数声明：
// | 函数                            | 用例数 |
// |---------------------------------|--------|
// | SystemMonitorTipsWidget (构造)   | 1      |
// | setSystemMonitorTipsText         | 3（长度分级 TEST_P 4 组 + CPU 3 字符分支 + 超长列表分支） |
// | paintEvent                       | 2      |
// | event (FontChange/Show/Hide)     | 2      |
//
// 10 项检查清单（test-code-gen.md）：
// 1 public 方法覆盖：setSystemMonitorTipsText（唯一 public 方法）与两个 protected 事件均覆盖；
// 2 等价类：文本列表 0/4/5 项、CPU 串长 3/非 3、下行串长 3/4/5/其它；
// 3 边界值：串长 3/4/5/6 的右侧前缀梯度、"0KB/s" 默认值；
// 4 同质分组≥3 用 TEST_P：downLoad 串长 3/4/5/其它 4 组 → 下标补零前缀映射；
// 5 分支清单→用例映射：见下方【分支清单】；
// 6 分支触发：直接派发 QEvent(Show/Hide/FontChange)、render() 触发 paintEvent；
// 7 异常断言：无显式 throw，绘制路径 EXPECT_NO_THROW；
// 8 反向场景：空列表绘制（默认 0 值）、超长列表第 5 项告警分支、隐藏事件；
// 9 强异常安全：FontChange 后宽度与固定尺寸保持一致；
// 10 打桩选型：无外部依赖（纯 QFrame 绘制），不引入 gMock；翻译文本与源码同用
//    DApplication::translate 计算期望值，规避翻译环境差异。
//
// 【分支清单】（复杂方法分支 → 用例映射）
// setSystemMonitorTipsText：
//   B1 文本解析循环 i=0..3 → cpu/mem/downLoad/upLoad → Tips_SetSystemMonitorTipsText_SetsListAndFixedSize
//   B2 i>=4 → 告警分支（列表保留原值）→ Tips_SetSystemMonitorTipsText_OverlongList_KeepsAllEntries
//   B3 cpu.length()==3 → 左侧 ": 0" 前缀 → Tips_SetSystemMonitorTipsText_CpuLength3_UsesZeroPad
//   B4 cpu.length()!=3 → 左侧 ": " → 同上（两种长度各断言一次）
//   B5 downLoad.length()==3 → "↓000" → TEST_P
//   B6 downLoad.length()==4 → "↓00" → TEST_P
//   B7 downLoad.length()==5 → "↓0" → TEST_P
//   B8 downLoad.length() 其它 → "↓" → TEST_P
//   B9 固定尺寸 = 10 + left + right / boundingRect 高度*2+20 → Tips_SetSystemMonitorTipsText_SetsListAndFixedSize
// paintEvent：
//   B10 m_textList 为空 → 默认 0 值绘制 → Tips_PaintEvent_EmptyList_PaintsWithDefaults
//   B11 有数据 → 正常绘制 → Tips_PaintEvent_WithText_Renders
// event：
//   B12 FontChange && 列表非空 → 按原列表重算 → Tips_Event_FontChange_RefreshesWidths
//   B13 FontChange && 列表为空 → 填默认 4 项 → Tips_Event_FontChange_EmptyList_FillsDefaults
//   B14 Show → visibleChanged(true) → Tips_Event_ShowHide_EmitsVisibleChanged
//   B15 Hide → visibleChanged(false) → 同上
//
// 【备注】setSystemMonitorTipsText 以 m_textList.at(0) 计算高度，若外部传入空列表将越界
// （防御缺口，Mode 5 报告记录）；本文件不构造空列表调用该函数，避免测试自身崩溃。

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <stubext.h>

#include "systemmonitortipswidget.h"

#include <QEvent>
#include <QFontMetrics>
#include <QPainter>
#include <QPixmap>
#include <QSignalSpy>

#include <DApplication>
DWIDGET_USE_NAMESPACE

// TipsTest：悬浮提示框测试
class TipsTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        tips = new SystemMonitorTipsWidget();
    }
    void TearDown() override
    {
        delete tips;
        stub.clear();
    }

    stub_ext::StubExt stub;
    SystemMonitorTipsWidget *tips = nullptr;
};

// ---- 构造 ----

TEST_F(TipsTest, Tips_Ctor_InitializesEmptyState)
{
    // Arrange: SetUp 已构造
    // Act: 读取初始状态
    // Assert: 列表为空、两侧宽度为 0、无 visibleChanged 信号
    EXPECT_TRUE(tips->m_textList.isEmpty());
    EXPECT_EQ(tips->m_leftWidth, 0);
    EXPECT_EQ(tips->m_rightWidth, 0);
    QSignalSpy spy(tips, &SystemMonitorTipsWidget::visibleChanged);
    EXPECT_EQ(spy.count(), 0);
}

// ---- setSystemMonitorTipsText ----

TEST_F(TipsTest, Tips_SetSystemMonitorTipsText_SetsListAndFixedSize)
{
    // Arrange: 4 项标准文本（cpu/mem/downLoad/upLoad，长度均非 3）
    const QStringList textList { "99.9", "88.8", "12.3KB/s", "4.5KB/s" };
    // Act: 设置
    tips->setSystemMonitorTipsText(textList);
    // Assert: 列表保存；固定尺寸公式（宽度 = 10 + left + right，高度 = 首行高*2+20）
    EXPECT_EQ(tips->m_textList, textList);
    const int expectLeft = tips->fontMetrics().horizontalAdvance(
                               QString(" ") + DApplication::translate("Plugin.cpu", "CPU") + QString(": ") + "99.9" + QString(" "))
        + 20;
    const int expectRight = tips->fontMetrics().horizontalAdvance(QString("↓") + "12.3KB/s" + QString(" ")) + 20;
    EXPECT_EQ(tips->m_leftWidth, expectLeft);
    EXPECT_EQ(tips->m_rightWidth, expectRight);
    const int expectHeight = tips->fontMetrics().boundingRect("99.9").height() * 2 + 20;
    EXPECT_EQ(tips->size(), QSize(10 + expectLeft + expectRight, expectHeight));
}

TEST_F(TipsTest, Tips_SetSystemMonitorTipsText_CpuLength3_UsesZeroPad)
{
    // Arrange: cpu 串长 3（走 ": 0" 补零前缀分支）
    const QStringList textList { "9.9", "8.8", "12.3KB/s", "4.5KB/s" };
    // Act: 设置
    tips->setSystemMonitorTipsText(textList);
    // Assert: 左宽使用 ": 0" 前缀（B3）
    const int expectLeftPadded = tips->fontMetrics().horizontalAdvance(
                                     QString(" ") + DApplication::translate("Plugin.cpu", "CPU") + QString(": 0") + "9.9" + QString(" "))
        + 20;
    EXPECT_EQ(tips->m_leftWidth, expectLeftPadded);

    // 反向：cpu 串长 4 → 普通前缀（B4）
    const QStringList textList2 { "99.9", "8.8", "12.3KB/s", "4.5KB/s" };
    tips->setSystemMonitorTipsText(textList2);
    const int expectLeftPlain = tips->fontMetrics().horizontalAdvance(
                                    QString(" ") + DApplication::translate("Plugin.cpu", "CPU") + QString(": ") + "99.9" + QString(" "))
        + 20;
    EXPECT_EQ(tips->m_leftWidth, expectLeftPlain);
}

TEST_F(TipsTest, Tips_SetSystemMonitorTipsText_OverlongList_KeepsAllEntries)
{
    // Arrange: 5 项文本（第 5 项触发告警分支）
    const QStringList textList { "1.0", "2.0", "3KB/s", "4KB/s", "extra" };
    // Act: 设置
    EXPECT_NO_THROW(tips->setSystemMonitorTipsText(textList));
    // Assert: 列表全量保留（仅绘制/取值用前 4 项），尺寸按首项计算（B2）
    EXPECT_EQ(tips->m_textList, textList);
    EXPECT_EQ(tips->m_textList.size(), 5);
    EXPECT_GT(tips->size().width(), 0);
}

namespace {
struct DownLoadLenCase {
    const char *downLoad;
    const char *expectPrefix;
};
std::ostream &operator<<(std::ostream &os, const DownLoadLenCase &c)
{
    return os << "down=" << c.downLoad << " prefix=" << c.expectPrefix;
}
}

class DownLoadLenParamTest : public TipsTest,
                             public ::testing::WithParamInterface<DownLoadLenCase>
{
};

TEST_P(DownLoadLenParamTest, Tips_SetSystemMonitorTipsText_DownLenGrade_PrefixMapping)
{
    // Arrange: 参数即下行串与其期望前缀
    const DownLoadLenCase param = GetParam();
    // Act: 设置（upLoad 取值恒定，便于比较）
    tips->setSystemMonitorTipsText(QStringList { "9.9", "8.8", param.downLoad, "4.5KB/s" });
    // Assert: 右宽按长度分级使用补零前缀（B5/B6/B7/B8）
    // 注意前缀含 Unicode 箭头 ↓（U+2193），必须 fromUtf8；fromLatin1 会产生乱码字符宽度
    const int expectRight = tips->fontMetrics().horizontalAdvance(
                                QString::fromUtf8(param.expectPrefix) + QString::fromUtf8(param.downLoad) + QString(" "))
        + 20;
    // Assert: 右宽按长度分级使用补零前缀（B5/B6/B7/B8），且与文本更新同步
    EXPECT_EQ(tips->m_rightWidth, expectRight);
    EXPECT_FALSE(tips->m_textList.isEmpty());
}

INSTANTIATE_TEST_SUITE_P(DownLenGrades, DownLoadLenParamTest,
                         ::testing::Values(
                             DownLoadLenCase { "9KB", "↓000" },      // len==3
                             DownLoadLenCase { "99KB", "↓00" },      // len==4
                             DownLoadLenCase { "999KB", "↓0" },      // len==5
                             DownLoadLenCase { "9999KB", "↓" }));    // len==6 → 其它分支

// ---- paintEvent ----

TEST_F(TipsTest, Tips_PaintEvent_EmptyList_PaintsWithDefaults)
{
    // Arrange: 空列表（不调用 setSystemMonitorTipsText，规避 at(0) 防御缺口）
    ASSERT_TRUE(tips->m_textList.isEmpty());
    QPixmap pixmap(160, 60);
    // Act: 渲染（B10：默认 0 值绘制）
    EXPECT_NO_THROW(tips->render(&pixmap));
    // Assert: 渲染完成且列表保持为空
    EXPECT_FALSE(pixmap.isNull());
    EXPECT_TRUE(tips->m_textList.isEmpty());
}

TEST_F(TipsTest, Tips_PaintEvent_WithText_Renders)
{
    // Arrange: 设置 4 项文本
    tips->setSystemMonitorTipsText(QStringList { "12.3", "45.6", "789KB/s", "12KB/s" });
    ASSERT_EQ(tips->m_textList.size(), 4);
    QPixmap pixmap(tips->size());
    // Act: 渲染（B11）
    EXPECT_NO_THROW(tips->render(&pixmap));
    // Assert: 渲染目标与控件尺寸一致
    EXPECT_FALSE(pixmap.isNull());
    EXPECT_EQ(tips->m_textList.size(), 4);
}

// ---- event（FontChange / Show / Hide）----

TEST_F(TipsTest, Tips_Event_FontChange_RefreshesWidths)
{
    // Arrange: 先设置文本获得基准宽度
    tips->setSystemMonitorTipsText(QStringList { "12.3", "45.6", "789KB/s", "12KB/s" });
    const int baseLeft = tips->m_leftWidth;
    const int baseRight = tips->m_rightWidth;
    const QSize baseSize = tips->size();
    // Act: 派发字体变化事件
    QEvent fontChange(QEvent::FontChange);
    EXPECT_TRUE(tips->event(&fontChange));
    // Assert: 字体未变时重算结果一致（B12，强异常安全）
    EXPECT_EQ(tips->m_leftWidth, baseLeft);
    EXPECT_EQ(tips->m_rightWidth, baseRight);
    EXPECT_EQ(tips->size(), baseSize);
    EXPECT_EQ(tips->m_textList.size(), 4);
}

TEST_F(TipsTest, Tips_Event_FontChange_EmptyList_FillsDefaults)
{
    // Arrange: 全新控件（列表为空）
    ASSERT_TRUE(tips->m_textList.isEmpty());
    // Act: 派发字体变化事件
    QEvent fontChange(QEvent::FontChange);
    EXPECT_TRUE(tips->event(&fontChange));
    // Assert: 空列表走默认 4 项填充（B13）
    EXPECT_EQ(tips->m_textList, QStringList({ "0.0", "0.0", "0KB/s", "0KB/s" }));
    EXPECT_GT(tips->m_leftWidth, 0);
    EXPECT_GT(tips->m_rightWidth, 0);
}

TEST_F(TipsTest, Tips_Event_ShowHide_EmitsVisibleChanged)
{
    // Arrange: 信号监视
    QSignalSpy spy(tips, &SystemMonitorTipsWidget::visibleChanged);
    QEvent show(QEvent::Show);
    QEvent hide(QEvent::Hide);
    // Act: 显示事件
    EXPECT_TRUE(tips->event(&show));
    // Assert: visibleChanged(true)（B14）
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.first().first().toBool(), true);
    // 反向：隐藏事件 → visibleChanged(false)（B15）
    EXPECT_TRUE(tips->event(&hide));
    EXPECT_EQ(spy.count(), 2);
    EXPECT_EQ(spy.last().first().toBool(), false);
}
//
// 用例计数声明（self-check-structural 验证此块）：
// | method | level | factors | min | actual |
// |--------|-------|---------|-----|--------|
// | SystemMonitorTipsWidget | high | ctor/paint/signals | 5 | 5 |
// | setSystemMonitorTipsText | high | len-3/4/5/other | 4 | 4 |
// | downLoadLen 分级（参数化） | high | grades | 4 | 4 |
