#ifndef __MODBUS_SERVER_H__
#define __MODBUS_SERVER_H__

#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <cmath>
#include <mutex>
#include <atomic>
#include <signal.h>

#ifdef WIN32
#include <winsock2.h>
#include <modbus/modbus.h>
#else
#include <errno.h>
#include <modbus/modbus.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
extern "C"
{
#include <sys/un.h>
}
#endif

namespace ModbusTCP {

class Server
{
public:
    Server();
    ~Server();

	// Initialize the modbus context
	bool configure(int regsize, int port = 502);
	bool listenAndAccept();

	int serveForever(bool notify_systemd = false);

	// Clear everything
	void finalize();

private:

	// Modbus 
	int m_port;
	modbus_t *m_ctx;
    int m_regsize;
	modbus_mapping_t *m_mapping;
	uint8_t m_queries[MODBUS_TCP_MAX_ADU_LENGTH];
	int m_headerLength;

	int m_server;
	fd_set m_refset, m_rdset;
	int m_fdmax;
};

}

#endif // __MODBUS_SERVER_H__
