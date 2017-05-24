#ifndef NETWORKTRAFFICFILTER_H
#define NETWORKTRAFFICFILTER_H

#include "libnethogs.h"
#include <mutex>
#include <map>

class NetworkTrafficFilter final
{
	NetworkTrafficFilter() = delete;
	
public:
	struct Update
	{
		int action;
		NethogsMonitorRecord record;
	};
	
	static void setRowUpdate(int action, NethogsMonitorRecord const& record);
	static void setNetHogsMonitorStatus(int status);

	static int  getNetHogsMonitorStatus();
	static bool getRowUpdate(Update& update);
    
private:
	typedef std::map<int, Update> RowUpdatesMap;
	
	static std::mutex m_mutex;
	static RowUpdatesMap m_row_updates_map;
	static int m_nethogs_monitor_status;
};

#endif // NETWORKTRAFFICFILTER_H
