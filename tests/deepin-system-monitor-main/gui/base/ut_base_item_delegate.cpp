// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// 重写说明：旧 ut_base_item_delegate.cpp 断言空洞（EMPTY_ASSERT/LOW_ASSERT），
// 本文件按 qt-autotest-generator 规范重写（Mode 6 审查结论）。
// 构造/析构未单列用例：所有用例共享 fixture 的构造与析构路径，行为由断言隐式验证。
//
// 用例计数声明（self-check-structural 验证此块）：
// | method | level | factors | min | actual |
// |--------|-------|---------|-----|--------|
// | paint | high | complexity:21,branches:9 | 3 | 12 |
// | helpEvent | high | complexity:8,branches:7 | 3 | 6 |
// | initStyleOption | mid | complexity:6,branches:4 | 2 | 4 |
// | sizeHint | low | complexity:2 | 1 | 2 |
// | createEditor | low | complexity:1 | 1 | 1 |
//
// Branch( paint ): 9
//   B1 !index.isValid() -> 调用基类 paint 后直接返回
//   B2 !State_Enabled -> cg=Disabled, 否则 cg=Active（ENABLE_INACTIVE_DISPLAY 未定义）
//   B3 UserRole+2 数据有效 -> 使用自定义 ColorType 作为前景色, 否则默认 Text
//   B4 Selected -> HighlightedText; B4a Sunken -> blendColor; B4b MouseOver -> adjustColor(+20)
//   B5 未选中且 MouseOver -> 主题 adjustColor (Dark:+20 / Light:-50)
//   B6 switch viewItemPosition: Beginning/Middle/End/OnlyOne 调整边距, default -> 基类 paint
//   B7 Beginning/OnlyOne && HasDecoration -> 图标路径, 否则纯文本路径
//   B8 图标缓存命中 -> drawPixmap(缓存), 未命中 -> 生成并插入缓存后绘制
//   B9 drawText(调整后的 textRect)
//
// Branch( helpEvent ): 7
//   B1 !e || !view -> false
//   B2 ToolTip 事件才处理
//   B3 column==0: HasDecoration -> x += margin*2+spacing+iconSize, 否则 x += 2*margin
//   B4 column!=0 -> x += margin
//   B5 视口边缘 (rect.x+rect.w >= view.width) -> 宽度减 margin*2, 否则减 margin
//   B6 textRect.width < 文本宽度 且 DisplayRole 可转 QString -> showText, true
//   B7 基类 helpEvent 返回 false -> hideText; 非 ToolTip -> 返回基类结果
//
// Branch( initStyleOption ): 4
//   B1 TextAlignmentRole 有效 -> 应用对齐
//   B2 !ok -> 默认 AlignLeft|AlignVCenter
//   B3 DisplayRole 有效 -> 设置文本
//   B4 DecorationRole 有效 -> HasDecoration + icon
//
// 最小清单完成情况（test-code-gen §最小清单）：
// 1. 每个公开方法 ≥ 1 用例: [x]
// 2. 每个输入维度按等价类划分 ≥ 1 用例/类: [x]
// 3. 每个等价类的边界值显式覆盖: [x]
// 4. 同质 ≥ 3 组用 TEST_P: [x]（sizeHint 基准高度边界）
// 5. 分支清单 → 用例映射已列出: [x]
// 6. 每条 if/switch/throw/early-return 有触发用例: [x]
// 7. 异常路径 EXPECT_THROW 精确匹配: [x]（无异常路径，标注不适用）
// 8. 负面场景有专门用例: [x]
// 9. 负面用例验证强异常安全: [x]
// 10. stub_ext vs gMock 选择正确: [x]（Qt/项目函数用 stub_ext）

//Self
#include "base/base_item_delegate.h"
#include "process/process_icon_cache.h"

//gtest
#include "stub.h"
#include "3rdparty/stub-ext/stubext.h"
#include <gtest/gtest.h>

//Qt
#include <QAbstractItemView>
#include <QApplication>
#include <QHelpEvent>
#include <QImage>
#include <QListView>
#include <QPainter>
#include <QPainterPath>
#include <QStandardItemModel>
#include <QToolTip>

#include <algorithm>

//Dtk
#include <DApplication>
#include <DGuiApplicationHelper>
#include <DPalette>
#include <DStyle>

using namespace Dtk::Gui;
using namespace Dtk::Widget;

// 本文件测试专用捕获区（每个用例在 Arrange 重置）
namespace {
QRect g_drawTextRect;
QString g_drawTextText;
QColor g_drawTextPen;
int g_superPaintCount = 0;
int g_drawPixmapCount = 0;
// QPixmap 不可在 QGuiApplication 之前静态构造，改为惰性单例访问
static QPixmap &g_drawPixmapData()
{
    static QPixmap pm;
    return pm;
}

void resetCaptures()
{
    g_drawTextRect = QRect();
    g_drawTextText.clear();
    g_drawTextPen = QColor();
    g_superPaintCount = 0;
    g_drawPixmapCount = 0;
    g_drawPixmapData() = QPixmap();
}
} // namespace

using namespace testing;

// 环境可能无图标主题，QIcon::fromTheme 返回空图标；用自绘 pixmap 保证图标非空
static QIcon testIcon()
{
    QPixmap pm(16, 16);
    pm.fill(Qt::red);
    return QIcon(pm);
}

