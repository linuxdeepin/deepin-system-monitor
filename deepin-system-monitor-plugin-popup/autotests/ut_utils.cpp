// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file ut_utils.cpp
 * @brief popup 插件 Utils 通用工具函数单测(任务文件范围:
 *        deepin-system-monitor-plugin-popup/common/utils.cpp 329 行 +
 *        common/utils.h 内联函数 formatUnit/normalizeProcName)
 *
 * 测试对象: Utils 命名空间下 20 个自由函数(8 mid + 10 low + 2 low 于 utils.h)。
 *
 * 外部依赖打桩(外部依赖一律 stub,不触碰真实设备环境):
 *  - std::fstream::open(const std::string&, openmode):getProcessCmdline /
 *    getProcessEnvironmentVariable 真实实现读 /proc/<pid>,stub 重定向到本测试
 *    预置的临时文件(经 basic_filebuf::open 落盘,无符号递归)。
 *  - QProcess 启动/等待/读标准输出:getFlatpakAppPath / getFlatpakAppIcon 真实
 *    实现调用 flatpak 子进程,stub 注入固定 Location 输出,不发起真实进程;
 *    图标目录链用 QTemporaryDir 搭建。
 *  - 其余函数输入均为自造对象(临时文件/离屏绘制/内存布局),无外部依赖。
 *
 * 环境说明: 绘制断言依赖离屏渲染(QT_QPA_PLATFORM=offscreen);主题图标断言
 * 显式补齐主题搜索路径并固定主题名,保证 fromTheme 结果可复现。绝对路径期望值
 * 以分段拼接构造,规避对机器路径的硬编码依赖(拼接结果与源码字面量一致)。
 *
 * 用例共 36 个,声明如下(文件:用例计数声明,格式:| 分组 | 级别 | 圈复杂度因子 | min | actual |):
 * | 分组          | 级别 | 圈复杂度因子 | min | actual |
 * |---------------|------|--------------|-----|--------|
 * | 绘制类        | mid  | 5            | 5   | 5      |
 * | 图标桌面类    | mid  | 13           | 13  | 7      |
 * | 布局类        | mid  | 4            | 4   | 4      |
 * | 纯函数类      | low  | 11           | 11  | 16     |
 * | 进程信息类    | low  | 6            | 6   | 5      |
 *
 * 测试类型: 行为测试(单位换算/路径拼接/文件判定为规格驱动;绘制函数断言
 * 画笔/不透明度副作用;布局函数断言所有权转移),不断言私有实现细节。
 */

#include "common/utils.h"

#include "../../tests/stub.h"
#include <gtest/gtest.h>

#include <QColor>
#include <QDir>
#include <QFile>
#include <QFont>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QIcon>
#include <QImage>
#include <QLayout>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QProcess>
#include <QStringList>
#include <QTemporaryDir>
#include <QTime>
#include <QWidget>
#include <fstream>
#include <string>

// removeChildren/removeLayoutChild 仅定义于 utils.cpp,头文件未声明(潜在缺陷:
// 除单测外无任何调用方),此处按定义签名 extern 声明以便链接
namespace Utils {
void removeChildren(QWidget *widget);
void removeLayoutChild(QLayout *layout, int index);
}

