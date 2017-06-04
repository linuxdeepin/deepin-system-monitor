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