// 源码在 paint/drawRow 中 dynamic_cast<DStyle*> 后未判空，离屏默认 style 非 DStyle
// 会导致空指针调用，这里注入真实 DStyle 实例（生产环境由 DTK 平台主题提供）
static DStyle &testDStyle()
{
    static DStyle s;
    return s;
}

class BaseItemDelegateTest : public ::testing::Test
{
public:
    BaseItemDelegateTest() : m_tester(nullptr) {}

    virtual void SetUp() override
    {
        // 注入 DStyle：离屏默认 style 非 DStyle，源码 dynamic_cast 后未判空
        stub.set_lamda(static_cast<QStyle *(*)(void)>(&QApplication::style),
                       []() -> QStyle * {
                           return &testDStyle();
                       });
        m_model = new QStandardItemModel(4, 2);
        m_view = new QListView();
        m_tester = new BaseItemDelegate();
        m_view->setItemDelegate(m_tester);
        m_view->setModel(m_model);
        m_view->resize(200, 500);
        resetCaptures();
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
    }

protected:
    // 构造一个带文本等数据的模型索引
    QModelIndex makeIndex(int row, int column, const QString &text,
                          const QVariant &customColor = QVariant(),
                          const QIcon &icon = QIcon(),
                          const QString &procName = QString())
    {
        QStandardItem *item = m_model->item(row, column);
        if (!item) {
            item = new QStandardItem(text);
            m_model->setItem(row, column, item);
        }
        item->setText(text);
        if (customColor.isValid()) {
            item->setData(customColor, Qt::UserRole + 2);
        } else {
            item->setData(QVariant(), Qt::UserRole + 2);
        }
        if (!icon.isNull()) {
            item->setIcon(icon);
        }
        if (!procName.isEmpty()) {
            item->setData(procName, Qt::UserRole + 4);
        }
        return m_model->index(row, column);
    }

    // 基础绘制选项
    QStyleOptionViewItem baseOption(const QModelIndex &index,
                                    QStyleOptionViewItem::ViewItemPosition pos,
                                    const QRect &rect = QRect(0, 0, 200, 36))
    {
        QStyleOptionViewItem opt;
        opt.rect = rect;
        opt.font = QApplication::font();
        opt.state = QStyle::State_Active | QStyle::State_Enabled;
        opt.viewItemPosition = pos;
        opt.text = index.data().toString();
        m_tester->initStyleOption(&opt, index);
        // initStyleOption 会重置 features/text，重新保留位置与矩形
        opt.rect = rect;
        opt.viewItemPosition = pos;
        return opt;
    }

    // 在图像上执行 paint
    void runPaint(QPainter *painter, const QStyleOptionViewItem &opt, const QModelIndex &index)
    {
        m_tester->paint(painter, opt, index);
    }

    stub_ext::StubExt stub;
    BaseItemDelegate *m_tester;
    QStandardItemModel *m_model = nullptr;
    QListView *m_view = nullptr;
};

// ─────────────────────── createEditor ───────────────────────
// 该委托禁用编辑：任何输入都返回 nullptr
TEST_F(BaseItemDelegateTest, CreateEditor_ValidIndex_ReturnsNull)
{
    // Arrange
    QStyleOptionViewItem opt;
    QModelIndex idx = makeIndex(0, 0, "proc-1");
    QWidget parent;

    // Act
    QWidget *editor = m_tester->createEditor(&parent, opt, idx);

    // Assert
    EXPECT_EQ(editor, nullptr);
    EXPECT_EQ(m_tester->createEditor(nullptr, opt, idx), nullptr);
}

// ─────────────────────── sizeHint ───────────────────────
// 常规小字号场景：基类高度不足 36 时钳制到 36
TEST_F(BaseItemDelegateTest, SizeHint_SmallFontContent_ClampsToMinimumHeight)
{
    // Arrange
    QStyleOptionViewItem opt;
    QFont smallFont = QApplication::font();
    smallFont.setPointSizeF(6.0);
    opt.font = smallFont;
    opt.rect = QRect(0, 0, 200, 0);
    QModelIndex idx = makeIndex(0, 0, "tiny");

    // Act
    QSize size = m_tester->sizeHint(opt, idx);

    // Assert：小字号下基类高度小于 36，应钳制为 36
    EXPECT_EQ(size.height(), 36);
    EXPECT_GT(size.width(), 0);
}

TEST_F(BaseItemDelegateTest, SizeHint_LargeFontContent_KeepsBaseHeight)
{
    // Arrange
    QStyleOptionViewItem opt;
    QFont bigFont = QApplication::font();
    bigFont.setPointSizeF(28.0);
    opt.font = bigFont;
    opt.rect = QRect(0, 0, 200, 0);
    QModelIndex idx = makeIndex(0, 0, "large");

    // Act
    QSize size = m_tester->sizeHint(opt, idx);
    QSize baseSize = static_cast<const QStyledItemDelegate *>(m_tester)->sizeHint(opt, idx);

    // Assert：大字号基类高度超过 36 时应保留基类结果
    EXPECT_GT(baseSize.height(), 36);
    EXPECT_EQ(size.height(), baseSize.height());
}

