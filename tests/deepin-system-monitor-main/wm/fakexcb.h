// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// fakexcb.h：wm 模块单元测试共享的 xcb 假环境。
//
// 说明：
// - wm 层直接依赖 xcb C 接口（xcb_connect / xcb_get_property / xcb_query_tree 等），
//   测试不得依赖真实 X 服务器（环境隔离），因此用 stub 拦截 xcb 入口，
//   返回按本文件注册表构造的伪造 cookie / reply。
// - 所有被拦截的 xcb 函数均为 libxcb 导出的真实函数（非头文件 static inline），
//   Debug(-O0) 构建下 stub.h 的入口补丁可稳定命中。
// - reply 结构体严格按 /usr/include/xcb/xproto.h 布局伪造，value 数据紧随结构体，
//   xcb_get_property_value / xcb_get_property_value_length 等真实访问函数可直接工作。
// - 本文件被多个测试 TU include：FakeX 为 TU 局部单例，静态桩函数为 TU 局部符号，
//   测试用例串行执行，各 TU 的注册表互不干扰。

#ifndef UT_WM_FAKEXCB_H
#define UT_WM_FAKEXCB_H

#include <stub.h>

#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>

#include <QByteArray>
#include <QHash>
#include <QMap>
#include <QString>
#include <QVector>

#include <cstdlib>
#include <cstring>

namespace ut_wm {

// ---------------- 伪造窗口 ----------------
struct FakeWindow
{
    xcb_window_t id = 0;
    pid_t pid = -1;                       // _NET_WM_PID（hasPidProp=false 时无此属性 → pid -1）
    bool hasPidProp = false;
    QString name;                         // _NET_WM_NAME（UTF8_STRING）；空串 → 无该属性
    QString wmName;                       // WM_NAME（ICCCM STRING 回退）
    bool hasDesktopProp = false;
    int desktop = 0;                      // _NET_WM_DESKTOP
    uint32_t mapState = XCB_MAP_STATE_VIEWABLE;
    uint32_t wclass = XCB_WINDOW_CLASS_INPUT_OUTPUT;
    QVector<xcb_atom_t> types;            // _NET_WM_WINDOW_TYPE（atom id 列表）
    QVector<xcb_atom_t> states;           // _NET_WM_STATE
    uint32_t extents[4] = { 0, 0, 0, 0 }; // _NET_FRAME_EXTENTS left,right,top,bottom
    uint16_t x = 0, y = 0;                // geometry（仅占位）
    uint16_t width = 0, height = 0;
    uint16_t border = 0;
    int16_t dstX = 0, dstY = 0;           // translate_coordinates 结果
    xcb_window_t parent = 0;
    QVector<xcb_window_t> childrenBottomUp;   // query_tree children（X 返回序：底→顶）
    QVector<uint32_t> iconData;           // _NET_WM_ICON 原始数据 {w,h,px...,w,h,px...}
};

// ---------------- 属性载荷 ----------------
struct FakeProp
{
    uint8_t format = 32;
    xcb_atom_t type = XCB_ATOM_NONE;
    QVector<uint32_t> words;   // format=32 数据
    QByteArray bytes;          // format=8 数据
    bool fail = false;         // xcb_get_property_reply 返回 nullptr
};

// ---------------- 假 X 世界（TU 局部单例）----------------
class FakeX
{
public:
    static FakeX &instance()
    {
        static FakeX world;
        return world;
    }

    FakeX() { rebuildConnStorage(); }

    void reset()
    {
        *this = FakeX();
        screenStorage[0].root = root;   // 同步首屏 root（WMConnection 构造读取 screen->root）
    }

    void rebuildConnStorage()
    {
        conn = reinterpret_cast<xcb_connection_t *>(&connStorage[0]);
    }

    // ---------- 连接层 ----------
    xcb_connection_t *conn = nullptr;
    char connStorage[16] = {};
    int connectError = 0;             // xcb_connection_has_error 返回值
    int connectCalls = 0;
    QByteArray lastDisplayName;       // 传给 xcb_connect 的 displayname（空 = nullptr，最后一次调用）
    QVector<QByteArray> displayNames; // 每次 xcb_connect 的 displayname 记录（按调用序）
    int screenNumber = 0;             // xcb_connect 写回的 screen number
    int screenCount = 1;              // xcb_setup_roots_length 返回值
    xcb_window_t root = 0x1;          // 第一个 screen 的 root window
    int disconnectCount = 0;          // xcb_disconnect 调用计数
    int flushCount = 0;
    int getAtomNameCalls = 0;         // xcb_get_atom_name 调用计数（缓存命中验证）

