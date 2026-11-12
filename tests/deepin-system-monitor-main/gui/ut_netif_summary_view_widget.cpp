// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

//Self
#include "netif_summary_view_widget.h"
#include "system/device_db.h"
#include "system/netif.h"
#include "system/netif_info_db.h"
#include "common/common.h"

//gtest
#include <gtest/gtest.h>

//Qt
#include <QPainter>
#include <QPaintEvent>
#include <QImage>

using namespace core::system;
using namespace common::format;

/***************************************FIXTURE begin*****************************************/
// 数据注入说明：测试构建为 -O2，NetifInfoDB::infoDB() 在头文件内联定义且被生产代码
// 全部内联（目标文件中无符号引用），首字节补丁式打桩无法生效。因此借助测试构建的
// -fno-access-control 直接向 DeviceDB 单例的 m_infoDB 写入合成数据，隔离真实系统数据源
// （不触发 Netlink//proc 读取），各测试结束时通过 clearInfoDB() 还原。
// 有线网卡 + IPv4/IPv6 地址，覆盖 refreshNetifInfo 的主要取数路径。
static std::shared_ptr<NetifInfo> makeWiredNetifInfo()
{
    auto info = std::make_shared<NetifInfo>();
    info->set_recv_bps(1024);
    info->set_sent_bps(512);

    // IPv4: addr 192.168.1.10/24, bcast 192.168.1.255 (addr/mask/bcast 均为字符串形式)
    inet_addr4_t addr4;
    addr4.family = AF_INET;
    addr4.addr = QByteArray("192.168.1.10");
    addr4.mask = QByteArray("255.255.255.0");
    addr4.bcast = QByteArray("192.168.1.255");
    info->updateAddr4Info({std::make_shared<inet_addr4_t>(addr4)});

    // IPv6: addr ::1/128 scope host
    inet_addr6_t addr6;
    addr6.family = AF_INET6;
    addr6.addr = QByteArray("::1");
    addr6.prefixlen = 128;
    addr6.scope = 0;
    info->updateAddr6Info({std::make_shared<inet_addr6_t>(addr6)});

    return info;
}

static const QMap<QByteArray, NetifInfoPtr> stub_infoDB_one()
{
    QMap<QByteArray, NetifInfoPtr> map;
    map.insert("aa", makeWiredNetifInfo());
    return map;
}

static const QMap<QByteArray, NetifInfoPtr> stub_infoDB_empty()
{
    return QMap<QByteArray, NetifInfoPtr>();
}

static void injectInfoDB(const QMap<QByteArray, NetifInfoPtr> &map)
{
    DeviceDB::instance()->netifInfoDB()->m_infoDB = map;
}

static void clearInfoDB()
{
    DeviceDB::instance()->netifInfoDB()->m_infoDB.clear();
}
/***************************************FIXTURE end**********************************************/

namespace {
// 在模型中按 strKey 查找行号，找不到返回 -1
int findRow(const NetInfoModel *model, const QString &key)
{
    for (int row = 0; row < model->rowCount(QModelIndex()); ++row) {
        if (model->data(model->index(row, 0), Qt::DisplayRole).toString() == key)
            return row;
    }
    return -1;
}

// 统计非纯白像素数：paint 类用例用它证明确实绘制了内容（而不仅是“不崩溃”）
int nonWhitePixels(const QImage &image)
{
    int drawn = 0;
    for (int y = 0; y < image.height(); ++y)
        for (int x = 0; x < image.width(); ++x)
            if (image.pixelColor(x, y) != Qt::white)
                ++drawn;
    return drawn;
}
}  // namespace

class UT_NetInfoModel : public ::testing::Test
{
public:
    UT_NetInfoModel() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        clearInfoDB();
        m_tester = new NetInfoModel;
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
        clearInfoDB();
    }

protected:
    NetInfoModel *m_tester;
};

TEST_F(UT_NetInfoModel, Ctor_DefaultState_TwoColumnEmptyModel)
{
    // Arrange：默认构造参数
    const QModelIndex root;

    // Act：默认构造（SetUp 完成）
    ASSERT_NE(m_tester, nullptr);

    // Assert：两列表格模型，初始无数据行
    EXPECT_EQ(m_tester->columnCount(root), 2);
    EXPECT_EQ(m_tester->rowCount(root), 0);
    EXPECT_TRUE(m_tester->m_listInfo.isEmpty());
}

