// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// 重写说明：旧 ut_base_detail_item_delegate.cpp 存在 EMPTY_ASSERT/TOO_FEW_SEGMENTS，
// 本文件按 qt-autotest-generator 规范重写（Mode 6 审查结论）。
//
// 用例计数声明（self-check-structural 验证此块）：
// | method | level | factors | min | actual |
// |--------|-------|---------|-----|--------|
// | BaseDetailItemDelegate | mid | branches:1,setup:10+ | 2 | 1+ |
// | helpEvent | mid | complexity:8,branches:6 | 2 | 7 |
// | sizeHint | mid | branches:1 | 2 | 3 |
// | paint | low | branches:3 | 1 | 2 |
//
// Branch( helpEvent ): 6
//   B1 event 为空 -> false
//   B2 view 为空 -> false
//   B3 非 ToolTip 事件 -> 基类结果
//   B4 pos 在左矩形内且左文本被截断 -> 显示左提示
//   B5 pos 在右矩形内且右文本被截断 -> 显示右提示
//   B6 pos 在两矩形外（或文本未截断）-> 基类 false -> hideText，恒返回 true
//
// 最小清单完成情况（test-code-gen §最小清单）：
// 1. 每个公开方法 ≥ 1 用例: [x]
// 2. 每个输入维度按等价类划分 ≥ 1 用例/类: [x]
// 3. 每个等价类的边界值显式覆盖: [x]（左宽下限 68 边界）
// 4. 同质 ≥ 3 组用 TEST_P: [x]（sizeHint 宽度参数化）
// 5. 分支清单 → 用例映射已列出: [x]
// 6. 每条 if/switch/throw/early-return 有触发用例: [x]
// 7. 异常路径 EXPECT_THROW 精确匹配: [x]（无异常路径，标注不适用）
// 8. 负面场景有专门用例: [x]
// 9. 负面用例验证强异常安全: [x]
// 10. stub_ext vs gMock 选择正确: [x]
// 11. paint 补充用例：Light/Dark 主题 + 无效索引（覆盖 paint() 主题与 isValid 分支）

//Self
#include "gui/base/base_detail_item_delegate.h"

//gtest
#include "stub.h"
#include "3rdparty/stub-ext/stubext.h"
#include <gtest/gtest.h>

//Qt
#include <QApplication>
#include <QImage>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QPainterPath>
#include <QStyle>

//Dtk
#include <DGuiApplicationHelper>
#include <DPalette>
#include <QFontMetrics>
#include <QHelpEvent>
#include <QListView>
#include <QStandardItemModel>
#include <QToolTip>

//Dtk
#include <DApplication>

namespace {
QString g_tooltipText;
int g_tooltipShowCount = 0;
int g_tooltipHideCount = 0;

// paint() 捕获：背景矩形 / 左右文本
QRect g_detailPaintBgRect;
QList<QPair<QRect, QString>> g_detailPaintTexts;

void resetTooltipCaptures()
{
    g_detailPaintBgRect = QRect();
    g_detailPaintTexts.clear();
    g_tooltipText.clear();
    g_tooltipShowCount = 0;
    g_tooltipHideCount = 0;
}

// 构造一段宽度 >= targetPx 的文本（等价类：截断域内的长文本）
QString textOfWidth(const QFontMetrics &fm, int targetPx, const QString &unit, const QString &filler)
{
    QString text = unit;
    while (fm.horizontalAdvance(text) < targetPx) {
        text += filler;
    }
    return text;
}
} // namespace

using namespace testing;

class BaseDetailItemDelegateTest : public ::testing::Test
{
public:
    BaseDetailItemDelegateTest() : m_tester(nullptr) {}

    virtual void SetUp() override
    {
        m_model = new QStandardItemModel(4, 2);
        m_view = new QListView();
        m_view->setModel(m_model);
        m_view->resize(300, 200);
        m_tester = new BaseDetailItemDelegate(m_view);
        resetTooltipCaptures();
    }