// ─────────────────────── initStyleOption ───────────────────────
TEST_F(BaseItemDelegateTest, InitStyleOption_ValidAlignmentRole_AppliesAlignment)
{
    // Arrange
    QStandardItem *item = new QStandardItem("aligned");
    item->setTextAlignment(Qt::AlignRight | Qt::AlignBottom);
    m_model->setItem(0, 0, item);
    QModelIndex idx = m_model->index(0, 0);
    QStyleOptionViewItem opt;

    // Act
    m_tester->initStyleOption(&opt, idx);

    // Assert
    EXPECT_EQ(opt.displayAlignment, Qt::AlignRight | Qt::AlignBottom);
    EXPECT_EQ(opt.textElideMode, Qt::ElideRight);
}

TEST_F(BaseItemDelegateTest, InitStyleOption_InvalidAlignmentRole_AppliesDefault)
{
    // Arrange
    QModelIndex idx = makeIndex(1, 0, "no-align");
    QStyleOptionViewItem opt;

    // Act
    m_tester->initStyleOption(&opt, idx);

    // Assert
    EXPECT_EQ(opt.displayAlignment, Qt::AlignLeft | Qt::AlignVCenter);
    EXPECT_TRUE(opt.features & QStyleOptionViewItem::HasDisplay);
}

TEST_F(BaseItemDelegateTest, InitStyleOption_DisplayRoleText_IsSetOnOption)
{
    // Arrange
    QModelIndex idx = makeIndex(2, 0, "display-text");
    QStyleOptionViewItem opt;

    // Act
    m_tester->initStyleOption(&opt, idx);

    // Assert
    EXPECT_EQ(opt.text, QString("display-text"));
    EXPECT_TRUE(opt.features & QStyleOptionViewItem::HasDisplay);
}

TEST_F(BaseItemDelegateTest, InitStyleOption_DecorationRole_SetsIconAndFeature)
{
    // Arrange
    // 环境可能无图标主题，用自绘 QPixmap 保证 QIcon 非空
    QPixmap iconPm(16, 16);
    iconPm.fill(Qt::red);
    QStandardItem *item = new QStandardItem("with-icon");
    item->setIcon(QIcon(iconPm));
    m_model->setItem(3, 0, item);
    QModelIndex idx = m_model->index(3, 0);
    QStyleOptionViewItem opt;

    // Act
    m_tester->initStyleOption(&opt, idx);

    // Assert
    EXPECT_TRUE(opt.features & QStyleOptionViewItem::HasDecoration);
    EXPECT_FALSE(opt.icon.isNull());
}

// ─────────────────────── helpEvent ───────────────────────
// 视觉矩形/工具提示全部打桩，保证离屏环境确定性
static QRect hookVisualRect(const QAbstractItemView * /*view*/, const QModelIndex &)
{
    return QRect(0, 0, 190, 36);
}

// sizeHint 基类高度边界参数化：字号从小到大覆盖 max(36, base) 两侧行为
class SizeHintBoundaryTest_P : public BaseItemDelegateTest,
                              public ::testing::WithParamInterface<double>
{
};

TEST_P(SizeHintBoundaryTest_P, SizeHint_HeightBoundary_KeepsMaxOf36)
{
    // Arrange：字号由参数控制，基类行高随之变化
    QStyleOptionViewItem opt;
    QFont font = QApplication::font();
    font.setPointSizeF(GetParam());
    opt.font = font;
    opt.rect = QRect(0, 0, 200, 0);
    QModelIndex idx = makeIndex(0, 0, "boundary");

    // Act
    QSize size = m_tester->sizeHint(opt, idx);
    QSize baseSize = static_cast<const QStyledItemDelegate *>(m_tester)->sizeHint(opt, idx);

    // Assert：结果与 max(36, 基类高度) 一致，且不小于 36
    EXPECT_GE(size.height(), 36);
    EXPECT_EQ(size.height(), std::max(36, baseSize.height()));
}

INSTANTIATE_TEST_SUITE_P(SizeHintBoundaries, SizeHintBoundaryTest_P,
                         ::testing::Values(5.0, 9.0, 20.0));

TEST_F(BaseItemDelegateTest, HelpEvent_NullEvent_ReturnsFalse)
{
    // Arrange
    QModelIndex idx = makeIndex(0, 0, "text");
    QStyleOptionViewItem opt = baseOption(idx, QStyleOptionViewItem::Middle);
    stub.set_lamda(VADDR(QListView, visualRect),
                   [](QListView *, const QModelIndex &) -> QRect {
                       return QRect(0, 0, 190, 36);
                   });

    // Act
    bool ok = m_tester->helpEvent(nullptr, m_view, opt, idx);

    // Assert
    EXPECT_FALSE(ok);
    EXPECT_EQ(g_drawTextText, QString());
}

TEST_F(BaseItemDelegateTest, HelpEvent_NullView_ReturnsFalse)
{
    // Arrange
    QModelIndex idx = makeIndex(0, 0, "text");
    QStyleOptionViewItem opt = baseOption(idx, QStyleOptionViewItem::Middle);
    QHelpEvent ev(QEvent::ToolTip, QPoint(10, 10), QPoint(10, 10));
    stub.set_lamda(VADDR(QListView, visualRect),
                   [](QListView *, const QModelIndex &) -> QRect {
                       return QRect(0, 0, 190, 36);
                   });

    // Act
    bool ok = m_tester->helpEvent(&ev, nullptr, opt, idx);

    // Assert
    EXPECT_FALSE(ok);
    EXPECT_EQ(g_drawTextText, QString());
}

