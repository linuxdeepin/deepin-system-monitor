// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BASE_STYLE_HELPER_H
#define BASE_STYLE_HELPER_H

#include <DApplication>
#include <DStyle>

DWIDGET_USE_NAMESPACE

static inline DStyle *getDStyle()
{
    return dynamic_cast<DStyle *>(DApplication::style());
}

#endif // BASE_STYLE_HELPER_H