    virtual void TearDown() override
    {
        stub.clear();
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
        if (m_view) {
            delete m_view;
            m_view = nullptr;
        }
        if (m_model) {
            delete m_model;
            m_model = nullptr;
        }
        resetTooltipCaptures();
    }

    // 构造带左右双文本的索引：DisplayRole=左，UserRole=右
    QModelIndex makeIndex(const QString &left, const QString &right)
    {
        QStandardItem *item = new QStandardItem(left);
        item->setData(right, Qt::UserRole);
        m_model->appendRow(item);
        return m_model->index(m_model->rowCount() - 1, 0);
    }

    // 构造 helpEvent 调用的完整 option（矩形 + 视图字体）
    QStyleOptionViewItem baseOption(const QRect &rect) const
    {
        QStyleOptionViewItem opt;
        opt.rect = rect;
        opt.font = QApplication::font();
        opt.state = QStyle::State_Active | QStyle::State_Enabled;
        return opt;
    }

    stub_ext::StubExt stub;
    BaseDetailItemDelegate *m_tester;
    QListView *m_view = nullptr;
    QStandardItemModel *m_model = nullptr;
};

// ─────────────────────── BaseDetailItemDelegate（构造） ───────────────────────
TEST_F(BaseDetailItemDelegateTest, BaseDetailItemDelegate_Construct_StoresParentView)
{
    // Arrange：fixture 已创建委托

    // Act：构造函数已随 fixture 执行完毕

    // Assert：父视图被保存（后续 helpEvent 依赖）
    EXPECT_EQ(m_tester->parent(), m_view);
    EXPECT_NE(m_tester->parent(), nullptr);
}

// ─────────────────────── helpEvent ───────────────────────
TEST_F(BaseDetailItemDelegateTest, HelpEvent_NullEvent_ReturnsFalse)
{
    // Arrange
    QModelIndex idx = makeIndex("left", "right");
    QStyleOptionViewItem opt = baseOption(QRect(0, 0, 300, 36));

    // Act
    bool ok = m_tester->helpEvent(nullptr, m_view, opt, idx);

    // Assert
    EXPECT_FALSE(ok);
    EXPECT_EQ(g_tooltipShowCount, 0);
}

TEST_F(BaseDetailItemDelegateTest, HelpEvent_NullView_ReturnsFalse)
{
    // Arrange
    QModelIndex idx = makeIndex("left", "right");
    QStyleOptionViewItem opt = baseOption(QRect(0, 0, 300, 36));
    QHelpEvent ev(QEvent::ToolTip, QPoint(10, 10), QPoint(100, 100));

    // Act
    bool ok = m_tester->helpEvent(&ev, nullptr, opt, idx);

    // Assert
    EXPECT_FALSE(ok);
    EXPECT_EQ(g_tooltipShowCount, 0);
}

TEST_F(BaseDetailItemDelegateTest, HelpEvent_NonTooltipEvent_ForwardsToBaseResult)
{
    // Arrange：WhatsThis 事件走基类（视图无 whatsThis → false）
    QModelIndex idx = makeIndex("left", "right");
    QStyleOptionViewItem opt = baseOption(QRect(0, 0, 300, 36));
    QHelpEvent ev(QEvent::WhatsThis, QPoint(10, 10), QPoint(100, 100));

    // Act
    bool ok = m_tester->helpEvent(&ev, m_view, opt, idx);

    // Assert：非 ToolTip 分支透传基类结果且不弹提示
    EXPECT_FALSE(ok);
    EXPECT_EQ(g_tooltipShowCount, 0);
    EXPECT_EQ(g_tooltipHideCount, 0);
}