TEST_F(UT_NetInfoModel, Refresh_UnknownMac_ModelStaysEmpty)
{
    // Arrange：空数据源（SetUp 完成 clearInfoDB）
    const QString unknownKey = QStringLiteral("unknown");

    // Act：以未知 MAC 刷新
    m_tester->refreshNetifInfo(unknownKey);

    // Assert：行数与内部列表均为空
    EXPECT_EQ(m_tester->rowCount(QModelIndex()), 0);
    EXPECT_TRUE(m_tester->m_listInfo.isEmpty());
}

// 分支说明(refreshNetifInfo): 多地址时 IPv4/IPv6 行带序号 / 无线网卡追加 ESSID/信号行
TEST_F(UT_NetInfoModel, test_refreshNetifInfo_04_multiAddr)
{
    // Arrange：2 个 IPv4 + 2 个 IPv6 的合成网卡注入 DB
    auto info = std::make_shared<NetifInfo>();
    info->set_recv_bps(1024);
    info->set_sent_bps(512);
    inet_addr4_t a4a, a4b;
    a4a.family = AF_INET;
    a4a.addr = QByteArray("192.168.1.10");
    a4a.mask = QByteArray("255.255.255.0");
    a4a.bcast = QByteArray("192.168.1.255");
    a4b.family = AF_INET;
    a4b.addr = QByteArray("10.0.0.5");
    a4b.mask = QByteArray("255.0.0.0");
    a4b.bcast = QByteArray("10.255.255.255");
    info->updateAddr4Info({std::make_shared<inet_addr4_t>(a4a), std::make_shared<inet_addr4_t>(a4b)});
    inet_addr6_t a6a, a6b;
    a6a.family = AF_INET6;
    a6a.addr = QByteArray("::1");
    a6a.prefixlen = 128;
    a6a.scope = 0;
    a6b.family = AF_INET6;
    a6b.addr = QByteArray("fe80::2");
    a6b.prefixlen = 64;
    a6b.scope = 252;  // link
    info->updateAddr6Info({std::make_shared<inet_addr6_t>(a6a), std::make_shared<inet_addr6_t>(a6b)});
    QMap<QByteArray, NetifInfoPtr> map;
    map.insert("aa", info);
    injectInfoDB(map);

    // Act：以命中 MAC 刷新
    m_tester->refreshNetifInfo("aa");

    // Assert：首地址无序号 + 后续地址带序号
    EXPECT_EQ(m_tester->rowCount(QModelIndex()), 19);  // 17 + IPv4 2 + IPv6 2 - 无序号重复计数
    int row4b = findRow(m_tester, QApplication::translate("NetInfoModel", "IPv4") + " 2");
    ASSERT_GE(row4b, 0);
    EXPECT_EQ(m_tester->data(m_tester->index(row4b, 1), Qt::DisplayRole).toString(),
              QString("10.0.0.5/255.0.0.0/10.255.255.255"));
    int row6b = findRow(m_tester, QApplication::translate("NetInfoModel", "IPv6") + " 2");
    ASSERT_GE(row6b, 0);
    EXPECT_EQ(m_tester->data(m_tester->index(row6b, 1), Qt::DisplayRole).toString(),
              QString("fe80::2/64/252"));
}

