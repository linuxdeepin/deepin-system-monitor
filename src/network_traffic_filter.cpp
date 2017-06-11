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

#include "network_traffic_filter.h"
#include <string.h>

std::mutex NetworkTrafficFilter::m_mutex;
NetworkTrafficFilter::RowUpdatesMap NetworkTrafficFilter::m_row_updates_map;
int NetworkTrafficFilter::m_nethogs_monitor_status = NETHOGS_STATUS_OK;

bool NetworkTrafficFilter::getRowUpdate(NetworkTrafficFilter::Update& update)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	if (m_row_updates_map.empty())
		return false;
	update = m_row_updates_map.begin()->second;
	m_row_updates_map.erase(m_row_updates_map.begin());
	return true;
}

int NetworkTrafficFilter::getNetHogsMonitorStatus()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_nethogs_monitor_status;
}

void NetworkTrafficFilter::setNetHogsMonitorStatus(int status) 
{ 
	std::lock_guard<std::mutex> lock(m_mutex);
	m_nethogs_monitor_status = status; 
}

void NetworkTrafficFilter::setRowUpdate(int action, NethogsMonitorRecord const& record)
{
	if (action == NETHOGS_APP_ACTION_REMOVE || record.sent_bytes || record.recv_bytes)
	{
		//save the update for GUI use
		std::lock_guard<std::mutex> lock(m_mutex);
		Update update;
		memset(&update, 0, sizeof(update));
		update.action = action;
		update.record = record;
		m_row_updates_map[record.record_id] = update;
	}	
}

void NetworkTrafficFilter::nethogsMonitorThreadProc()
{
	const int status = nethogsmonitor_loop(&onNethogsUpdate);
	NetworkTrafficFilter::setNetHogsMonitorStatus(status);
}

void NetworkTrafficFilter::onNethogsUpdate(int action, NethogsMonitorRecord const* update)
{
	NetworkTrafficFilter::setRowUpdate(action, *update);
}