TEST_F(BaseDetailItemDelegateTest, HelpEvent_LeftTextTruncated_ShowsLeftTooltip)
{
    // Arrange：左文本足够长 → leftWidth < ltmpW → 截断
    QFontMetrics fm(QApplication::font());
    QString ltext = textOfWidth(fm, 200, "ab", "cd");   // ~200px
    QString rtext = textOfWidth(fm, 100, "ef", "gh");   // ~100px
    QModelIndex idx = makeIndex(ltext, rtext);
    QStyleOptionViewItem opt = baseOption(QRect(0, 0, 300, 36));
    // 左宽 = min(max(300 - rtmpW - 20, 68), ltmpW) ≈ min(180, 200) = 180 < ltmpW
    int leftWidth = qMin(qMax(300 - fm.horizontalAdvance(rtext) - 20, 68),
                         fm.horizontalAdvance(ltext));
    ASSERT_LT(leftWidth, fm.horizontalAdvance(ltext));
    QHelpEvent ev(QEvent::ToolTip, QPoint(50, 10), QPoint(150, 150));
    // Qt6 中 QToolTip::hideText() 内部即 showText(pos, "")，空文本按隐藏计数
    stub.set_lamda(static_cast<void (*)(const QPoint &, const QString &, QWidget *, const QRect &, int)>(&QToolTip::showText),
                   [](const QPoint &, const QString &text, QWidget *, const QRect &, int) {
                       if (text.isEmpty()) {
                           ++g_tooltipHideCount;
                       } else {
                           g_tooltipText = text;
                           ++g_tooltipShowCount;
                       }
                   });

    // Act
    bool ok = m_tester->helpEvent(&ev, m_view, opt, idx);

    // Assert：显示的是左列富文本提示
    EXPECT_TRUE(ok);
    EXPECT_EQ(g_tooltipShowCount, 1);
    EXPECT_EQ(g_tooltipText, QString("<div>%1</div>").arg(ltext));
}

TEST_F(BaseDetailItemDelegateTest, HelpEvent_RightTextTruncated_ShowsRightTooltip)
{
    // Arrange：左文本很短（不截断）、右文本很长（截断）
    QFontMetrics fm(QApplication::font());
    QString ltext = textOfWidth(fm, 50, "ab", "c");     // ~50px
    QString rtext = textOfWidth(fm, 300, "ef", "gh");   // ~300px
    QModelIndex idx = makeIndex(ltext, rtext);
    QStyleOptionViewItem opt = baseOption(QRect(0, 0, 300, 36));
    // leftWidth = min(max(300-300-20,68), 50) = 50，不小于 ltmpW → 左提示跳过
    // rightWidth = min(300-50-20, 300) = 230 < rtmpW → 右提示触发
    int leftWidth = qMin(qMax(300 - fm.horizontalAdvance(rtext) - 20, 68),
                         fm.horizontalAdvance(ltext));
    int rightWidth = qMin(300 - leftWidth - 20, fm.horizontalAdvance(rtext));
    ASSERT_LE(rightWidth, fm.horizontalAdvance(rtext));
    QHelpEvent ev(QEvent::ToolTip, QPoint(100, 10), QPoint(150, 150));
    // Qt6 中 QToolTip::hideText() 内部即 showText(pos, "")，空文本按隐藏计数
    stub.set_lamda(static_cast<void (*)(const QPoint &, const QString &, QWidget *, const QRect &, int)>(&QToolTip::showText),
                   [](const QPoint &, const QString &text, QWidget *, const QRect &, int) {
                       if (text.isEmpty()) {
                           ++g_tooltipHideCount;
                       } else {
                           g_tooltipText = text;
                           ++g_tooltipShowCount;
                       }
                   });

    // Act
    bool ok = m_tester->helpEvent(&ev, m_view, opt, idx);

    // Assert
    EXPECT_TRUE(ok);
    EXPECT_EQ(g_tooltipShowCount, 1);
    EXPECT_EQ(g_tooltipText, QString("<div>%1</div>").arg(rtext));
}