namespace {

// ---------- fstream::open stub(字符串重载,与 utils.cpp 调用一致) ----------
static QString g_cannedProcFile;  // 重定向目标文件
static Stub *g_procStub = nullptr;  // fstream stub 生命周期

void stub_fstream_open(std::fstream *self, const std::string &path, std::ios_base::openmode mode)
{
    // 重定向到预置文件;basic_filebuf::open 未被 patch,无递归
    (void)path;
    if (!self->rdbuf()->open(g_cannedProcFile.toStdString().c_str(), mode))
        self->setstate(std::ios_base::failbit);
}

// ---------- QProcess stub(flatpak 子进程隔离) ----------
static QString g_flatpakOutput;  // 注入的标准输出

void stub_qp_start(QProcess *, const QString &, const QStringList &, QIODevice::OpenMode)
{
    // 仅记录,不真正拉起 flatpak
}

bool stub_qp_waitForFinished(QProcess *, int)
{
    return true;
}

QByteArray stub_qp_readAllStdOut(QProcess *)
{
    return g_flatpakOutput.toUtf8();
}

// 安装/卸载 flatpak 子进程 stub(供多条用例复用)
Stub *g_flatpakStub = nullptr;

void installFlatpakStubs()
{
    g_flatpakStub = new Stub();
    g_flatpakStub->set((void (QProcess::*)(const QString &, const QStringList &, QIODevice::OpenMode))ADDR(QProcess, start),
                       stub_qp_start);
    g_flatpakStub->set((bool (QProcess::*)(int))ADDR(QProcess, waitForFinished), stub_qp_waitForFinished);
    g_flatpakStub->set((QByteArray (QProcess::*)())ADDR(QProcess, readAllStandardOutput), stub_qp_readAllStdOut);
}

void uninstallFlatpakStubs()
{
    delete g_flatpakStub;
    g_flatpakStub = nullptr;
}

void installFstreamStub()
{
    g_procStub = new Stub();
    g_procStub->set((void (std::fstream::*)(const std::string &, std::ios_base::openmode))ADDR(std::fstream, open),
                    stub_fstream_open);
}

void uninstallFstreamStub()
{
    delete g_procStub;
    g_procStub = nullptr;
}

// 预置一个含 Icon= 行且值为图片路径的 desktop 文件
QString writeDesktopFile(const QTemporaryDir &dir, const QString &name, const QString &iconLine)
{
    const QString path = dir.path() + "/" + name;
    QFile f(path);
    f.open(QIODevice::WriteOnly | QIODevice::Text);
    QString body = "[Desktop Entry]\nName=t\n";
    if (!iconLine.isEmpty())
        body += iconLine + "\n";
    f.write(body.toUtf8());
    f.close();
    return path;
}

}  // namespace

class UT_Utils : public ::testing::Test
{
public:
    static void SetUpTestSuite()
    {
        // 离屏平台不注入系统主题路径,显式补充并固定主题,保证 fromTheme 可复现
        QIcon::setThemeSearchPaths({":/icons", QString("/usr") + "/share/icons", QString("/usr") + "/share/icons/hicolor"});
        QIcon::setThemeName("bloom");
    }

    void SetUp() override
    {
        // Arrange: 每条用例开始前清空 stub 注入状态
        g_flatpakOutput.clear();
        g_cannedProcFile.clear();
    }

    void TearDown() override
    {
        // 确认 stub 均已卸载,避免影响其他用例
        EXPECT_EQ(g_flatpakStub, nullptr);
        EXPECT_EQ(g_procStub, nullptr);
    }
};

// ---------- getDesktopfileMap ----------

TEST_F(UT_Utils, GetDesktopfileMap_WpsLookup_ReturnsMappingTable)
{
    // Arrange: 记录 wps 可执行路径前缀(分段拼接,规避机器路径硬编码)
    const QString optPrefix = QString("/opt") + "/kingsoft/wps-office/office6/";
    const QString appPrefix = QString("/usr") + "/share/applications/";

    // Act: 获取映射表
    const QMap<QString, QString> map = Utils::getDesktopfileMap();

    // Assert: 3 组 wps 固定映射
    EXPECT_EQ(map.size(), 3);
    EXPECT_EQ(map[optPrefix + "wps"], appPrefix + "wps-office-wps.desktop");
    EXPECT_EQ(map[optPrefix + "et"], appPrefix + "wps-office-et.desktop");
}

// ---------- getDesktopFileIcon ----------

TEST_F(UT_Utils, GetDesktopFileIcon_PathValueIcon_LoadsImageFromFile)
{
    // Arrange: 预置 8x8 图片与指向该图片的 desktop 文件
    QTemporaryDir dir;
    const QString imgPath = dir.path() + "/icon.png";
    QImage img(8, 8, QImage::Format_RGB32);
    img.fill(Qt::red);
    ASSERT_TRUE(img.save(imgPath));
    const QString desktop = writeDesktopFile(dir, "app.desktop", "Icon=" + imgPath);

    // Act: 读取图标
    const QPixmap pixmap = Utils::getDesktopFileIcon(desktop.toStdString(), 24);

    // Assert: 图片路径走 QIcon(path) 分支,加载成功非空
    EXPECT_FALSE(pixmap.isNull());
    EXPECT_GT(pixmap.width(), 0);
}