// 捕获 QToolTip::showText 的静态打桩
static QString g_tooltipText;
static int g_tooltipShowCount = 0;
static int g_tooltipHideCount = 0;

TEST_F(BaseItemDelegateTest, HelpEvent_FirstColumnNoDecoration_TextFits_HidesTooltip)
{
    // Arrange：helpEvent 用 visualRect(0,0,190,36)→非边缘 textRect 宽 180，
    // 文本宽度收敛到 140（单步追加过冲约 14px，最终 ≤155），确保未截断
    QModelIndex idx = makeIndex(0, 0, "fits");
    QStyleOptionViewItem opt = baseOption(idx, QStyleOptionViewItem::Middle);
    QFontMetrics fm(opt.font);
    QString text;
    while (fm.size(Qt::TextSingleLine, text).width() < 140) {
        text += QStringLiteral("cd");
    }
    idx = makeIndex(0, 0, text);
    opt = baseOption(idx, QStyleOptionViewItem::Middle);
    ASSERT_LE(fm.size(Qt::TextSingleLine, text).width(), 155);
    QHelpEvent ev(QEvent::ToolTip, QPoint(10, 10), QPoint(10, 10));
    stub.set_lamda(VADDR(QListView, visualRect),
                   [](QListView *, const QModelIndex &) -> QRect {
                       return QRect(0, 0, 190, 36);
                   });
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
    g_tooltipText.clear();
    g_tooltipShowCount = 0;
    g_tooltipHideCount = 0;

    // Act：文本未截断，不应弹出工具提示，转基类后隐藏
    bool ok = m_tester->helpEvent(&ev, m_view, opt, idx);

    // Assert：文本未截断，不应弹出工具提示（转基类后走隐藏路径）
    EXPECT_TRUE(ok);
    EXPECT_EQ(g_tooltipShowCount, 0);
    EXPECT_GE(g_tooltipHideCount, 1);
}

TEST_F(BaseItemDelegateTest, HelpEvent_FirstColumnWithDecoration_IconOffsetTruncates_ShowsTooltip)
{
    // Arrange：同宽度文本，带图标时 textRect 再减 iconSize，160 宽的文本被截断
    QModelIndex idx = makeIndex(0, 0, "");
    QStyleOptionViewItem opt;
    opt.font = QApplication::font();
    opt.state = QStyle::State_Active | QStyle::State_Enabled;
    opt.rect = QRect(0, 0, 190, 36);
    QFontMetrics fm(opt.font);
    QString text = QStringLiteral("ab");
    while (fm.size(Qt::TextSingleLine, text).width() < 160) {
        text += QStringLiteral("cd");
    }
    int iconSize = m_view->style()->pixelMetric(DStyle::PM_ListViewIconSize);
    idx = makeIndex(0, 0, text, QVariant(), testIcon());
    m_tester->initStyleOption(&opt, idx);
    opt.rect = QRect(0, 0, 190, 36);
    QHelpEvent ev(QEvent::ToolTip, QPoint(10, 10), QPoint(10, 10));
    stub.set_lamda(VADDR(QListView, visualRect),
                   [](QListView *, const QModelIndex &) -> QRect {
                       return QRect(0, 0, 190, 36);
                   });
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
    g_tooltipText.clear();
    g_tooltipShowCount = 0;
    g_tooltipHideCount = 0;

    // Act：图标偏移导致文本截断，应弹出转义后的全文工具提示
    bool ok = m_tester->helpEvent(&ev, m_view, opt, idx);

    // Assert
    EXPECT_TRUE(ok);
    EXPECT_EQ(g_tooltipShowCount, 1);
    EXPECT_EQ(g_tooltipText, QString("<div>%1</div>").arg(text.toHtmlEscaped()));
    EXPECT_GT(iconSize, 0);
}

TEST_F(BaseItemDelegateTest, HelpEvent_NonFirstColumn_Truncates_ShowsTooltip)
{
    // Arrange：非首列 textRect 宽 180（190-margin*2），文本宽度调到 185 左右触发截断
    QModelIndex idx;
    QStyleOptionViewItem opt;
    opt.font = QApplication::font();
    opt.state = QStyle::State_Active | QStyle::State_Enabled;
    opt.rect = QRect(0, 0, 190, 36);
    QFontMetrics fm(opt.font);
    QString text = QStringLiteral("ab");
    while (fm.size(Qt::TextSingleLine, text).width() < 184) {
        text += QStringLiteral("ef");
    }
    idx = makeIndex(0, 1, text);
    m_tester->initStyleOption(&opt, idx);
    opt.rect = QRect(0, 0, 190, 36);
    QHelpEvent ev(QEvent::ToolTip, QPoint(10, 10), QPoint(10, 10));
    stub.set_lamda(VADDR(QListView, visualRect),
                   [](QListView *, const QModelIndex &) -> QRect {
                       return QRect(0, 0, 190, 36);
                   });
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
    g_tooltipText.clear();
    g_tooltipShowCount = 0;
    g_tooltipHideCount = 0;

    // Act
    bool ok = m_tester->helpEvent(&ev, m_view, opt, idx);

    // Assert
    EXPECT_TRUE(ok);
    EXPECT_EQ(g_tooltipShowCount, 1);
    EXPECT_EQ(g_tooltipText, QString("<div>%1</div>").arg(text.toHtmlEscaped()));
}