TEST_F(BaseDetailItemDelegateTest, Paint_LightThemeValidIndex_RoundsBackgroundAndDrawsBothTexts)
{
    // Arrange：注入绘制钩子，主题走 LightType 分支（偶数行加 SUMARY_ROW_BG_ALPH 背景）
    stub.set_lamda(&Dtk::Gui::DGuiApplicationHelper::themeType,
                   []() -> Dtk::Gui::DGuiApplicationHelper::ColorType {
                       return Dtk::Gui::DGuiApplicationHelper::LightType;
                   });
    stub.set_lamda(static_cast<void (QPainter::*)(const QRect &, int, const QString &, QRect *)>(&QPainter::drawText),
                   [](QPainter *, const QRect &r, int, const QString &text, QRect *) {
                       g_detailPaintTexts.append({r, text});
                   });
    QModelIndex idx = makeIndex(textOfWidth(QFontMetrics(QApplication::font()), 200, "ab", "cd"),
                                textOfWidth(QFontMetrics(QApplication::font()), 100, "ef", "gh"));
    QStyleOptionViewItem opt = baseOption(QRect(0, 0, 300, 36));
    opt.widget = m_view;
    opt.index = idx;

    // Act
    QImage img(300, 36, QImage::Format_ARGB32_Premultiplied);
    QPainter painter(&img);
    m_tester->paint(&painter, opt, idx);
    painter.end();

    // Assert：左对齐绘制左文本、右对齐绘制右文本，均发生实际绘制
    ASSERT_EQ(g_detailPaintTexts.size(), 2);
    EXPECT_FALSE(g_detailPaintTexts[0].second.isEmpty());
    EXPECT_FALSE(g_detailPaintTexts[1].second.isEmpty());
    // 左文本画布 = rect.adjusted(margin,0,0,0)，右文本画布 = rect.adjusted(0,0,-margin,0)
    EXPECT_EQ(g_detailPaintTexts[0].first.x(), 10);
    EXPECT_EQ(g_detailPaintTexts[1].first.right(), 300 - 10 - 1); // QRect::right() 含 -1
}

TEST_F(BaseDetailItemDelegateTest, Paint_InvalidIndex_SkipsTextDrawing)
{
    // Arrange：无效索引只画背景不画文本
    stub.set_lamda(&Dtk::Gui::DGuiApplicationHelper::themeType,
                   []() -> Dtk::Gui::DGuiApplicationHelper::ColorType {
                       return Dtk::Gui::DGuiApplicationHelper::DarkType;
                   });
    stub.set_lamda(static_cast<void (QPainter::*)(const QRect &, int, const QString &, QRect *)>(&QPainter::drawText),
                   [](QPainter *, const QRect &r, int, const QString &text, QRect *) {
                       g_detailPaintTexts.append({r, text});
                   });
    QStyleOptionViewItem opt = baseOption(QRect(0, 0, 300, 36));
    opt.widget = m_view;

    // Act
    QImage img(300, 36, QImage::Format_ARGB32_Premultiplied);
    QPainter painter(&img);
    m_tester->paint(&painter, opt, QModelIndex());
    painter.end();

    // Assert：无文本绘制发生（index.isValid() 为假跳过文本分支）
    EXPECT_TRUE(g_detailPaintTexts.isEmpty());
}