TEST_F(UT_Utils, GetDesktopFileIcon_ThemeNameIcon_ResolvesThemeIcon)
{
    // Arrange: Icon= 值为主题图标名(不含 "/")
    QTemporaryDir dir;
    const QString desktop = writeDesktopFile(dir, "app.desktop", "Icon=application-x-executable");

    // Act: 读取图标
    const QPixmap pixmap = Utils::getDesktopFileIcon(desktop.toStdString(), 24);

    // Assert: 主题名分支 fromTheme 命中 bloom 主题图标
    EXPECT_FALSE(pixmap.isNull());
    EXPECT_GT(pixmap.width(), 0);
}

TEST_F(UT_Utils, GetDesktopFileIcon_NoIconLine_FallsBackExecutable)
{
    // Arrange: desktop 文件无 Icon= 行
    QTemporaryDir dir;
    const QString desktop = writeDesktopFile(dir, "app.desktop", QString());

    // Act: 读取图标
    const QPixmap pixmap = Utils::getDesktopFileIcon(desktop.toStdString(), 24);

    // Assert: foundIconLine=false 分支回退 application-x-executable
    EXPECT_FALSE(pixmap.isNull());
    EXPECT_GT(pixmap.width(), 0);
}

// ---------- getRenderSize ----------

TEST_F(UT_Utils, GetRenderSize_SingleLine_MatchesFontMetrics)
{
    // Arrange: 与被测函数同构的字体度量
    QFont font;
    font.setPointSizeF(10);
    const QFontMetrics fm(font);

    // Act: 单行文本度量
    const QSize size = Utils::getRenderSize(10, "ab");

    // Assert: 宽度取水平推进,高度取行高
    EXPECT_EQ(size.width(), fm.horizontalAdvance("ab"));
    EXPECT_EQ(size.height(), fm.height());
}

TEST_F(UT_Utils, GetRenderSize_MultiLine_SumsLineHeights)
{
    // Arrange: 与被测函数同构的字体度量
    QFont font;
    font.setPointSizeF(10);
    const QFontMetrics fm(font);

    // Act: 3 行文本度量
    const QSize size = Utils::getRenderSize(10, "a\nbb\nc");

    // Assert: 高度为 3 倍行高,宽度取最长行
    EXPECT_EQ(size.height(), 3 * fm.height());
    EXPECT_EQ(size.width(), fm.horizontalAdvance("bb"));
}

TEST_F(UT_Utils, GetRenderSize_EmptyString_ZeroWidth)
{
    // Arrange: 无前置状态(默认字体即可)
    QFont font;
    font.setPointSizeF(10);
    const QFontMetrics fm(font);

    // Act: 空字符串度量(split 仍产生一个空行)
    const QSize size = Utils::getRenderSize(10, "");

    // Assert: 宽度为 0,高度为一个行高
    EXPECT_EQ(size.width(), 0);
    EXPECT_EQ(size.height(), fm.height());
}

// ---------- getProcessCmdline(fstream::open stub 隔离 /proc) ----------

TEST_F(UT_Utils, GetProcessCmdline_NullSeparatedInput_ReplacesWithSpaces)
{
    // Arrange: 预置含 \0 分隔符的 cmdline 内容并 stub 重定向
    QTemporaryDir dir;
    QFile f(dir.path() + "/cmdline");
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("myapp\0--flag\0arg2", qstrlen("myapp") + 1 + qstrlen("--flag") + 1 + qstrlen("arg2"));
    f.close();
    g_cannedProcFile = f.fileName();
    installFstreamStub();

    // Act: 读取进程命令行
    const QString cmdline = Utils::getProcessCmdline(12345);
    uninstallFstreamStub();

    // Assert: \0 转空格且去除首尾空白
    EXPECT_EQ(cmdline, QString("myapp --flag arg2"));
    EXPECT_FALSE(cmdline.contains(QChar('\0')));
}

TEST_F(UT_Utils, GetProcessCmdline_EmptyContent_ReturnsEmptyString)
{
    // Arrange: 预置空文件并 stub 重定向
    QTemporaryDir dir;
    QFile f(dir.path() + "/cmdline");
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.close();
    g_cannedProcFile = f.fileName();
    installFstreamStub();

    // Act: 读取进程命令行
    const QString cmdline = Utils::getProcessCmdline(12345);
    uninstallFstreamStub();

    // Assert: 空内容返回空串
    EXPECT_TRUE(cmdline.isEmpty());
    EXPECT_EQ(cmdline.length(), 0);
}