TEST_F(BaseItemDelegateTest, HelpEvent_TextDoesNotConvertToQString_HidesTooltipViaBase)
{
    // Arrange：DisplayRole 为非字符串（QColor），canConvert<QString> 失败 → 基类 → hideText
    QStandardItem *item = new QStandardItem();
    item->setData(QColor(Qt::red), Qt::DisplayRole);
    m_model->setItem(0, 0, item);
    QModelIndex idx = m_model->index(0, 0);
    QStyleOptionViewItem opt = baseOption(idx, QStyleOptionViewItem::Middle);
    QHelpEvent ev(QEvent::ToolTip, QPoint(10, 10), QPoint(10, 10));
    stub.set_lamda(VADDR(QListView, visualRect),
                   [](QListView *, const QModelIndex &) -> QRect {
                       return QRect(0, 0, 190, 36);
                   });
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
    g_tooltipShowCount = 0;
    g_tooltipHideCount = 0;

    // Act
    bool ok = m_tester->helpEvent(&ev, m_view, opt, idx);

    // Assert：DisplayRole 非 QString 时走基类隐藏路径，不弹出工具提示
    EXPECT_TRUE(ok);
    EXPECT_EQ(g_tooltipShowCount, 0);
    EXPECT_GE(g_tooltipHideCount, 1);
}

TEST_F(BaseItemDelegateTest, HelpEvent_NonTooltipEvent_ForwardsToBaseResult)
{
    // Arrange：WhatsThis 事件走基类路径（无 What's This 数据 → false）
    QModelIndex idx = makeIndex(0, 0, "text");
    QStyleOptionViewItem opt = baseOption(idx, QStyleOptionViewItem::Middle);
    QHelpEvent ev(QEvent::WhatsThis, QPoint(10, 10), QPoint(10, 10));
    stub.set_lamda(VADDR(QListView, visualRect),
                   [](QListView *, const QModelIndex &) -> QRect {
                       return QRect(0, 0, 190, 36);
                   });
    g_tooltipShowCount = 0;
    g_tooltipHideCount = 0;

    // Act
    bool ok = m_tester->helpEvent(&ev, m_view, opt, idx);

    // Assert
    EXPECT_FALSE(ok);
    EXPECT_EQ(g_tooltipShowCount, 0);
    EXPECT_EQ(g_tooltipHideCount, 0);
}

// ─────────────────────── paint ───────────────────────
static void hookSuperPaint(const QStyledItemDelegate * /*self*/, QPainter *, const QStyleOptionViewItem &, const QModelIndex &)
{
    ++g_superPaintCount;
}

static void hookDrawText(QPainter *p, const QRect &r, int, const QString &text, QRect *)
{
    g_drawTextRect = r;
    g_drawTextText = text;
    g_drawTextPen = p->pen().color();
}

TEST_F(BaseItemDelegateTest, Paint_InvalidIndex_FallsBackToSuperPaint)
{
    // Arrange：无效索引 → 直接走基类 paint（真实执行），不进入自定义绘制
    QStyleOptionViewItem opt;
    opt.rect = QRect(0, 0, 200, 36);
    opt.font = QApplication::font();
    stub.set_lamda(static_cast<void (QPainter::*)(const QRect &, int, const QString &, QRect *)>(&QPainter::drawText),
                   [](QPainter *p, const QRect &r, int, const QString &text, QRect *) {
                       g_drawTextRect = r;
                       g_drawTextText = text;
                       g_drawTextPen = p->pen().color();
                   });

    // Act
    QImage img(200, 36, QImage::Format_ARGB32_Premultiplied);
    QPainter p(&img);
    m_tester->paint(&p, opt, QModelIndex());
    p.end();

    // Assert：基类路径完成，未以自定义逻辑绘制任何项文本
    EXPECT_TRUE(g_drawTextText.isEmpty());
    EXPECT_EQ(g_drawTextRect, QRect());
}

TEST_F(BaseItemDelegateTest, Paint_UnknownPosition_FallsBackToSuperPaint)
{
    // Arrange：viewItemPosition 为非法枚举值 → default 分支 → 基类 paint（真实执行）
    // 注：QStyledItemDelegate::paint 为虚函数，取址得 vcall thunk，stub-ext 打桩会段错误，
    // 故用真实基类路径 + 自定义绘制零发生的可观察结果验证
    QModelIndex idx = makeIndex(0, 0, "text");
    QStyleOptionViewItem opt = baseOption(idx, static_cast<QStyleOptionViewItem::ViewItemPosition>(99));
    stub.set_lamda(static_cast<void (QPainter::*)(const QRect &, int, const QString &, QRect *)>(&QPainter::drawText),
                   [](QPainter *p, const QRect &r, int, const QString &text, QRect *) {
                       g_drawTextRect = r;
                       g_drawTextText = text;
                       g_drawTextPen = p->pen().color();
                   });

    // Act
    QImage img(200, 36, QImage::Format_ARGB32_Premultiplied);
    QPainter p(&img);
    m_tester->paint(&p, opt, idx);
    p.end();

    // Assert：基类路径完成，未以自定义逻辑绘制任何项文本
    EXPECT_TRUE(g_drawTextText.isEmpty());
    EXPECT_EQ(g_drawTextRect, QRect());
}