TEST_F(UT_NetInfoModel, test_refreshNetifInfo_05_wireless)
{
    // Arrange：无线网卡（isWireless + iw_info 私有成员直接注入）
    // isWireless/iw_info 位于私有成员 d（NetifInfoPrivate，-fno-access-control 可写）
    auto info = std::make_shared<NetifInfo>();
    info->d->isWireless = true;
    info->d->iw_info->essid = QByteArray("test-ssid");
    info->d->iw_info->qual.qual = 70;    // linkQuality
    // 注意：源码公式为 -100 + 60 * (level/100.0)，level=60 → -64 dBm
    info->d->iw_info->qual.level = 60;   // signalLevel
    info->d->iw_info->qual.noise = 55;   // noiseLevel
    QMap<QByteArray, NetifInfoPtr> map;
    map.insert("wlan", info);
    injectInfoDB(map);

    // Act：以无线 MAC 刷新
    m_tester->refreshNetifInfo("wlan");

    // Assert：ESSID/Link quality/Signal strength/Noise level 行存在且取值正确
    int rowEssid = findRow(m_tester, QApplication::translate("NetInfoModel", "ESSID"));
    ASSERT_GE(rowEssid, 0);
    EXPECT_EQ(m_tester->data(m_tester->index(rowEssid, 1), Qt::DisplayRole).toString(),
              QString("test-ssid"));
    int rowLq = findRow(m_tester, QApplication::translate("NetInfoModel", "Link quality"));
    ASSERT_GE(rowLq, 0);
    EXPECT_EQ(m_tester->data(m_tester->index(rowLq, 1), Qt::DisplayRole).toString(),
              QString("70"));
    int rowSs = findRow(m_tester, QApplication::translate("NetInfoModel", "Signal strength"));
    ASSERT_GE(rowSs, 0);
    // -100 + 60 * (60/100.0) = -64 dBm
    EXPECT_EQ(m_tester->data(m_tester->index(rowSs, 1), Qt::DisplayRole).toString(),
              QString("-64 dBm"));
    int rowNoise = findRow(m_tester, QApplication::translate("NetInfoModel", "Noise level"));
    ASSERT_GE(rowNoise, 0);
    EXPECT_EQ(m_tester->data(m_tester->index(rowNoise, 1), Qt::DisplayRole).toString(),
              QString("55 dBm"));
}

// 分支说明(refreshNetifInfo 无线段): essid 为空->无 ESSID 行 / noise==0->无 Noise level 行
TEST_F(UT_NetInfoModel, test_refreshNetifInfo_06_wirelessNoEssidNoNoise)
{
    // Arrange：无线网卡但 essid 为空、noise 为 0
    auto info = std::make_shared<NetifInfo>();
    info->d->isWireless = true;
    info->d->iw_info->essid = QByteArray("");
    info->d->iw_info->qual.qual = 40;
    info->d->iw_info->qual.level = 100;  // -100 + 60 * (100/100.0) = -40 dBm
    info->d->iw_info->qual.noise = 0;
    QMap<QByteArray, NetifInfoPtr> map;
    map.insert("wlan", info);
    injectInfoDB(map);

    // Act：刷新
    m_tester->refreshNetifInfo("wlan");

    // Assert：ESSID 与 Noise level 行不存在；信号强度行仍在且为 -40 dBm
    EXPECT_LT(findRow(m_tester, QApplication::translate("NetInfoModel", "ESSID")), 0);
    EXPECT_LT(findRow(m_tester, QApplication::translate("NetInfoModel", "Noise level")), 0);
    int rowSs = findRow(m_tester, QApplication::translate("NetInfoModel", "Signal strength"));
    ASSERT_GE(rowSs, 0);
    EXPECT_EQ(m_tester->data(m_tester->index(rowSs, 1), Qt::DisplayRole).toString(),
              QString("-40 dBm"));
}

