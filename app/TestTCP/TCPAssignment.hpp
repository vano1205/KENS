/*
 * E_TCPAssignment.hpp
 *
 *  Created on: 2014. 11. 20.
 *      Author: Keunhong Lee
 */

#ifndef E_TCPASSIGNMENT_HPP_
#define E_TCPASSIGNMENT_HPP_


#include <E/Networking/E_Networking.hpp>
#include <E/Networking/E_Host.hpp>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <netinet/in.h>


#include <E/E_TimerModule.hpp>

#include <map>

namespace E
{

class SockContext 
{
public:
	int state;
	int seq;
	int srcIP, srcPort, desIP,desPort;

public:
	SockContext(){
		desIP=-1;
		desPort=-1;
	}
};
class TCPAssignment : public HostModule, public NetworkModule, public SystemCallInterface, private NetworkLog, private TimerModule
{
private:
	std::map<int, SockContext> addrfdlist;
private:
	virtual void timerCallback(void* payload) final;

public:
	TCPAssignment(Host* host);
	virtual void initialize();
	virtual void finalize();
	virtual ~TCPAssignment();

	int syscall_socket(UUID syscallUUID, int pid, int type, int protocol);
	int syscall_close(UUID syscallUUID, int pid, int fd);
	int syscall_bind(UUID syscallUUID, int pid, int sockfd, struct sockaddr *my_addr, socklen_t addrlen); 
	int syscall_getsockname(UUID syscallUUID, int pid, int sockfd, struct sockaddr *addr, socklen_t *addrlen);
protected:
	virtual void systemCallback(UUID syscallUUID, int pid, const SystemCallParameter& param) final;
	virtual void packetArrived(std::string fromModule, Packet* packet) final;
};

class TCPAssignmentProvider
{
private:
	TCPAssignmentProvider() {}
	~TCPAssignmentProvider() {}
public:
	static HostModule* allocate(Host* host) { return new TCPAssignment(host); }
};

}


#endif /* E_TCPASSIGNMENT_HPP_ */