    xcb_screen_t screenStorage[4] = {};

    // ---------- atom ----------
    int nextAtomId = 2000;
    QMap<QByteArray, xcb_atom_t> atomByName;
    QMap<xcb_atom_t, QByteArray> nameByAtom;

    xcb_atom_t internAtom(const QByteArray &name)
    {
        auto it = atomByName.constFind(name);
        if (it != atomByName.constEnd())
            return it.value();
        const xcb_atom_t id = static_cast<xcb_atom_t>(nextAtomId++);
        atomByName.insert(name, id);
        nameByAtom.insert(id, name);
        return id;
    }

    QByteArray atomName(xcb_atom_t atom) const { return nameByAtom.value(atom); }

    // ---------- cookie/reply 注册表（sequence → 数据）----------
    unsigned seqCounter = 0;
    unsigned icccmNameRequests = 0;   // xcb_icccm_get_wm_name 请求计数

    struct SeqEntry
    {
        xcb_window_t win = 0;       // geometry/attrs/coords/tree 请求的窗口
        xcb_atom_t atom = 0;        // intern/atom_name/get_property 的 atom
        FakeProp prop;              // get_property 载荷
    };
    QMap<unsigned, SeqEntry> seqTable;

    // 失败注入：返回 nullptr 的 reply 计数（>0 时命中一次减一）
    int failInternReply = 0;
    int failPropertyReply = 0;
    int failGeometryReply = 0;
    int failAttrReply = 0;
    int failCoordsReply = 0;
    int failTreeReply = 0;
    int failAtomNameReply = 0;
    int failIcccmNameReply = 0;
    // 按窗口的定向失败注入（构造期 buildWindowTreeSchema 逐窗口驱动时使用）
    QHash<xcb_window_t, int> failCoordsReplyFor;

    // ---------- 窗口 ----------
    QMap<xcb_window_t, FakeWindow> windows;
    QVector<xcb_window_t> clientListBottomUp;   // root 的 _NET_CLIENT_LIST_STACKING
    bool hasClientList = false;

    FakeWindow &window(xcb_window_t id) { return windows[id]; }

    // ---------- getTrayWindows 注入 ----------
    QList<uint32_t> trayWindows;
    bool trayReplyEmpty = false;   // true → arguments() 返回空列表（触发 first() 崩溃路径，勿用）

    // ---------- Process /proc 桩数据 ----------
    QString procUser = "test";                    // Process::userName 桩返回值
    uint defaultUid = 1000;                       // 未注册 pid 的默认 uid
    QMap<pid_t, uint> procUid;                    // pid → uid 覆盖
    QMap<pid_t, QByteArrayList> procCmdline;      // pid → cmdline 桩返回值

