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


namespace E
{

class SockContext 
{
public:
	int state;
	int srcIP, srcPort, desIP,desPort;
	int seqnum,acknum;
	UUID syscallID;
	int backlog;
	struct sockaddr *addrinfo;
	int pid;
	std::list<int> dupsocklist;
	std::list<int> backloglist;
	bool acceptflag;

public:
	SockContext(){
		srcIP=-1;
		srcPort=-1;
		syscallID=-1;
		pid=-1;
		seqnum=0;
		acknum=0;
	}
};

class Header
{
public: 
	uint16_t srcPort;
	uint16_t desPort;
	uint32_t seqnum;
	uint32_t acknum;
	uint8_t len;
	uint8_t flags;
	uint16_t window;
	uint16_t checksum;
	uint16_t urg_ptr;

public:
	Header(){
		flags=0;
		window=htons(51200);
	}
};

class TCPAssignment : public HostModule, public NetworkModule, public SystemCallInterface, private NetworkLog, private TimerModule
{
private:
	std::multimap<int, SockContext> addrfdlist;
	std::multimap<int, SockContext> closelist;
	#define CLOSED 0
	#define LISTENS 1
	#define SYNSENT 2
	#define SYNRCVD 3
	#define ESTAB 4
	#define FIN_WAIT1 5
	#define FIN_WAIT2 6
	#define CLOSE_WAIT 7
	#define TIMED_WAIT 8
	#define LAST_ACK 9

	#define URG 32
	#define ACK 16
	#define PSH 8
	#define RST 4
	#define SYN 2
	#define FIN 1

private:
	virtual void timerCallback(void* payload) final;

public:
	TCPAssignment(Host* host);
	virtual void initialize();
	virtual void finalize();
	virtual ~TCPAssignment();

	std::multimap<int, SockContext>::iterator mapfindbypid(int pid, int fd);
	void sendTCPPacket(Packet *newPacket,Header *tcpHeader, uint32_t desIP, uint32_t srcIP, uint16_t desPort, uint16_t srcPort, uint32_t seqnum, uint32_t acknum, uint8_t flags);
	void syscall_socket(UUID syscallUUID, int pid, int type, int protocol);
	void syscall_close(UUID syscallUUID, int pid, int fd);
	void syscall_bind(UUID syscallUUID, int pid, int sockfd, struct sockaddr *my_addr, socklen_t addrlen); 
	void syscall_getsockname(UUID syscallUUID, int pid, int sockfd, struct sockaddr *addr, socklen_t *addrlen);
	void syscall_connect(UUID syscallUUID, int pid, int sockfd, struct sockaddr *addr, socklen_t addrlen);
	void syscall_getpeername(UUID syscallUUID, int pid, int sockfd, struct sockaddr *addr, socklen_t *addrlen);
	void syscall_listen(UUID syscallUUID, int pid, int sockfd, int backlog);
	void syscall_accept(UUID syscallUUID, int pid, int sockfd, struct sockaddr *addr, socklen_t *addrlen);

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