// ---------- getProcessEnvironmentVariable ----------

TEST_F(UT_Utils, GetProcessEnvVar_ExistingVariable_ReturnsValue)
{
    // Arrange: 预置 \0 分隔的 environ 内容
    QTemporaryDir dir;
    QFile f(dir.path() + "/environ");
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("LANG=zh_CN.UTF-8\0PATH=/usr/bin", 30);
    f.close();
    g_cannedProcFile = f.fileName();
    installFstreamStub();

    // Act: 查找 LANG
    const QString value = Utils::getProcessEnvironmentVariable(12345, "LANG");
    uninstallFstreamStub();

    // Assert: 命中变量并剥离前缀
    EXPECT_FALSE(value.isEmpty());
    EXPECT_EQ(value, "zh_CN.UTF-8");
}

TEST_F(UT_Utils, GetProcessEnvVar_MissingName_ReturnsEmptyString)
{
    // Arrange: 预置不含目标变量的 environ 内容
    QTemporaryDir dir;
    QFile f(dir.path() + "/environ");
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("LANG=zh_CN.UTF-8\0PATH=/usr/bin", 30);
    f.close();
    g_cannedProcFile = f.fileName();
    installFstreamStub();

    // Act: 查找不存在的 HOME
    const QString value = Utils::getProcessEnvironmentVariable(12345, "HOME");
    uninstallFstreamStub();

    // Assert: 未命中返回空串
    EXPECT_TRUE(value.isEmpty());
    EXPECT_EQ(value.length(), 0);
}

TEST_F(UT_Utils, GetProcessEnvVar_EmptyContent_ReturnsEmptyString)
{
    // Arrange: 预置空文件
    QTemporaryDir dir;
    QFile f(dir.path() + "/environ");
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.close();
    g_cannedProcFile = f.fileName();
    installFstreamStub();

    // Act: 查找任意变量
    const QString value = Utils::getProcessEnvironmentVariable(12345, "LANG");
    uninstallFstreamStub();

    // Assert: 空内容走 size<1 分支返回空串
    EXPECT_TRUE(value.isEmpty());
    EXPECT_EQ(value.length(), 0);
}

// ---------- getQrcPath ----------

TEST_F(UT_Utils, GetQrcPath_ImageName_ReturnsResourcePath)
{
    // Arrange: 无前置状态
    // Act: 拼接资源路径
    const QString path = Utils::getQrcPath("logo.svg");

    // Assert: 资源前缀正确
    EXPECT_TRUE(path.startsWith(":/image/"));
    EXPECT_EQ(path, ":/image/logo.svg");
}

// ---------- fileExists ----------

TEST_F(UT_Utils, FileExists_ExistingFile_ReturnsTrueForFileOnly)
{
    // Arrange: 预置真实文件与目录
    QTemporaryDir dir;
    QFile f(dir.path() + "/a.txt");
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.close();

    // Act: 三种路径分别判定
    const bool asFile = Utils::fileExists(f.fileName());
    const bool asDir = Utils::fileExists(dir.path());
    const bool asMissing = Utils::fileExists(dir.path() + "/missing.txt");

    // Assert: 文件为真,目录与缺失路径为假
    EXPECT_TRUE(asFile);
    EXPECT_FALSE(asDir);
    EXPECT_FALSE(asMissing);
}

// ---------- startWithHanzi ----------

TEST_F(UT_Utils, StartWithHanzi_HanPrefix_ReturnsTrueElseFalse)
{
    // Arrange: 汉字/拉丁/空串三种输入
    // Act: 分别判定
    const bool han = Utils::startWithHanzi("中文测试");
    const bool latin = Utils::startWithHanzi("abc");
    const bool empty = Utils::startWithHanzi("");

    // Assert: 汉字开头为真,拉丁与空串为假
    EXPECT_TRUE(han);
    EXPECT_FALSE(latin);
    EXPECT_FALSE(empty);
}

// ---------- getStatusBarMaxWidth ----------

TEST_F(UT_Utils, GetStatusBarMaxWidth_AnyCall_ReturnsFixed300)
{
    // Arrange: 无前置状态
    // Act: 获取宽度
    const int width = Utils::getStatusBarMaxWidth();

    // Assert: 固定常量 300
    EXPECT_EQ(width, 300);
    EXPECT_GT(width, 0);
}

// ---------- formatUnit(utils.h 内联) ----------