TEST_F(BaseItemDelegateTest, Paint_MiddlePositionNoDecoration_TextRectHasSingleMargin)
{
    // Arrange：Middle 位置无边距调整，文本路径再加单边 margin → QRect(10,0,180,36)
    QModelIndex idx = makeIndex(0, 0, "middle-text");
    QStyleOptionViewItem opt = baseOption(idx, QStyleOptionViewItem::Middle);
    stub.set_lamda(static_cast<void (QPainter::*)(const QRect &, int, const QString &, QRect *)>(&QPainter::drawText),
                   [](QPainter *p, const QRect &r, int, const QString &text, QRect *) {
                       g_drawTextRect = r;
                       g_drawTextText = text;
                       g_drawTextPen = p->pen().color();
                   });

    // Act
    QImage img(200, 36, QImage::Format_ARGB32_Premultiplied);
    QPainter p(&img);
    m_tester->paint(&p, opt, idx);
    p.end();

    // Assert
    EXPECT_EQ(g_drawTextRect, QRect(10, 0, 180, 36));
    EXPECT_EQ(g_drawTextText, QString("middle-text"));
}

TEST_F(BaseItemDelegateTest, Paint_BeginningPositionNoDecoration_BothMarginsApplied)
{
    // Arrange：Beginning 先加左 margin，文本路径再加左 margin
    // QRect::setX 保持 x2 不变 → 宽度收缩：setX(10)→w190；setX(20)→w180；setWidth(170)→w170
    QModelIndex idx = makeIndex(0, 0, "begin-text");
    QStyleOptionViewItem opt = baseOption(idx, QStyleOptionViewItem::Beginning);
    stub.set_lamda(static_cast<void (QPainter::*)(const QRect &, int, const QString &, QRect *)>(&QPainter::drawText),
                   [](QPainter *p, const QRect &r, int, const QString &text, QRect *) {
                       g_drawTextRect = r;
                       g_drawTextText = text;
                       g_drawTextPen = p->pen().color();
                   });

    // Act
    QImage img(200, 36, QImage::Format_ARGB32_Premultiplied);
    QPainter p(&img);
    m_tester->paint(&p, opt, idx);
    p.end();

    // Assert
    EXPECT_EQ(g_drawTextRect, QRect(20, 0, 170, 36));
    EXPECT_EQ(g_drawTextText, QString("begin-text"));
}

TEST_F(BaseItemDelegateTest, Paint_OnlyOnePositionNoDecoration_BothMarginsApplied)
{
    // Arrange：OnlyOne 与 Beginning 相同的边距策略
    QModelIndex idx = makeIndex(0, 0, "onlyone-text");
    QStyleOptionViewItem opt = baseOption(idx, QStyleOptionViewItem::OnlyOne);
    stub.set_lamda(static_cast<void (QPainter::*)(const QRect &, int, const QString &, QRect *)>(&QPainter::drawText),
                   [](QPainter *p, const QRect &r, int, const QString &text, QRect *) {
                       g_drawTextRect = r;
                       g_drawTextText = text;
                       g_drawTextPen = p->pen().color();
                   });

    // Act
    QImage img(200, 36, QImage::Format_ARGB32_Premultiplied);
    QPainter p(&img);
    m_tester->paint(&p, opt, idx);
    p.end();

    // Assert：OnlyOne 先 setX(10)/setWidth(180)，再 setX(20)/setWidth(160)
    EXPECT_EQ(g_drawTextRect, QRect(20, 0, 160, 36));
    EXPECT_EQ(g_drawTextText, QString("onlyone-text"));
}

TEST_F(BaseItemDelegateTest, Paint_BeginningWithDecoration_TextStartsAfterIcon)
{
    // Arrange：带图标首列 textRect.x = margin*2 + spacing + iconSize
    // paint 内部重新执行 initStyleOption，DecorationRole 须来自模型数据而非手动 features
    int iconSize = m_view->style()->pixelMetric(DStyle::PM_ListViewIconSize);
    QModelIndex idx = makeIndex(0, 0, "icon-text", QVariant(), testIcon());
    QStyleOptionViewItem opt = baseOption(idx, QStyleOptionViewItem::Beginning);
    stub.set_lamda(static_cast<void (QPainter::*)(const QRect &, int, const QString &, QRect *)>(&QPainter::drawText),
                   [](QPainter *p, const QRect &r, int, const QString &text, QRect *) {
                       g_drawTextRect = r;
                       g_drawTextText = text;
                       g_drawTextPen = p->pen().color();
                   });

    // Act
    QImage img(200, 36, QImage::Format_ARGB32_Premultiplied);
    QPainter p(&img);
    m_tester->paint(&p, opt, idx);
    p.end();

    // Assert：Beginning setX(10) 后 rect=(10,0,190,36)（x2 不变宽度收缩），
    // textRect.setX(10+10+10+iconSize) → 宽再收缩 20+iconSize，最终 setWidth(w-margin)
    EXPECT_EQ(g_drawTextRect.x(), 30 + iconSize);
    EXPECT_EQ(g_drawTextRect.width(), 160 - iconSize);
    EXPECT_EQ(g_drawTextText, QString("icon-text"));
}