    // ---------- 构造标准世界 ----------
    // top->bottom: normal(0x101) → dock(0x102) → hidden(0x103) → desktop(0x104)
    //              → inputonly(0x105) → nopid(0x106) → unmapped(0x107)
    // query_tree children 按 X 返回序（底→顶）注册。
    void buildStandardWorld()
    {
        reset();
        const xcb_window_t rootId = root;
        FakeWindow normal;
        normal.id = 0x101;
        normal.pid = 2001;
        normal.hasPidProp = true;
        normal.name = "Text Editor";
        normal.wmName = "Text Editor";
        normal.hasDesktopProp = true;
        normal.desktop = 0;
        normal.wclass = XCB_WINDOW_CLASS_INPUT_OUTPUT;
        normal.types << internAtom("_NET_WM_WINDOW_TYPE_NORMAL");
        normal.states.clear();
        normal.extents[0] = 4; normal.extents[1] = 4; normal.extents[2] = 4; normal.extents[3] = 4;
        normal.x = 98; normal.y = 98; normal.width = 300; normal.height = 200; normal.border = 2;
        normal.dstX = 100; normal.dstY = 100;
        normal.parent = rootId;
        windows.insert(normal.id, normal);

        FakeWindow dock;
        dock.id = 0x102;
        dock.pid = 2002;
        dock.hasPidProp = true;
        dock.name = "Dock";
        dock.wmName = "Dock";
        dock.hasDesktopProp = true;
        dock.desktop = 0;
        dock.types << internAtom("_NET_WM_WINDOW_TYPE_DOCK");
        dock.width = 500; dock.height = 40;
        dock.dstX = 0; dock.dstY = 0;
        dock.parent = rootId;
        windows.insert(dock.id, dock);

        FakeWindow hidden;
        hidden.id = 0x103;
        hidden.pid = 2003;
        hidden.hasPidProp = true;
        hidden.name = "Hidden";
        hidden.wmName = "Hidden";
        hidden.hasDesktopProp = true;
        hidden.desktop = 0;
        hidden.types << internAtom("_NET_WM_WINDOW_TYPE_NORMAL");
        hidden.states << internAtom("_NET_WM_STATE_HIDDEN");
        hidden.width = 100; hidden.height = 100;
        hidden.parent = rootId;
        windows.insert(hidden.id, hidden);

        FakeWindow desktop;
        desktop.id = 0x104;
        desktop.pid = 2004;
        desktop.hasPidProp = true;
        desktop.name = "Desktop";
        desktop.wmName = "Desktop";
        desktop.hasDesktopProp = true;
        desktop.desktop = 0;
        desktop.types << internAtom("_NET_WM_WINDOW_TYPE_DESKTOP");
        desktop.width = 1024; desktop.height = 768;
        desktop.parent = rootId;
        windows.insert(desktop.id, desktop);

        FakeWindow inputonly;
        inputonly.id = 0x105;
        inputonly.pid = 2005;
        inputonly.hasPidProp = true;
        inputonly.name = "InputOnly";
        inputonly.wmName = "InputOnly";
        inputonly.hasDesktopProp = true;
        inputonly.desktop = 0;
        inputonly.types << internAtom("_NET_WM_WINDOW_TYPE_NORMAL");
        inputonly.wclass = XCB_WINDOW_CLASS_INPUT_ONLY;
        inputonly.width = 200; inputonly.height = 200;
        inputonly.parent = rootId;
        windows.insert(inputonly.id, inputonly);

        FakeWindow nopid;
        nopid.id = 0x106;
        nopid.pid = -1;
        nopid.hasPidProp = false;
        nopid.name = "NoPid";
        nopid.wmName = "NoPid";
        nopid.hasDesktopProp = true;
        nopid.desktop = 0;
        nopid.types << internAtom("_NET_WM_WINDOW_TYPE_NORMAL");
        nopid.width = 100; nopid.height = 100;
        nopid.parent = rootId;
        windows.insert(nopid.id, nopid);

        FakeWindow unmapped;
        unmapped.id = 0x107;
        unmapped.pid = 2007;
        unmapped.hasPidProp = true;
        unmapped.name = "Unmapped";
        unmapped.wmName = "Unmapped";
        unmapped.hasDesktopProp = true;
        unmapped.desktop = 0;
        unmapped.types << internAtom("_NET_WM_WINDOW_TYPE_NORMAL");
        unmapped.mapState = XCB_MAP_STATE_UNMAPPED;
        unmapped.width = 100; unmapped.height = 100;
        unmapped.parent = rootId;
        windows.insert(unmapped.id, unmapped);

        // X 返回序：底→顶
        const QVector<xcb_window_t> bottomUp = { 0x107, 0x106, 0x105, 0x104, 0x103, 0x102, 0x101 };
        for (FakeWindow &w : windows)
            w.childrenBottomUp.clear();
        FakeWindow &rootWin = windows[rootId];
        rootWin.id = rootId;
        rootWin.childrenBottomUp = bottomUp;
        clientListBottomUp = bottomUp;
        hasClientList = true;
        screenStorage[0].root = rootId;
    }