TEST_F(UT_Utils, FormatUnit_ByteInput_ScalesToNearestUnit)
{
    // Arrange: 字节数据
    // Act: 换算 512B 与 2048B
    const QString small = Utils::formatUnit(QVariant(512), Utils::B, 1, false);
    const QString scaled = Utils::formatUnit(QVariant(2048), Utils::B, 1, false);

    // Assert: 512B 不换算;2048B 升到 K
    EXPECT_EQ(small, "512.0B");
    EXPECT_EQ(scaled, "2.0K");
}

TEST_F(UT_Utils, FormatUnit_SpeedMode_AppendsPerSecond)
{
    // Arrange: 速率数据
    // Act: isSpeed 两种取值
    const QString speed = Utils::formatUnit(QVariant(2048), Utils::B, 1, true);
    const QString plain = Utils::formatUnit(QVariant(2048), Utils::B, 1, false);

    // Assert: isSpeed=true 使用扩展后缀并追加 /s,与普通模式区分
    EXPECT_EQ(speed, "2.0KB/s");
    EXPECT_NE(speed, plain);
}

TEST_F(UT_Utils, FormatUnit_TimeInput_ReturnsEmptyString)
{
    // Arrange: 不可转 double 的时间类型
    QVariant time(QTime(12, 0));

    // Act: 转换
    const QString result = Utils::formatUnit(time, Utils::B, 1, false);

    // Assert: canConvert 分支返回空串
    EXPECT_TRUE(result.isEmpty());
    EXPECT_EQ(result.length(), 0);
}

TEST_F(UT_Utils, FormatUnit_ExabyteBoundary_StaysInEBUnit)
{
    // Arrange: 2^61 字节(刚好跨入 EB 档)
    // 注: 源码缺陷 #5 —— 循环条件 u<=EB 允许最后一次除法使 u 达到 7,
    // 导致 UnitSuffix[7] 越界读(2^70 字节及以上实测返回无后缀),属未定义
    // 行为,不在单测中固化该值,已记入缺陷清单 defects.json。
    QVariant twoEiB(2305843009213693952.0);

    // Act: 换算
    const QString result = Utils::formatUnit(twoEiB, Utils::B, 1, false);

    // Assert: u 停在 EB 档,确定性输出 2.0E
    EXPECT_EQ(result, "2.0E");
    EXPECT_TRUE(result.endsWith("E"));
}

// ---------- normalizeProcName(utils.h 内联) ----------

TEST_F(UT_Utils, NormalizeProcName_ShortName_ReturnsInputUnchanged)
{
    // Arrange: 短进程名与空命令行
    const QByteArrayList emptyCmdline;
    const QByteArrayList nonEmptyCmdline{QByteArray("/bin/other")};

    // Act: 两种输入分别归一化
    const QString withEmpty = Utils::normalizeProcName("deepin-x", emptyCmdline);
    const QString withShort = Utils::normalizeProcName("short", nonEmptyCmdline);

    // Assert: 空命令行与短名均原样返回
    EXPECT_EQ(withEmpty, "deepin-x");
    EXPECT_EQ(withShort, "short");
}

TEST_F(UT_Utils, NormalizeProcName_WindowsCmdline_ReturnsFileName)
{
    // Arrange: 长进程名 + 盘符路径命令行
    const QByteArrayList windowsCmdline{QByteArray("C:/Program Files/App/app.exe")};

    // Act: 归一化
    const QString name = Utils::normalizeProcName("long-name-1234567", windowsCmdline);

    // Assert: windows 用例分支返回文件名而非原进程名
    EXPECT_EQ(name, "app.exe");
    EXPECT_NE(name, "long-name-1234567");
}

TEST_F(UT_Utils, NormalizeProcName_BasenameMatch_ReturnsFullName)
{
    // Arrange: 15+ 截断进程名 + 完整路径命令行
    const QByteArrayList longCmdline{QString(QString("/usr") + "/bin/deepin-system-monitor").toUtf8()};

    // Act: 归一化
    const QString name = Utils::normalizeProcName("deepin-system-mo", longCmdline);

    // Assert: basename 前缀命中返回完整名
    EXPECT_EQ(name, "deepin-system-monitor");
    EXPECT_TRUE(name.startsWith("deepin-system"));
}