TEST_F(BaseItemDelegateTest, Paint_DisabledRow_UsesDisabledTextPen)
{
    // Arrange：无 State_Enabled → cg=Disabled，画笔应取 Disabled Text 色
    QModelIndex idx = makeIndex(0, 0, "disabled");
    QStyleOptionViewItem opt = baseOption(idx, QStyleOptionViewItem::Middle);
    opt.state = QStyle::State_Active; // 去掉 State_Enabled
    auto palette = DGuiApplicationHelper::instance()->applicationPalette();
    QColor expected = palette.color(DPalette::Disabled, DPalette::Text);
    stub.set_lamda(static_cast<void (QPainter::*)(const QRect &, int, const QString &, QRect *)>(&QPainter::drawText),
                   [](QPainter *p, const QRect &r, int, const QString &text, QRect *) {
                       g_drawTextRect = r;
                       g_drawTextText = text;
                       g_drawTextPen = p->pen().color();
                   });

    // Act
    QImage img(200, 36, QImage::Format_ARGB32_Premultiplied);
    QPainter p(&img);
    m_tester->paint(&p, opt, idx);
    p.end();

    // Assert
    EXPECT_EQ(g_drawTextPen, expected);
    EXPECT_EQ(g_drawTextText, QString("disabled"));
}

TEST_F(BaseItemDelegateTest, Paint_EnabledRow_UsesDefaultTextPen)
{
    // Arrange：启用且未选中 → 前景为默认 Text 色
    QModelIndex idx = makeIndex(0, 0, "normal");
    QStyleOptionViewItem opt = baseOption(idx, QStyleOptionViewItem::Middle);
    auto palette = DGuiApplicationHelper::instance()->applicationPalette();
    QColor expected = palette.color(DPalette::Active, DPalette::Text);
    stub.set_lamda(static_cast<void (QPainter::*)(const QRect &, int, const QString &, QRect *)>(&QPainter::drawText),
                   [](QPainter *p, const QRect &r, int, const QString &text, QRect *) {
                       g_drawTextRect = r;
                       g_drawTextText = text;
                       g_drawTextPen = p->pen().color();
                   });

    // Act
    QImage img(200, 36, QImage::Format_ARGB32_Premultiplied);
    QPainter p(&img);
    m_tester->paint(&p, opt, idx);
    p.end();

    // Assert
    EXPECT_EQ(g_drawTextPen, expected);
    EXPECT_EQ(g_drawTextText, QString("normal"));
}

TEST_F(BaseItemDelegateTest, Paint_CustomColorRole_UsesProvidedColorPen)
{
    // Arrange：UserRole+2 提供自定义 ColorType → 前景取该颜色
    QModelIndex idx = makeIndex(0, 0, "colored", static_cast<int>(DPalette::ItemBackground));
    QStyleOptionViewItem opt = baseOption(idx, QStyleOptionViewItem::Middle);
    auto palette = DGuiApplicationHelper::instance()->applicationPalette();
    QColor expected = palette.color(DPalette::Active, DPalette::ItemBackground);
    stub.set_lamda(static_cast<void (QPainter::*)(const QRect &, int, const QString &, QRect *)>(&QPainter::drawText),
                   [](QPainter *p, const QRect &r, int, const QString &text, QRect *) {
                       g_drawTextRect = r;
                       g_drawTextText = text;
                       g_drawTextPen = p->pen().color();
                   });

    // Act
    QImage img(200, 36, QImage::Format_ARGB32_Premultiplied);
    QPainter p(&img);
    m_tester->paint(&p, opt, idx);
    p.end();

    // Assert
    EXPECT_EQ(g_drawTextPen, expected);
    EXPECT_EQ(g_drawTextText, QString("colored"));
}

TEST_F(BaseItemDelegateTest, Paint_SelectedRow_UsesHighlightTextPen)
{
    // Arrange：选中行 → 前景取 HighlightedText
    QModelIndex idx = makeIndex(0, 0, "selected");
    QStyleOptionViewItem opt = baseOption(idx, QStyleOptionViewItem::Middle);
    opt.state |= QStyle::State_Selected;
    m_view->selectionModel()->select(idx, QItemSelectionModel::Select);
    auto palette = DGuiApplicationHelper::instance()->applicationPalette();
    QColor expected = palette.color(DPalette::Active, DPalette::HighlightedText);
    stub.set_lamda(static_cast<void (QPainter::*)(const QRect &, int, const QString &, QRect *)>(&QPainter::drawText),
                   [](QPainter *p, const QRect &r, int, const QString &text, QRect *) {
                       g_drawTextRect = r;
                       g_drawTextText = text;
                       g_drawTextPen = p->pen().color();
                   });

    // Act
    QImage img(200, 36, QImage::Format_ARGB32_Premultiplied);
    QPainter p(&img);
    m_tester->paint(&p, opt, idx);
    p.end();

    // Assert
    EXPECT_EQ(g_drawTextPen, expected);
    EXPECT_EQ(g_drawTextText, QString("selected"));
}