// 分支说明(refreshNetifInfo): key命中且有IPv4->IPv4行 / 有IPv6->IPv6行 /
//                           wireless分支(跳过) / 连接类型+MAC+带宽+收发包字节错误行
TEST_F(UT_NetInfoModel, test_refreshNetifInfo_02_wiredNet)
{
    // Arrange：有线网卡 + IPv4 + IPv6 注入 DB
    injectInfoDB(stub_infoDB_one());

    // Act：以命中 MAC 刷新
    m_tester->refreshNetifInfo("aa");

    // Assert：IPv4 + IPv6 + Connection type + MAC + Bandwidth + RX(6) + TX(6) = 17 行
    EXPECT_EQ(m_tester->rowCount(QModelIndex()), 17);

    // IPv4 行：地址/掩码/广播
    int row4 = findRow(m_tester, "IPv4");
    ASSERT_GE(row4, 0);
    EXPECT_EQ(m_tester->data(m_tester->index(row4, 1), Qt::DisplayRole).toString(),
              QString("192.168.1.10/255.255.255.0/192.168.1.255"));

    // IPv6 行：地址/前缀/scope
    int row6 = findRow(m_tester, "IPv6");
    ASSERT_GE(row6, 0);
    EXPECT_EQ(m_tester->data(m_tester->index(row6, 1), Qt::DisplayRole).toString(),
              QString("::1/128/0"));

    // 连接类型行：合成 NetifInfo 的 conn_type 无公共 setter，默认构造为空
    int rowConn = findRow(m_tester, QApplication::translate("NetInfoModel", "Connection type"));
    ASSERT_GE(rowConn, 0);
    EXPECT_EQ(m_tester->data(m_tester->index(rowConn, 1), Qt::DisplayRole).toString(),
              QString());

    // RX packets 取自 recv 累计计数（默认 0）
    int rowRx = findRow(m_tester, QApplication::translate("NetInfoModel", "RX packets"));
    ASSERT_GE(rowRx, 0);
    EXPECT_EQ(m_tester->data(m_tester->index(rowRx, 1), Qt::DisplayRole).toString(), QString("0"));
}

TEST_F(UT_NetInfoModel, test_refreshNetifInfo_03_emptyDB)
{
    // Arrange：先填充有效数据并刷新一次
    {
        injectInfoDB(stub_infoDB_one());
        m_tester->refreshNetifInfo("aa");
        ASSERT_EQ(m_tester->rowCount(QModelIndex()), 17);
    }

    // Act：清空数据源后再次刷新
    injectInfoDB(stub_infoDB_empty());
    m_tester->refreshNetifInfo("aa");

    // Assert：refreshNetifInfo 重置旧行
    EXPECT_EQ(m_tester->rowCount(QModelIndex()), 0);
    EXPECT_TRUE(m_tester->m_listInfo.isEmpty());
}

TEST_F(UT_NetInfoModel, test_data_01_invalidAndOOB)
{
    // Arrange：空模型（未知 MAC 刷新后）
    m_tester->refreshNetifInfo("unknown");

    // Act：分别取非法根索引、越界行、越界负行、UserRole 四种取值
    const QVariant rootData = m_tester->data(QModelIndex(), Qt::DisplayRole);
    const QVariant oobData = m_tester->data(m_tester->index(0, 0), Qt::DisplayRole);
    const QVariant oobRole = m_tester->data(m_tester->index(0, 0), Qt::UserRole);
    const QVariant negData = m_tester->data(m_tester->index(-1, 0), Qt::DisplayRole);

    // Assert：非法/越界索引返回空 QVariant
    EXPECT_FALSE(rootData.isValid());
    EXPECT_FALSE(oobData.isValid());
    EXPECT_FALSE(oobRole.isValid());
    EXPECT_FALSE(negData.isValid());
}

TEST_F(UT_NetInfoModel, test_data_03_staleIndexAfterReset)
{
    // Arrange：先取有效索引
    injectInfoDB(stub_infoDB_one());
    m_tester->refreshNetifInfo("aa");
    QModelIndex stale = m_tester->index(0, 0);
    ASSERT_TRUE(stale.isValid());
    EXPECT_EQ(m_tester->data(stale, Qt::DisplayRole).toString(),
              QApplication::translate("NetInfoModel", "IPv4"));

    // Act：清空数据源刷新，旧索引失效
    injectInfoDB(stub_infoDB_empty());
    m_tester->refreshNetifInfo("aa");

    // Assert：旧索引越界 -> 走 iRow>=count 的空返回分支
    EXPECT_FALSE(m_tester->data(stale, Qt::DisplayRole).isValid());
    EXPECT_FALSE(m_tester->data(stale, Qt::UserRole).isValid());
}

