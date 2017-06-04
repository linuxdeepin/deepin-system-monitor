/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2017 Deepin, Inc.
 *               2011 ~ 2017 Wang Yong
 *
 * Author:     Wang Yong <wangyong@deepin.com>
 * Maintainer: Wang Yong <wangyong@deepin.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */ 

#ifndef NETWORKTRAFFICFILTER_H
#define NETWORKTRAFFICFILTER_H

#include "libnethogs.h"
#include <map>
#include <mutex>

class NetworkTrafficFilter final
{
public:
	NetworkTrafficFilter() = delete;
    
	struct Update
	{
		int action;
		NethogsMonitorRecord record;
	};
	
	static bool getRowUpdate(Update& update);
	static int  getNetHogsMonitorStatus();
	static void setNetHogsMonitorStatus(int status);
	static void setRowUpdate(int action, NethogsMonitorRecord const& record);
    static void nethogsMonitorThreadProc();
    static void onNethogsUpdate(int action, NethogsMonitorRecord const* update);
    
private:
	typedef std::map<int, Update> RowUpdatesMap;
	static RowUpdatesMap m_row_updates_map;
	static int m_nethogs_monitor_status;
	static std::mutex m_mutex;
};

#endif // NETWORKTRAFFICFILTER_H
