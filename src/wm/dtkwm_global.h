/**
 * Copyright (C) 2017 Deepin Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 **/

#pragma once

#include <dtkcore_global.h>

#define DWM_NAMESPACE Wm
#define DTK_WM_NAMESPACE DTK_NAMESPACE::Wm

#   define DWM_BEGIN_NAMESPACE namespace DTK_NAMESPACE { namespace DWM_NAMESPACE {
#   define DWM_END_NAMESPACE }}
#   define DWM_USE_NAMESPACE using namespace DTK_WM_NAMESPACE;