TEST_F(UT_NetInfoModel, test_data_02_displayAndUserRole)
{
    // Arrange：注入数据并刷新
    injectInfoDB(stub_infoDB_one());
    m_tester->refreshNetifInfo("aa");
    ASSERT_GT(m_tester->rowCount(QModelIndex()), 0);

    // Act：分别取 DisplayRole 两列、DecorationRole、UserRole
    const QString displayKey = m_tester->data(m_tester->index(0, 0), Qt::DisplayRole).toString();
    const QString displayValue = m_tester->data(m_tester->index(0, 1), Qt::DisplayRole).toString();
    const QVariant decoration = m_tester->data(m_tester->index(0, 0), Qt::DecorationRole);
    ShowInfo info = m_tester->data(m_tester->index(0, 0), Qt::UserRole).value<ShowInfo>();

    // Assert：DisplayRole 返回 key/value 两列；其他 Role 无效；UserRole 返回 ShowInfo
    EXPECT_EQ(displayKey, m_tester->m_listInfo.at(0).strKey);
    EXPECT_EQ(displayValue, m_tester->m_listInfo.at(0).strValue);
    EXPECT_FALSE(decoration.isValid());
    EXPECT_EQ(info.strKey, m_tester->m_listInfo.at(0).strKey);
    EXPECT_EQ(info.strValue, m_tester->m_listInfo.at(0).strValue);
    EXPECT_EQ(info.eType, ShowInfo::IPV4);
}

class UT_NetInfoDetailItemDelegate : public ::testing::Test
{
public:
    UT_NetInfoDetailItemDelegate() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        clearInfoDB();
        static QWidget parent;
        m_tester = new NetInfoDetailItemDelegate(&parent);
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
        clearInfoDB();
    }

protected:
    NetInfoDetailItemDelegate *m_tester;
};

TEST_F(UT_NetInfoDetailItemDelegate, Ctor_DefaultState_DefaultFontAndNoData)
{
    // Arrange：默认字体参照
    const QFont defaultFont;

    // Act：默认构造（SetUp 完成）
    ASSERT_NE(m_tester, nullptr);

    // Assert：默认字体、指针有效
    EXPECT_TRUE(m_tester->m_font == defaultFont);
    EXPECT_EQ(m_tester->m_font.pointSize(), defaultFont.pointSize());
}

TEST_F(UT_NetInfoDetailItemDelegate, test_setFont_01)
{
    // Arrange：目标字体
    QFont font;
    font.setPointSize(18);
    font.setBold(true);

    // Act：设置字体
    m_tester->setFont(font);

    // Assert：字体完整同步到成员
    EXPECT_EQ(m_tester->m_font, font);
    EXPECT_EQ(m_tester->m_font.pointSize(), 18);
    EXPECT_TRUE(m_tester->m_font.bold());
}

// 分支说明(sizeHint): 非法index->230x36 / Normal ShowInfo->230x36 /
//                    IPV4|IPV6->按3行文本高度+2*TOPMARGIN
TEST_F(UT_NetInfoDetailItemDelegate, test_sizeHint_01_invalidIndex)
{
    // Arrange：空模型（index(0,0) 为非法索引）+ 空 option
    QStyleOptionViewItem option;
    NetInfoModel emptyModel;
    ASSERT_FALSE(emptyModel.index(0, 0).isValid());

    // Act：非法 index 与越界行分别取行高
    QSize sizeInvalid = m_tester->sizeHint(option, QModelIndex());
    QSize sizeInvalidRow = m_tester->sizeHint(option, emptyModel.index(0, 0));

    // Assert：均退回默认行高 230x36
    EXPECT_EQ(sizeInvalid, QSize(230, 36));
    EXPECT_EQ(sizeInvalidRow, QSize(230, 36));
}

TEST_F(UT_NetInfoDetailItemDelegate, test_sizeHint_02_ipRowTallHeight)
{
    // Arrange：注入数据并构造填充后的模型
    injectInfoDB(stub_infoDB_one());

    NetInfoModel model;
    model.refreshNetifInfo("aa");
    ASSERT_GT(model.rowCount(QModelIndex()), 0);

    // Act：IPv4 行（IPV4 类型）取行高
    QStyleOptionViewItem option;
    option.rect = QRect(0, 0, 300, 60);
    option.font = QFont();
    QSize size = m_tester->sizeHint(option, model.index(0, 0));

    // Assert：高度 = 3 行文本 + 2*TOPMARGIN(10)，宽度固定 230
    int fontHeight = QFontMetrics(m_tester->m_font).height();
    EXPECT_EQ(size, QSize(230, fontHeight * 3 + 2 * 10));
    EXPECT_EQ(size.width(), 230);
}