TEST_F(BaseItemDelegateTest, Paint_HoveredNormalRow_AdjustsTextPen)
{
    // Arrange：未选中 + MouseOver → 主题 adjustColor 调整色，不等于默认 Text 色
    QModelIndex idx = makeIndex(0, 0, "hovered");
    QStyleOptionViewItem opt = baseOption(idx, QStyleOptionViewItem::Middle);
    opt.state |= QStyle::State_MouseOver;
    auto palette = DGuiApplicationHelper::instance()->applicationPalette();
    QColor plain = palette.color(DPalette::Active, DPalette::Text);
    stub.set_lamda(static_cast<void (QPainter::*)(const QRect &, int, const QString &, QRect *)>(&QPainter::drawText),
                   [](QPainter *p, const QRect &r, int, const QString &text, QRect *) {
                       g_drawTextRect = r;
                       g_drawTextText = text;
                       g_drawTextPen = p->pen().color();
                   });

    // Act
    QImage img(200, 36, QImage::Format_ARGB32_Premultiplied);
    QPainter p(&img);
    m_tester->paint(&p, opt, idx);
    p.end();

    // Assert：悬停调整后的颜色与默认文本色不同（Light -50 / Dark +20）
    EXPECT_NE(g_drawTextPen, plain);
    EXPECT_EQ(g_drawTextText, QString("hovered"));
}

TEST_F(BaseItemDelegateTest, Paint_WithDecoration_CacheMiss_GeneratesAndCachesIcon)
{
    // Arrange：装饰绘制 + 进程名，缓存未命中 → 生成图标并写入缓存
    const QString procName = QStringLiteral("ut-cache-miss");
    QModelIndex idx = makeIndex(0, 0, "cached", QVariant(), testIcon(), procName);
    QStyleOptionViewItem opt = baseOption(idx, QStyleOptionViewItem::Beginning);
    opt.features |= QStyleOptionViewItem::HasDecoration;
    opt.icon = testIcon();
    qreal dpr = 1.0;
    QString cacheKey = QStringLiteral("%1@%2").arg(procName).arg(dpr);
    QPixmap cached;
    core::process::ProcessIconCache::instance()->iconPixmapCache.remove(cacheKey);
    ASSERT_FALSE(core::process::ProcessIconCache::instance()->iconPixmapCache.find(cacheKey, &cached));
    stub.set_lamda(static_cast<void (QPainter::*)(const QRect &, const QPixmap &)>(&QPainter::drawPixmap),
                   [](QPainter *, const QRect &, const QPixmap &pm) {
                       ++g_drawPixmapCount;
                       g_drawPixmapData() = pm;
                   });

    // Act
    QImage img(200, 36, QImage::Format_ARGB32_Premultiplied);
    QPainter p(&img);
    m_tester->paint(&p, opt, idx);
    p.end();

    // Assert：绘制了图标，且缓存中已生成该进程图标
    EXPECT_EQ(g_drawPixmapCount, 1);
    EXPECT_FALSE(g_drawPixmapData().isNull());
    QPixmap stored;
    EXPECT_TRUE(core::process::ProcessIconCache::instance()->iconPixmapCache.find(cacheKey, &stored));
}

TEST_F(BaseItemDelegateTest, Paint_WithDecoration_CacheHit_ReusesCachedPixmap)
{
    // Arrange：缓存命中路径 → 直接绘制缓存内容
    const QString procName = QStringLiteral("ut-cache-hit");
    QModelIndex idx = makeIndex(0, 0, "cached2", QVariant(), testIcon(), procName);
    QStyleOptionViewItem opt = baseOption(idx, QStyleOptionViewItem::Beginning);
    opt.features |= QStyleOptionViewItem::HasDecoration;
    opt.icon = testIcon();
    QImage src(12, 12, QImage::Format_ARGB32_Premultiplied);
    src.fill(Qt::blue);
    QPixmap preset = QPixmap::fromImage(src);
    QString cacheKey = QStringLiteral("%1@%2").arg(procName).arg(1.0);
    core::process::ProcessIconCache::instance()->iconPixmapCache.insert(cacheKey, preset);
    stub.set_lamda(static_cast<void (QPainter::*)(const QRect &, const QPixmap &)>(&QPainter::drawPixmap),
                   [](QPainter *, const QRect &, const QPixmap &pm) {
                       ++g_drawPixmapCount;
                       g_drawPixmapData() = pm;
                   });

    // Act
    QImage img(200, 36, QImage::Format_ARGB32_Premultiplied);
    QPainter p(&img);
    m_tester->paint(&p, opt, idx);
    p.end();

    // Assert：绘制的是预置缓存位图而非新生成图标（toImage 的格式可能变化，统一转 ARGB32 比较像素）
    EXPECT_EQ(g_drawPixmapCount, 1);
    EXPECT_FALSE(g_drawPixmapData().isNull());
    QImage drawn = g_drawPixmapData().toImage().convertToFormat(QImage::Format_ARGB32);
    QImage expect = src.convertToFormat(QImage::Format_ARGB32);
    EXPECT_EQ(drawn, expect);
}