    // ---------- 构造 client list 世界（WMWindowList 用）----------
    // clients（X 返回序 底→顶）: 0x107(nopid-normal,unmapped) 0x105(ionly-normal)
    //                            0x102(dialog) 0x101(normal)；托盘窗口 0x201
    void buildClientWorld()
    {
        reset();
        FakeWindow unmapped;
        unmapped.id = 0x107;
        unmapped.pid = 3007;
        unmapped.hasPidProp = true;
        unmapped.name = "Hidden Client";
        unmapped.wmName = "Hidden Client";
        unmapped.hasDesktopProp = true;
        unmapped.desktop = 0;
        unmapped.types << internAtom("_NET_WM_WINDOW_TYPE_NORMAL");
        unmapped.mapState = XCB_MAP_STATE_UNMAPPED;
        windows.insert(unmapped.id, unmapped);

        FakeWindow ionly;
        ionly.id = 0x105;
        ionly.pid = 3005;
        ionly.hasPidProp = true;
        ionly.name = "Input Client";
        ionly.wmName = "Input Client";
        ionly.hasDesktopProp = true;
        ionly.desktop = 0;
        ionly.types << internAtom("_NET_WM_WINDOW_TYPE_NORMAL");
        ionly.wclass = XCB_WINDOW_CLASS_INPUT_ONLY;
        windows.insert(ionly.id, ionly);

        FakeWindow dialog;
        dialog.id = 0x102;
        dialog.pid = 3002;
        dialog.hasPidProp = true;
        dialog.name = "Calc";
        dialog.wmName = "Calc";
        dialog.hasDesktopProp = true;
        dialog.desktop = 0;
        dialog.types << internAtom("_NET_WM_WINDOW_TYPE_DIALOG");
        windows.insert(dialog.id, dialog);

        FakeWindow normal;
        normal.id = 0x101;
        normal.pid = 3001;
        normal.hasPidProp = true;
        normal.name = "Files";
        normal.wmName = "Files";
        normal.hasDesktopProp = true;
        normal.desktop = 0;
        normal.types << internAtom("_NET_WM_WINDOW_TYPE_NORMAL");
        windows.insert(normal.id, normal);

        FakeWindow tray;
        tray.id = 0x201;
        tray.pid = 3003;
        tray.hasPidProp = true;
        tray.name = "Tray App";
        tray.wmName = "Tray App";
        tray.hasDesktopProp = true;
        tray.desktop = 0;
        tray.types << internAtom("_NET_WM_WINDOW_TYPE_NORMAL");
        windows.insert(tray.id, tray);

        const QVector<xcb_window_t> bottomUp = { 0x107, 0x105, 0x102, 0x101 };
        const xcb_window_t rootId = root;
        FakeWindow &rootWin = windows[rootId];
        rootWin.id = rootId;
        rootWin.childrenBottomUp = bottomUp;
        clientListBottomUp = bottomUp;
        hasClientList = true;
        screenStorage[0].root = rootId;
    }
};

// ================= stub 函数（TU 局部静态） =================

static xcb_connection_t *stub_xcb_connect(const char *displayname, int *screenp)
{
    FakeX &w = FakeX::instance();
    w.connectCalls++;
    w.lastDisplayName = displayname ? QByteArray(displayname) : QByteArray();
    w.displayNames.append(w.lastDisplayName);
    if (screenp)
        *screenp = w.screenNumber;
    return w.conn;
}

static int stub_xcb_connection_has_error(xcb_connection_t *c)
{
    Q_UNUSED(c);
    return FakeX::instance().connectError;
}

static void stub_xcb_disconnect(xcb_connection_t *c)
{
    Q_UNUSED(c);
    FakeX::instance().disconnectCount++;
}

static int stub_xcb_flush(xcb_connection_t *c)
{
    Q_UNUSED(c);
    FakeX::instance().flushCount++;
    return 1;
}

static void stub_xcb_discard_reply(xcb_connection_t *c, unsigned int sequence)
{
    Q_UNUSED(c);
    Q_UNUSED(sequence);
}

static const xcb_setup_t *stub_xcb_get_setup(xcb_connection_t *c)
{
    Q_UNUSED(c);
    // 从不解引用：roots_iterator/roots_length 均已拦截
    return reinterpret_cast<const xcb_setup_t *>(&FakeX::instance().screenStorage[0]);
}

static xcb_screen_iterator_t stub_xcb_setup_roots_iterator(const xcb_setup_t *R)
{
    Q_UNUSED(R);
    FakeX &w = FakeX::instance();
    xcb_screen_iterator_t iter;
    iter.data = &w.screenStorage[0];
    iter.rem = w.screenCount > 0 ? w.screenCount : 0;
    iter.index = 0;
    return iter;
}

static int stub_xcb_setup_roots_length(const xcb_setup_t *R)
{
    Q_UNUSED(R);
    return FakeX::instance().screenCount;
}

static void stub_xcb_screen_next(xcb_screen_iterator_t *i)
{
    if (!i)
        return;
    FakeX &w = FakeX::instance();
    if (i->rem > 0) {
        ++(i->data);
        --(i->rem);
    }
    Q_UNUSED(w);
}

// ---------- intern_atom ----------
static xcb_intern_atom_cookie_t stub_xcb_intern_atom(xcb_connection_t *c,
                                                     uint8_t only_if_exists,
                                                     uint16_t len, const char *name)
{
    Q_UNUSED(c);
    Q_UNUSED(only_if_exists);
    FakeX &w = FakeX::instance();
    const unsigned seq = ++w.seqCounter;
    FakeX::SeqEntry &e = w.seqTable[seq];
    e.atom = w.internAtom(QByteArray(name, int(len)));
    xcb_intern_atom_cookie_t cookie;
    cookie.sequence = seq;
    return cookie;
}

static xcb_intern_atom_reply_t *stub_xcb_intern_atom_reply(xcb_connection_t *c,
                                                           xcb_intern_atom_cookie_t cookie,
                                                           xcb_generic_error_t **e)
{
    Q_UNUSED(c);
    Q_UNUSED(e);
    FakeX &w = FakeX::instance();
    if (w.failInternReply > 0) {
        w.failInternReply--;
        return nullptr;
    }
    xcb_intern_atom_reply_t *r
        = static_cast<xcb_intern_atom_reply_t *>(calloc(1, sizeof(xcb_intern_atom_reply_t)));
    r->response_type = XCB_INTERN_ATOM;
    r->sequence = uint16_t(cookie.sequence);
    r->atom = w.seqTable.value(cookie.sequence).atom;
    return r;
}

// ---------- get_property ----------
static FakeProp makeProperty(xcb_window_t win, xcb_atom_t atom)
{
    FakeX &w = FakeX::instance();
    FakeProp p;
    const QByteArray atomStr = w.atomName(atom);

    if (atomStr == "_NET_WM_PID") {
        const FakeWindow &fw = w.windows.value(win);
        if (!fw.hasPidProp) {
            p.type = XCB_ATOM_NONE;
            return p;
        }
        p.type = XCB_ATOM_CARDINAL;
        p.words << uint32_t(fw.pid);
        return p;
    }
    if (atomStr == "_NET_WM_NAME") {
        const FakeWindow &fw = w.windows.value(win);
        if (fw.name.isEmpty()) {
            p.type = XCB_ATOM_NONE;
            return p;
        }
        p.type = w.atomByName.value("UTF8_STRING", XCB_ATOM_STRING);
        p.format = 8;
        p.bytes = fw.name.toUtf8();
        return p;
    }
    if (atomStr == "_NET_WM_DESKTOP") {
        const FakeWindow &fw = w.windows.value(win);
        if (!fw.hasDesktopProp) {
            p.type = XCB_ATOM_NONE;
            return p;
        }
        p.type = XCB_ATOM_CARDINAL;
        p.words << uint32_t(fw.desktop);
        return p;
    }
    if (atomStr == "_NET_WM_WINDOW_TYPE") {
        const FakeWindow &fw = w.windows.value(win);
        p.type = XCB_ATOM_ATOM;
        p.words = fw.types;
        return p;
    }
    if (atomStr == "_NET_WM_STATE") {
        const FakeWindow &fw = w.windows.value(win);
        p.type = XCB_ATOM_ATOM;
        p.words = fw.states;
        return p;
    }
    if (atomStr == "_NET_FRAME_EXTENTS") {
        const FakeWindow &fw = w.windows.value(win);
        p.type = XCB_ATOM_CARDINAL;
        for (int i = 0; i < 4; ++i)
            p.words << fw.extents[i];
        return p;
    }
    if (atomStr == "_NET_WM_ICON") {
        const FakeWindow &fw = w.windows.value(win);
        p.type = XCB_ATOM_CARDINAL;
        p.words = fw.iconData;
        return p;
    }
    if (atomStr == "_NET_CLIENT_LIST_STACKING") {
        if (!w.hasClientList) {
            p.type = XCB_ATOM_NONE;
            return p;
        }
        p.type = XCB_ATOM_WINDOW;
        p.words = w.clientListBottomUp;
        return p;
    }
    // 未知属性：type = NONE（各调用方据此走默认分支）
    p.type = XCB_ATOM_NONE;
    return p;
}

static xcb_get_property_cookie_t stub_xcb_get_property(xcb_connection_t *c, uint8_t _delete,
                                                       xcb_window_t window, xcb_atom_t property,
                                                       xcb_atom_t type, uint32_t long_offset,
                                                       uint32_t long_length)
{
    Q_UNUSED(c);
    Q_UNUSED(_delete);
    Q_UNUSED(type);
    Q_UNUSED(long_offset);
    Q_UNUSED(long_length);
    FakeX &w = FakeX::instance();
    const unsigned seq = ++w.seqCounter;
    FakeX::SeqEntry &e = w.seqTable[seq];
    e.win = window;
    e.atom = property;
    e.prop = makeProperty(window, property);
    xcb_get_property_cookie_t cookie;
    cookie.sequence = seq;
    return cookie;
}

static xcb_get_property_reply_t *stub_xcb_get_property_reply(xcb_connection_t *c,
                                                             xcb_get_property_cookie_t cookie,
                                                             xcb_generic_error_t **e)
{
    Q_UNUSED(c);
    Q_UNUSED(e);
    FakeX &w = FakeX::instance();
    if (w.failPropertyReply > 0) {
        w.failPropertyReply--;
        return nullptr;
    }
    const FakeProp &p = w.seqTable.value(cookie.sequence).prop;
    const int valueBytes = p.format == 8 ? p.bytes.size() : int(p.words.size()) * 4;
    xcb_get_property_reply_t *r = static_cast<xcb_get_property_reply_t *>(
        calloc(1, sizeof(xcb_get_property_reply_t) + size_t(valueBytes) + 8));
    if (getenv("FAKEXCB_DEBUG") && p.format == 8)
        fprintf(stderr, "[FAKEXCB] get_property_reply seq=%u type=%u fmt=%u bytes=[%s]\n",
                cookie.sequence, p.type, p.format, p.bytes.left(20).constData());
    r->response_type = XCB_GET_PROPERTY;
    r->format = p.format;
    r->sequence = uint16_t(cookie.sequence);
    r->type = p.type;
    r->value_len = p.format == 8 ? uint32_t(p.bytes.size()) : uint32_t(p.words.size());
    if (p.format == 8 && !p.bytes.isEmpty())
        memcpy(reinterpret_cast<uint8_t *>(r) + sizeof(xcb_get_property_reply_t),
               p.bytes.constData(), size_t(p.bytes.size()));
    else if (p.format == 32 && !p.words.isEmpty())
        memcpy(reinterpret_cast<uint8_t *>(r) + sizeof(xcb_get_property_reply_t),
               p.words.constData(), size_t(p.words.size()) * 4);
    return r;
}

// ---------- icccm wm_name ----------
static xcb_get_property_cookie_t stub_xcb_icccm_get_wm_name(xcb_connection_t *c,
                                                            xcb_window_t window)
{
    Q_UNUSED(c);
    FakeX &w = FakeX::instance();
    const unsigned seq = ++w.seqCounter;
    FakeX::SeqEntry &e = w.seqTable[seq];
    e.win = window;
    const FakeWindow &fw = w.windows.value(window);
    FakeProp &p = e.prop;
    if (fw.wmName.isEmpty()) {
        p.type = XCB_ATOM_NONE;
    } else {
        p.type = XCB_ATOM_STRING;
        p.format = 8;
        p.bytes = fw.wmName.toLocal8Bit();
    }
    ++w.icccmNameRequests;
    xcb_get_property_cookie_t cookie;
    cookie.sequence = seq;
    return cookie;
}

static xcb_get_property_reply_t *stub_xcb_icccm_get_wm_name_reply(xcb_connection_t *c,
                                                                  xcb_get_property_cookie_t cookie,
                                                                  xcb_generic_error_t **e)
{
    Q_UNUSED(c);
    Q_UNUSED(e);
    FakeX &w = FakeX::instance();
    if (w.failIcccmNameReply > 0) {
        w.failIcccmNameReply--;
        return nullptr;
    }
    const FakeProp &p = w.seqTable.value(cookie.sequence).prop;
    xcb_get_property_reply_t *r = static_cast<xcb_get_property_reply_t *>(
        calloc(1, sizeof(xcb_get_property_reply_t) + size_t(p.bytes.size()) + 8));
    r->response_type = XCB_GET_PROPERTY;
    r->format = p.format;
    r->sequence = uint16_t(cookie.sequence);
    r->type = p.type;
    r->value_len = uint32_t(p.bytes.size());
    if (!p.bytes.isEmpty())
        memcpy(reinterpret_cast<uint8_t *>(r) + sizeof(xcb_get_property_reply_t),
               p.bytes.constData(), size_t(p.bytes.size()));
    return r;
}

// ---------- geometry / attributes / coordinates ----------
static xcb_get_geometry_cookie_t stub_xcb_get_geometry(xcb_connection_t *c, xcb_window_t window)
{
    Q_UNUSED(c);
    FakeX &w = FakeX::instance();
    const unsigned seq = ++w.seqCounter;
    FakeX::SeqEntry &e = w.seqTable[seq];
    e.win = window;
    xcb_get_geometry_cookie_t cookie;
    cookie.sequence = seq;
    return cookie;
}

static xcb_get_geometry_reply_t *stub_xcb_get_geometry_reply(xcb_connection_t *c,
                                                             xcb_get_geometry_cookie_t cookie,
                                                             xcb_generic_error_t **e)
{
    Q_UNUSED(c);
    Q_UNUSED(e);
    FakeX &w = FakeX::instance();
    if (w.failGeometryReply > 0) {
        w.failGeometryReply--;
        return nullptr;
    }
    const FakeWindow &fw = w.windows.value(w.seqTable.value(cookie.sequence).win);
    xcb_get_geometry_reply_t *r
        = static_cast<xcb_get_geometry_reply_t *>(calloc(1, sizeof(xcb_get_geometry_reply_t)));
    r->response_type = XCB_GET_GEOMETRY;
    r->sequence = uint16_t(cookie.sequence);
    r->root = w.root;
    r->x = fw.x;
    r->y = fw.y;
    r->width = fw.width;
    r->height = fw.height;
    r->border_width = fw.border;
    return r;
}

static xcb_get_window_attributes_cookie_t stub_xcb_get_window_attributes(xcb_connection_t *c,
                                                                         xcb_window_t window)
{
    Q_UNUSED(c);
    FakeX &w = FakeX::instance();
    const unsigned seq = ++w.seqCounter;
    FakeX::SeqEntry &e = w.seqTable[seq];
    e.win = window;
    xcb_get_window_attributes_cookie_t cookie;
    cookie.sequence = seq;
    return cookie;
}

static xcb_get_window_attributes_reply_t *stub_xcb_get_window_attributes_reply(
    xcb_connection_t *c, xcb_get_window_attributes_cookie_t cookie, xcb_generic_error_t **e)
{
    Q_UNUSED(c);
    Q_UNUSED(e);
    FakeX &w = FakeX::instance();
    if (w.failAttrReply > 0) {
        w.failAttrReply--;
        return nullptr;
    }
    const FakeWindow &fw = w.windows.value(w.seqTable.value(cookie.sequence).win);
    xcb_get_window_attributes_reply_t *r = static_cast<xcb_get_window_attributes_reply_t *>(
        calloc(1, sizeof(xcb_get_window_attributes_reply_t)));
    r->response_type = XCB_GET_WINDOW_ATTRIBUTES;
    r->sequence = uint16_t(cookie.sequence);
    r->_class = uint16_t(fw.wclass);
    r->map_state = uint8_t(fw.mapState);
    return r;
}

static xcb_translate_coordinates_cookie_t stub_xcb_translate_coordinates(
    xcb_connection_t *c, xcb_window_t src_window, xcb_window_t dst_window, int16_t src_x,
    int16_t src_y)
{
    Q_UNUSED(c);
    Q_UNUSED(dst_window);
    Q_UNUSED(src_x);
    Q_UNUSED(src_y);
    FakeX &w = FakeX::instance();
    const unsigned seq = ++w.seqCounter;
    FakeX::SeqEntry &e = w.seqTable[seq];
    e.win = src_window;
    xcb_translate_coordinates_cookie_t cookie;
    cookie.sequence = seq;
    return cookie;
}

static xcb_translate_coordinates_reply_t *stub_xcb_translate_coordinates_reply(
    xcb_connection_t *c, xcb_translate_coordinates_cookie_t cookie, xcb_generic_error_t **e)
{
    Q_UNUSED(c);
    Q_UNUSED(e);
    FakeX &w = FakeX::instance();
    if (w.failCoordsReply > 0) {
        w.failCoordsReply--;
        return nullptr;
    }
    const xcb_window_t seqWin = w.seqTable.value(cookie.sequence).win;
    if (w.failCoordsReplyFor.value(seqWin, 0) > 0) {
        w.failCoordsReplyFor[seqWin]--;
        return nullptr;
    }
    const FakeWindow &fw = w.windows.value(seqWin);
    xcb_translate_coordinates_reply_t *r = static_cast<xcb_translate_coordinates_reply_t *>(
        calloc(1, sizeof(xcb_translate_coordinates_reply_t)));
    r->response_type = XCB_TRANSLATE_COORDINATES;
    r->sequence = uint16_t(cookie.sequence);
    r->dst_x = fw.dstX;
    r->dst_y = fw.dstY;
    return r;
}

// ---------- query_tree ----------
static xcb_query_tree_cookie_t stub_xcb_query_tree(xcb_connection_t *c, xcb_window_t window)
{
    Q_UNUSED(c);
    FakeX &w = FakeX::instance();
    const unsigned seq = ++w.seqCounter;
    FakeX::SeqEntry &e = w.seqTable[seq];
    e.win = window;
    xcb_query_tree_cookie_t cookie;
    cookie.sequence = seq;
    return cookie;
}

static xcb_query_tree_reply_t *stub_xcb_query_tree_reply(xcb_connection_t *c,
                                                         xcb_query_tree_cookie_t cookie,
                                                         xcb_generic_error_t **e)
{
    Q_UNUSED(c);
    Q_UNUSED(e);
    FakeX &w = FakeX::instance();
    if (w.failTreeReply > 0) {
        w.failTreeReply--;
        return nullptr;
    }
    const FakeWindow &fw = w.windows.value(w.seqTable.value(cookie.sequence).win);
    const int n = fw.childrenBottomUp.size();
    xcb_query_tree_reply_t *r = static_cast<xcb_query_tree_reply_t *>(
        calloc(1, sizeof(xcb_query_tree_reply_t) + size_t(n) * sizeof(xcb_window_t) + 8));
    r->response_type = XCB_QUERY_TREE;
    r->sequence = uint16_t(cookie.sequence);
    r->root = w.root;
    r->parent = fw.parent;
    r->children_len = uint16_t(n);
    if (n > 0)
        memcpy(reinterpret_cast<uint8_t *>(r) + sizeof(xcb_query_tree_reply_t),
               fw.childrenBottomUp.constData(), size_t(n) * sizeof(xcb_window_t));
    return r;
}

// ---------- get_atom_name ----------
static xcb_get_atom_name_cookie_t stub_xcb_get_atom_name(xcb_connection_t *c, xcb_atom_t atom)
{
    Q_UNUSED(c);
    FakeX &w = FakeX::instance();
    w.getAtomNameCalls++;
    const unsigned seq = ++w.seqCounter;
    FakeX::SeqEntry &e = w.seqTable[seq];
    e.atom = atom;
    xcb_get_atom_name_cookie_t cookie;
    cookie.sequence = seq;
    return cookie;
}

static xcb_get_atom_name_reply_t *stub_xcb_get_atom_name_reply(xcb_connection_t *c,
                                                               xcb_get_atom_name_cookie_t cookie,
                                                               xcb_generic_error_t **e)
{
    Q_UNUSED(c);
    Q_UNUSED(e);
    FakeX &w = FakeX::instance();
    if (w.failAtomNameReply > 0) {
        w.failAtomNameReply--;
        return nullptr;
    }
    const QByteArray name = w.atomName(w.seqTable.value(cookie.sequence).atom);
    xcb_get_atom_name_reply_t *r = static_cast<xcb_get_atom_name_reply_t *>(
        calloc(1, sizeof(xcb_get_atom_name_reply_t) + size_t(name.size()) + 8));
    r->response_type = XCB_GET_ATOM_NAME;
    r->sequence = uint16_t(cookie.sequence);
    r->name_len = uint16_t(name.size());
    if (!name.isEmpty())
        memcpy(reinterpret_cast<uint8_t *>(r) + sizeof(xcb_get_atom_name_reply_t),
               name.constData(), size_t(name.size()));
    return r;
}

}   // namespace ut_wm

#endif   // UT_WM_FAKEXCB_H