TEST_F(UT_NetInfoDetailItemDelegate, test_sizeHint_03_normalRowHeight)
{
    // Arrange：注入数据，找到 Normal 类型行
    injectInfoDB(stub_infoDB_one());

    NetInfoModel model;
    model.refreshNetifInfo("aa");
    ASSERT_GT(model.rowCount(QModelIndex()), 0);

    int macRow = -1;
    for (int row = 0; row < model.rowCount(QModelIndex()); ++row) {
        ShowInfo info = model.data(model.index(row, 0), Qt::UserRole).value<ShowInfo>();
        if (info.eType == ShowInfo::Normal) {
            macRow = row;
            break;
        }
    }
    ASSERT_GE(macRow, 0);

    // Act：Normal 行取行高
    QStyleOptionViewItem option;
    option.rect = QRect(0, 0, 300, 40);
    option.font = QFont();
    QSize size = m_tester->sizeHint(option, model.index(macRow, 0));

    // Assert：固定行高 36，不随 option.rect 变化
    EXPECT_EQ(size, QSize(230, 36));
    EXPECT_EQ(size.height(), 36);
}

// 分支说明(paint): 列1且IPV类型->按 IP/掩码/广播 三行绘制 / 列0且IPV类型->标题绘制 /
//                 其余->单行居中文本；奇偶行底色交替
TEST_F(UT_NetInfoDetailItemDelegate, test_paint_01_ipColumnTallRow)
{
    // Arrange：第 1 列 IPV4 行（IP address/Netmask/Broadcast 三行文本）
    injectInfoDB(stub_infoDB_one());

    NetInfoModel model;
    model.refreshNetifInfo("aa");
    ASSERT_GT(model.rowCount(QModelIndex()), 0);

    static QWidget optionWidget;
    QImage image(300, 60, QImage::Format_ARGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    QStyleOptionViewItem option;
    option.widget = &optionWidget;
    option.rect = QRect(0, 0, 300, 60);
    option.font = QFont();

    // Act：合法索引绘制一次；非法索引绘制一次作为对照
    m_tester->paint(&painter, option, model.index(0, 1));
    painter.end();

    QImage blank(300, 60, QImage::Format_ARGB32);
    blank.fill(Qt::white);
    QPainter painterBlank(&blank);
    m_tester->paint(&painterBlank, option, QModelIndex());
    painterBlank.end();

    // Assert：合法索引落了像素；非法索引（row=-1 走奇数行透明背景 + 无文本）不落像素
    EXPECT_GT(nonWhitePixels(image), 0);
    EXPECT_EQ(nonWhitePixels(blank), 0);
}

TEST_F(UT_NetInfoDetailItemDelegate, test_paint_02_normalRow)
{
    // Arrange：Normal 类型行（单行文本）
    injectInfoDB(stub_infoDB_one());

    NetInfoModel model;
    model.refreshNetifInfo("aa");
    ASSERT_GT(model.rowCount(QModelIndex()), 0);

    int macRow = -1;
    for (int row = 0; row < model.rowCount(QModelIndex()); ++row) {
        ShowInfo info = model.data(model.index(row, 0), Qt::UserRole).value<ShowInfo>();
        if (info.eType == ShowInfo::Normal) {
            macRow = row;
            break;
        }
    }
    ASSERT_GE(macRow, 0);

    static QWidget optionWidget;
    QImage image(300, 36, QImage::Format_ARGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    QStyleOptionViewItem option;
    option.widget = &optionWidget;
    option.rect = QRect(0, 0, 300, 36);
    option.font = QFont();

    // Act：合法索引绘制一次；非法索引绘制一次作为对照
    m_tester->paint(&painter, option, model.index(macRow, 1));
    painter.end();

    QImage blank(300, 36, QImage::Format_ARGB32);
    blank.fill(Qt::white);
    QPainter painterBlank(&blank);
    m_tester->paint(&painterBlank, option, QModelIndex());
    painterBlank.end();

    // Assert：合法索引落了像素；非法索引不落像素
    EXPECT_GT(nonWhitePixels(image), 0);
    EXPECT_EQ(nonWhitePixels(blank), 0);
}

TEST_F(UT_NetInfoDetailItemDelegate, test_paint_03_firstColumnIpTitle)
{
    // Arrange：第 0 列 IPV 行（仅标题文本）
    injectInfoDB(stub_infoDB_one());

    NetInfoModel model;
    model.refreshNetifInfo("aa");
    ASSERT_GT(model.rowCount(QModelIndex()), 0);

    static QWidget optionWidget;
    QImage image(300, 60, QImage::Format_ARGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    QStyleOptionViewItem option;
    option.widget = &optionWidget;
    option.rect = QRect(0, 0, 300, 60);
    option.font = QFont();

    // Act：合法索引绘制一次；非法索引绘制一次作为对照
    m_tester->paint(&painter, option, model.index(0, 0));
    painter.end();

    QImage blank(300, 60, QImage::Format_ARGB32);
    blank.fill(Qt::white);
    QPainter painterBlank(&blank);
    m_tester->paint(&painterBlank, option, QModelIndex());
    painterBlank.end();

    // Assert：标题文本已落在画布上；非法索引不落像素
    EXPECT_GT(nonWhitePixels(image), 0);
    EXPECT_EQ(nonWhitePixels(blank), 0);
}

TEST_F(UT_NetInfoDetailItemDelegate, test_paint_04_ipv6RowDarkTheme)
{
    // Arrange：深色主题 + IPv6 行第 1 列（Prefixlen/Scope 三行分支 + 最宽 key 计算）
    DGuiApplicationHelper::instance()->setPaletteType(DGuiApplicationHelper::ColorType::DarkType);
    injectInfoDB(stub_infoDB_one());

    NetInfoModel model;
    model.refreshNetifInfo("aa");
    int row6 = -1;
    for (int row = 0; row < model.rowCount(QModelIndex()); ++row) {
        ShowInfo info = model.data(model.index(row, 0), Qt::UserRole).value<ShowInfo>();
        if (info.eType == ShowInfo::IPV6) {
            row6 = row;
            break;
        }
    }
    ASSERT_GE(row6, 0);

    static QWidget optionWidget;
    QImage image(300, 60, QImage::Format_ARGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    QStyleOptionViewItem option;
    option.widget = &optionWidget;
    option.rect = QRect(0, 0, 300, 60);
    option.font = QFont();

    // Act：深色主题下绘制 IPv6 行；再以偶数行覆盖另一背景色分支
    m_tester->paint(&painter, option, model.index(row6, 1));
    painter.end();

    QImage image2(300, 60, QImage::Format_ARGB32);
    image2.fill(Qt::white);
    QPainter painter2(&image2);
    m_tester->paint(&painter2, option, model.index(0, 1));
    painter2.end();

    // Assert：两次绘制均落像素；深色主题与偶数行分支产物不同
    EXPECT_GT(nonWhitePixels(image), 0);
    EXPECT_GT(nonWhitePixels(image2), 0);
    EXPECT_FALSE(image == image2);

    // 还原浅色主题，避免影响其他用例
    DGuiApplicationHelper::instance()->setPaletteType(DGuiApplicationHelper::ColorType::LightType);
}

class UT_NetifSummaryViewWidget : public ::testing::Test
{
public:
    UT_NetifSummaryViewWidget() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        clearInfoDB();
        static QWidget parent;
        m_tester = new NetifSummaryViewWidget(&parent);
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
        clearInfoDB();
    }

protected:
    NetifSummaryViewWidget *m_tester;
};

TEST_F(UT_NetifSummaryViewWidget, Ctor_DefaultState_ModelAndDelegateWired)
{
    // Arrange：默认构造参数
    static QWidget parent;

    // Act：默认构造
    NetifSummaryViewWidget widget(&parent);

    // Assert：模型与委托装配、当前 key 为空、两列表头
    EXPECT_EQ(widget.model(), widget.m_netInfoModel);
    EXPECT_EQ(widget.itemDelegate(), widget.m_netInfoDetailItemDelegate);
    EXPECT_TRUE(widget.m_strCurrentKey.isEmpty());
    EXPECT_EQ(widget.horizontalHeader()->count(), 2);
}

TEST_F(UT_NetifSummaryViewWidget, test_fontChanged_01)
{
    // Arrange：目标字体
    QFont font;
    font.setBold(true);
    font.setPointSize(14);

    // Act：字体变化回调
    m_tester->fontChanged(font);

    // Assert：委托字体同步 + 表格固定高度 260
    EXPECT_EQ(m_tester->m_font, font);
    EXPECT_EQ(m_tester->font(), font);
    EXPECT_EQ(m_tester->m_netInfoDetailItemDelegate->m_font, font);
    EXPECT_EQ(m_tester->minimumHeight(), 260);
    EXPECT_EQ(m_tester->maximumHeight(), 260);
}

TEST_F(UT_NetifSummaryViewWidget, test_onNetifItemClicked_01)
{
    // Arrange：注入一条有线网卡数据
    injectInfoDB(stub_infoDB_one());

    // Act：点击条目
    m_tester->onNetifItemClicked("aa");

    // Assert：记录 key 并刷新模型数据
    EXPECT_EQ(m_tester->m_strCurrentKey, QString("aa"));
    EXPECT_EQ(m_tester->m_netInfoModel->rowCount(QModelIndex()), 17);
}

TEST_F(UT_NetifSummaryViewWidget, test_onNetifItemClicked_02_unknownMac)
{
    // Arrange：空数据源
    injectInfoDB(stub_infoDB_empty());

    // Act：点击未知 MAC
    m_tester->onNetifItemClicked("unknown");

    // Assert：key 已记录，模型清空
    EXPECT_EQ(m_tester->m_strCurrentKey, QString("unknown"));
    EXPECT_EQ(m_tester->m_netInfoModel->rowCount(QModelIndex()), 0);
}

TEST_F(UT_NetifSummaryViewWidget, test_onModelUpdate_01_refresh)
{
    // Arrange：注入数据并选中条目
    injectInfoDB(stub_infoDB_one());
    m_tester->onNetifItemClicked("aa");
    ASSERT_EQ(m_tester->m_netInfoModel->rowCount(QModelIndex()), 17);

    // Act：数据源清空后触发 DB 更新回调
    injectInfoDB(stub_infoDB_empty());
    m_tester->onModelUpdate();

    // Assert：以当前 key 重新取数（模型重置，key 保留）
    EXPECT_EQ(m_tester->m_netInfoModel->rowCount(QModelIndex()), 0);
    EXPECT_EQ(m_tester->m_strCurrentKey, QString("aa"));

    // 数据恢复后重新填充
    injectInfoDB(stub_infoDB_one());
    m_tester->onModelUpdate();
    EXPECT_EQ(m_tester->m_netInfoModel->rowCount(QModelIndex()), 17);
    EXPECT_EQ(m_tester->m_strCurrentKey, QString("aa"));
}

// 分支说明(paintEvent): 表头分隔竖线 + 圆角外框绘制
TEST_F(UT_NetifSummaryViewWidget, test_paintEvent_01)
{
    // Arrange：空表格先离屏渲染一次作为基线
    m_tester->resize(600, 400);
    QImage imageEmpty(600, 400, QImage::Format_ARGB32);
    imageEmpty.fill(Qt::white);
    m_tester->render(&imageEmpty);

    // Act：注入数据并选中条目后再次渲染（paintEvent 在 viewport 上自绘分隔线与外框）
    injectInfoDB(stub_infoDB_one());
    m_tester->onNetifItemClicked("aa");
    QImage image(600, 400, QImage::Format_ARGB32);
    image.fill(Qt::white);
    m_tester->render(&image);

    // Assert：表头/外框等已落在画布上；有数据后内容像素多于空表格基线
    EXPECT_GT(nonWhitePixels(image), 0);
    EXPECT_GT(nonWhitePixels(image), nonWhitePixels(imageEmpty));
}