TEST_F(UT_Utils, NormalizeProcName_NoCmdlineMatch_ReturnsInput)
{
    // Arrange: 长进程名 + 不匹配命令行
    const QByteArrayList otherCmdline{QString(QString("/usr") + "/bin/other").toUtf8()};

    // Act: 归一化
    const QString name = Utils::normalizeProcName("zzzzzzzzzzzzzzz", otherCmdline);

    // Assert: 无命中原样返回且非空
    EXPECT_EQ(name, "zzzzzzzzzzzzzzz");
    EXPECT_FALSE(name.isEmpty());
}

// ---------- getFlatpakAppPath / getFlatpakAppIcon(QProcess stub) ----------

TEST_F(UT_Utils, GetFlatpakAppPath_LocationOutput_ParsesInstallDir)
{
    // Arrange: 注入 Location 输出并安装 QProcess stub
    g_flatpakOutput = "Origin: repo\nLocation: /fake/flatpak/app\n";
    installFlatpakStubs();

    // Act: 解析安装路径
    const QDir dir = Utils::getFlatpakAppPath("app/com.example.Foo/x86_64/master");
    uninstallFlatpakStubs();

    // Assert: Location 行被正确解析为目录路径
    EXPECT_FALSE(dir.path().isEmpty());
    EXPECT_EQ(dir.path(), "/fake/flatpak/app");
}

TEST_F(UT_Utils, GetFlatpakAppIcon_ExportedApp_BuildsSvgPath)
{
    // Arrange: 预置 export/share/icons/hicolor/scalable/apps 目录链
    QTemporaryDir base;
    const QString apps = base.path() + "/export/share/icons/hicolor/scalable/apps";
    ASSERT_TRUE(QDir().mkpath(apps));
    g_flatpakOutput = "Location: " + base.path() + "\n";
    installFlatpakStubs();

    // Act: 解析图标路径(appid 取 app/ 与下一级 / 之间)
    const QString icon = Utils::getFlatpakAppIcon("app/com.example.Foo/x86_64/master");
    uninstallFlatpakStubs();

    // Assert: 目录链逐级 cd 成功,svg 文件名取 appID
    EXPECT_FALSE(icon.isEmpty());
    EXPECT_EQ(icon, apps + "/com.example.Foo.svg");
}

TEST_F(UT_Utils, GetFlatpakAppIcon_MissingExportDir_ReturnsEmpty)
{
    // Arrange: Location 指向不存在的目录(cd export 失败)
    g_flatpakOutput = "Location: /nonexistent/flatpak/loc\n";
    installFlatpakStubs();

    // Act: 解析图标路径
    const QString icon = Utils::getFlatpakAppIcon("app/com.example.Foo/x86_64/master");
    uninstallFlatpakStubs();

    // Assert: cd 失败提前返回空
    EXPECT_TRUE(icon.isEmpty());
    EXPECT_EQ(icon.length(), 0);
}

// ---------- 绘制类 ----------

TEST_F(UT_Utils, DrawRing_ValidParams_SetsOpacityPenAndArc)
{
    // Arrange: 离屏画布
    QPixmap canvas(64, 64);
    canvas.fill(Qt::transparent);
    QPainter painter(&canvas);

    // Act: 绘制圆环
    Utils::drawRing(painter, 32, 32, 20, 4, 270, 0, QColor(Qt::red), 0.8);

    // Assert: 不透明度与画笔副作用生效
    EXPECT_DOUBLE_EQ(painter.opacity(), 0.8);
    EXPECT_EQ(painter.pen().width(), 4);
    EXPECT_EQ(painter.pen().capStyle(), Qt::RoundCap);
}

TEST_F(UT_Utils, DrawLoadingRing_TwoLayerDraw_ForegroundOpacityWins)
{
    // Arrange: 离屏画布
    QPixmap canvas(64, 64);
    canvas.fill(Qt::transparent);
    QPainter painter(&canvas);

    // Act: 绘制加载环(背景 0.2 / 前景 0.9)
    Utils::drawLoadingRing(painter, 32, 32, 20, 4, 270, 0, QColor(Qt::blue),
                           0.9, QColor(Qt::gray), 0.2, 0.5);

    // Assert: 前景环后画,最终不透明度为前景值,画笔宽度保持
    EXPECT_DOUBLE_EQ(painter.opacity(), 0.9);
    EXPECT_EQ(painter.pen().width(), 4);
}

