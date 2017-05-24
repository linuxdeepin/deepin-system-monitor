#include "network_traffic_filter.h"
#include <string.h>

std::mutex NetworkTrafficFilter::m_mutex;
NetworkTrafficFilter::RowUpdatesMap NetworkTrafficFilter::m_row_updates_map;
int NetworkTrafficFilter::m_nethogs_monitor_status = NETHOGS_STATUS_OK;

void NetworkTrafficFilter::setRowUpdate(int action, NethogsMonitorRecord const& record)
{
	if( action == NETHOGS_APP_ACTION_REMOVE || record.sent_bytes || record.recv_bytes )
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

bool NetworkTrafficFilter::getRowUpdate(NetworkTrafficFilter::Update& update)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	if( m_row_updates_map.empty() )
		return false;
	update = m_row_updates_map.begin()->second;
	m_row_updates_map.erase(m_row_updates_map.begin());
	return true;
}

void NetworkTrafficFilter::setNetHogsMonitorStatus(int status) 
{ 
	std::lock_guard<std::mutex> lock(m_mutex);
	m_nethogs_monitor_status = status; 
}

int NetworkTrafficFilter::getNetHogsMonitorStatus()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_nethogs_monitor_status;
}