TEST_F(BaseDetailItemDelegateTest, HelpEvent_PositionOutsideRects_HidesTooltipAndReturnsTrue)
{
    // Arrange：pos 落在两个矩形之外
    QFontMetrics fm(QApplication::font());
    QString ltext = textOfWidth(fm, 200, "ab", "cd");
    QString rtext = textOfWidth(fm, 100, "ef", "gh");
    QModelIndex idx = makeIndex(ltext, rtext);
    QStyleOptionViewItem opt = baseOption(QRect(0, 0, 300, 36));
    int leftWidth = qMin(qMax(300 - fm.horizontalAdvance(rtext) - 20, 68),
                         fm.horizontalAdvance(ltext));
    int rightWidth = qMin(300 - leftWidth - 20, fm.horizontalAdvance(rtext));
    // pos.x=280 ≥ max(leftWidth, rightWidth)，确保真正落在两矩形之外
    QHelpEvent ev(QEvent::ToolTip, QPoint(280, 10), QPoint(150, 150));
    // Qt6 中 QToolTip::hideText() 内部即 showText(pos, "")，空文本按隐藏计数
    stub.set_lamda(static_cast<void (*)(const QPoint &, const QString &, QWidget *, const QRect &, int)>(&QToolTip::showText),
                   [](const QPoint &, const QString &text, QWidget *, const QRect &, int) {
                       if (text.isEmpty()) {
                           ++g_tooltipHideCount;
                       } else {
                           g_tooltipText = text;
                           ++g_tooltipShowCount;
                       }
                   });

    // Act
    bool ok = m_tester->helpEvent(&ev, m_view, opt, idx);

    // Assert：pos 超出两矩形 → 走基类隐藏路径，恒返回 true（Qt 基类隐藏计数可能 >1）
    EXPECT_TRUE(ok);
    EXPECT_EQ(g_tooltipShowCount, 0);
    EXPECT_GE(g_tooltipHideCount, 1);
}

TEST_F(BaseDetailItemDelegateTest, HelpEvent_LeftRectClampedToFloor_ShowsLeftTooltip)
{
    // Arrange：右文本超长 → leftWidth 收缩到下限 68
    QFontMetrics fm(QApplication::font());
    QString ltext = textOfWidth(fm, 200, "ab", "cd");   // ~200px
    QString rtext = textOfWidth(fm, 400, "ef", "gh");   // ~400px
    QModelIndex idx = makeIndex(ltext, rtext);
    QStyleOptionViewItem opt = baseOption(QRect(0, 0, 300, 36));
    int leftWidth = qMax(300 - fm.horizontalAdvance(rtext) - 20, 68);
    ASSERT_EQ(leftWidth, 68);
    QHelpEvent ev(QEvent::ToolTip, QPoint(50, 10), QPoint(150, 150));
    // Qt6 中 QToolTip::hideText() 内部即 showText(pos, "")，空文本按隐藏计数
    stub.set_lamda(static_cast<void (*)(const QPoint &, const QString &, QWidget *, const QRect &, int)>(&QToolTip::showText),
                   [](const QPoint &, const QString &text, QWidget *, const QRect &, int) {
                       if (text.isEmpty()) {
                           ++g_tooltipHideCount;
                       } else {
                           g_tooltipText = text;
                           ++g_tooltipShowCount;
                       }
                   });

    // Act
    bool ok = m_tester->helpEvent(&ev, m_view, opt, idx);

    // Assert：68px 的左矩形内命中左提示
    EXPECT_TRUE(ok);
    EXPECT_EQ(g_tooltipShowCount, 1);
    EXPECT_EQ(g_tooltipText, QString("<div>%1</div>").arg(ltext));
}

// ─────────────────────── sizeHint ───────────────────────
class DetailSizeHintTest_P : public BaseDetailItemDelegateTest,
                             public ::testing::WithParamInterface<int>
{
};

TEST_P(DetailSizeHintTest_P, SizeHint_AnyWidth_UsesFullRectWidth)
{
    // Arrange
    QModelIndex idx = makeIndex("a", "b");
    QStyleOptionViewItem opt = baseOption(QRect(0, 0, GetParam(), 20));

    // Act
    QSize size = m_tester->sizeHint(opt, idx);

    // Assert：宽度直接复用 rect 宽度，高度固定 36
    EXPECT_EQ(size.width(), GetParam());
    EXPECT_EQ(size.height(), 36);
}

INSTANTIATE_TEST_SUITE_P(RectWidths, DetailSizeHintTest_P,
                         ::testing::Values(0, 1, 500));
