// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// 兼容转发头：统一到 3rdparty/stub-ext/stub.h 的唯一 Stub 定义，
// 避免新旧两份 stub_ext::Stub 类定义共存引发 ODR 冲突（vtable 布局不一致导致崩溃）。
#pragma once
#include "3rdparty/stub-ext/stub.h"