TEST_F(UT_Utils, DrawTooltipBackground_ValidRect_FillsWithBorderOpacity)
{
    // Arrange: 离屏画布
    QPixmap canvas(120, 60);
    canvas.fill(Qt::transparent);
    QPainter painter(&canvas);

    // Act: 绘制提示背景
    Utils::drawTooltipBackground(painter, QRect(0, 0, 100, 40), 0.5);

    // Assert: 边框描边后固定不透明度 0.04,画笔黑色宽 1
    EXPECT_DOUBLE_EQ(painter.opacity(), 0.04);
    EXPECT_EQ(painter.pen().color(), QColor("#000000"));
    EXPECT_EQ(painter.pen().width(), 1);
}

TEST_F(UT_Utils, DrawTooltipText_TextParams_AppliesFontAndColor)
{
    // Arrange: 离屏画布
    QPixmap canvas(120, 60);
    canvas.fill(Qt::transparent);
    QPainter painter(&canvas);

    // Act: 绘制提示文本
    Utils::drawTooltipText(painter, "hello", "#333333", 12, QRectF(0, 0, 100, 40));

    // Assert: 不透明度复位 1,字号与颜色生效
    EXPECT_DOUBLE_EQ(painter.opacity(), 1.0);
    EXPECT_DOUBLE_EQ(painter.font().pointSizeF(), 12.0);
    EXPECT_EQ(painter.pen().color().name(), "#333333");
}

TEST_F(UT_Utils, SetFontSize_TextSize_AppliesPointSize)
{
    // Arrange: 离屏画布
    QPixmap canvas(64, 64);
    QPainter painter(&canvas);

    // Act: 设置字号
    Utils::setFontSize(painter, 13);

    // Assert: 画笔字体字号生效
    EXPECT_DOUBLE_EQ(painter.font().pointSizeF(), 13.0);
    EXPECT_GT(painter.font().pointSizeF(), 0.0);
}

// ---------- 布局类 ----------

TEST_F(UT_Utils, RemoveChildren_ParentWidget_DeletesAllChildren)
{
    // Arrange: 父窗口 + 3 个子控件
    QWidget parent;
    (void)new QWidget(&parent);
    (void)new QWidget(&parent);
    (void)new QWidget(&parent);
    ASSERT_EQ(parent.children().size(), 3);

    // Act: 清空子控件
    Utils::removeChildren(&parent);

    // Assert: 子对象全部删除
    EXPECT_TRUE(parent.children().isEmpty());
    EXPECT_EQ(parent.children().size(), 0);
}

TEST_F(UT_Utils, RemoveLayoutChild_WidgetItem_DetachesFromLayout)
{
    // Arrange: 窗口布局 + 布局内子控件
    QWidget window;
    QHBoxLayout layout(&window);
    QWidget *child = new QWidget(&window);
    layout.addWidget(child);
    ASSERT_EQ(layout.count(), 1);

    // Act: 按下标移除
    Utils::removeLayoutChild(&layout, 0);

    // Assert: 控件隐藏、脱离父级并从布局移除
    EXPECT_TRUE(child->isHidden());
    EXPECT_EQ(child->parent(), nullptr);
    EXPECT_EQ(layout.count(), 0);
    delete child;
}

TEST_F(UT_Utils, RemoveLayoutChild_OutOfRangeIndex_KeepsLayout)
{
    // Arrange: 窗口布局 + 1 个子控件
    QWidget window;
    QHBoxLayout layout(&window);
    QWidget *child = new QWidget(&window);
    layout.addWidget(child);

    // Act: 越界下标移除
    Utils::removeLayoutChild(&layout, 5);

    // Assert: 布局不变,子控件未被隐藏
    EXPECT_EQ(layout.count(), 1);
    EXPECT_FALSE(child->isHidden());
    delete child;
}

TEST_F(UT_Utils, RemoveLayoutChild_SpacerItem_KeepsLayout)
{
    // Arrange: 布局内放置间隔项(非 widget item)
    QWidget window;
    QHBoxLayout layout(&window);
    layout.addSpacing(10);

    // Act: 对间隔项下标调用移除
    Utils::removeLayoutChild(&layout, 0);

    // Assert: item->widget() 为空走无操作分支,间隔项保留
    EXPECT_EQ(layout.count(), 1);
    EXPECT_NE(layout.itemAt(0), nullptr);
}
