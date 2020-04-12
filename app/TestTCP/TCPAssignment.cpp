/*
 * E_TCPAssignment.cpp
 *
 *  Created on: 2014. 11. 20.
 *      Author: Keunhong Lee
 */


#include <E/E_Common.hpp>
#include <E/Networking/E_Host.hpp>
#include <E/Networking/E_Networking.hpp>
#include <cerrno>
#include <E/Networking/E_Packet.hpp>
#include <E/Networking/E_NetworkUtil.hpp>
#include "TCPAssignment.hpp"

namespace E
{

TCPAssignment::TCPAssignment(Host* host) : HostModule("TCP", host),
		NetworkModule(this->getHostModuleName(), host->getNetworkSystem()),
		SystemCallInterface(AF_INET, IPPROTO_TCP, host),
		NetworkLog(host->getNetworkSystem()),
		TimerModule(host->getSystem())
{

}

TCPAssignment::~TCPAssignment()
{

}

void TCPAssignment::initialize()
{

}

void TCPAssignment::finalize()
{

}

void TCPAssignment::systemCallback(UUID syscallUUID, int pid, const SystemCallParameter& param)
{
	switch(param.syscallNumber)
	{
	case SOCKET:{
		this->syscall_socket(syscallUUID, pid, param.param1_int, param.param2_int);
		break;
		}
	case CLOSE:
		this->syscall_close(syscallUUID, pid, param.param1_int);
		break;
	case READ:
		//this->syscall_read(syscallUUID, pid, param.param1_int, param.param2_ptr, param.param3_int);
		break;
	case WRITE:
		//this->syscall_write(syscallUUID, pid, param.param1_int, param.param2_ptr, param.param3_int);
		break;
	case CONNECT:
		this->syscall_connect(syscallUUID, pid, param.param1_int,
				static_cast<struct sockaddr*>(param.param2_ptr), (socklen_t)param.param3_int);
		break;
	case LISTEN:
		this->syscall_listen(syscallUUID, pid, param.param1_int, param.param2_int);
		break;
	case ACCEPT:
		this->syscall_accept(syscallUUID, pid, param.param1_int,
				static_cast<struct sockaddr*>(param.param2_ptr),
				static_cast<socklen_t*>(param.param3_ptr));
		break;
	case BIND:
		this->syscall_bind(syscallUUID, pid, param.param1_int,
				static_cast<struct sockaddr *>(param.param2_ptr),
				(socklen_t) param.param3_int);
		break;
	case GETSOCKNAME:
		this->syscall_getsockname(syscallUUID, pid, param.param1_int,
			static_cast<struct sockaddr *>(param.param2_ptr),
			static_cast<socklen_t*>(param.param3_ptr));
		break;
	case GETPEERNAME:
		this->syscall_getpeername(syscallUUID, pid, param.param1_int,
				static_cast<struct sockaddr *>(param.param2_ptr),
		    	static_cast<socklen_t*>(param.param3_ptr));
		break;
	default:
		assert(0);
	}
}

std::map<int, SockContext>::iterator TCPAssignment::mapfindbypid(int pid, int fd){
	auto it=addrfdlist.begin();
	//printf("map find called by pid %d, fd %d\n",pid,fd);
	//printf("size is %d\n",addrfdlist.size());
	while(it!=addrfdlist.end()){
		int fdcmp = it->first;
		int pidcmp = it->second.pid;
			//printf("pidcmp is %d\n",pidcmp);
		if(fd==fdcmp&&pid==pidcmp){
			return it;
		}		
		it++;
	}
	printf("not found!\n");
}

void TCPAssignment::syscall_socket(UUID syscallUUID, int pid, int domain, int type){
	if(domain!=PF_INET||type!=SOCK_STREAM)
		this->returnSystemCall(syscallUUID,-1);
	int fd=this->createFileDescriptor(pid);
	SockContext *context= new SockContext();
	context->pid=pid;
	//printf("socket start pid %d, fd is %d\n",pid,fd);

	

	// SockKey *sockkey = new SockKey();
	// sockkey->pid=-1;
	// sockkey->fd=fd;

	addrfdlist.insert(std::pair<int,SockContext>(fd,*context));
	// auto it=addrfdlist.begin();
	// while(it!=addrfdlist.end()){
	// 	printf("addfdlist entry pid is %d, fd is %d\n",it->second.pid,it->first);	
	// 	it++;
	// }
	//auto entry= mapfindbypid(pid,fd);
	//entry->second.pid=pid;
	//printf(" for check in socket is %d\n",entry->second.pid);
	//printf("sie in socket is %d\n",addrfdlist.size());
	this->returnSystemCall(syscallUUID,fd);
}

void TCPAssignment::syscall_close(UUID syscallUUID, int pid, int fd){
	//printf("close fd %d\n",fd);
	addrfdlist.erase(mapfindbypid(pid,fd));
	this->removeFileDescriptor(pid,fd);
	this->returnSystemCall(syscallUUID,0);
}

void TCPAssignment:: syscall_connect(UUID syscallUUID, int pid, int sockfd, struct sockaddr *addr, socklen_t addrlen){
	const struct sockaddr_in *addr_int = (const struct sockaddr_in *)addr;
	unsigned short int desPort= ntohs(addr_int->sin_port);
	uint8_t desIP[4], srcIP[4];
	uint32_t desIP32, srcIP32;
	int srcPort;
	int tableidx;

	desIP32= ntohl((addr_int->sin_addr).s_addr);
	desIP[0]=desIP32>>24;
	desIP[1]=(u_char)(desIP32>>16);
	desIP[2]=(u_char)(desIP32>>8);
	desIP[3]=(u_char)desIP32;

	SockContext *context= &mapfindbypid(pid,sockfd)->second;
	context->desIP=desIP32;
	context->desPort=desPort;

	tableidx= this->getHost()->getRoutingTable(desIP);

	if(context->srcIP==-1&&context->srcPort==-1){
		this->getHost()->getIPAddr(srcIP,tableidx);

		srcIP32=srcIP[0]<<24|srcIP[1]<<16|srcIP[2]<<8|srcIP[3];
		// Dynamic and/or Private Port : 49152 ~ 65535
		while(1){
			int tempport=rand()%64510+1024;
			auto it=addrfdlist.begin();
			while(it!=addrfdlist.end()){
				int cmpport = it->second.srcPort;
				if(tempport==cmpport)
					break;
				it++;
			}
			srcPort=tempport;
			break;
		}
	}
	else{
		srcIP32=context->srcIP;
		srcPort=context->srcPort;
	}
	context->srcIP=srcIP32;
	context->srcPort=srcPort;

//	entry->second.synbit=1;

	srcIP32=htonl(srcIP32);
	desIP32=htonl(desIP32);

	Packet* newPacket= this->allocatePacket(54);
	newPacket->writeData(14+12,&srcIP32,4);
	newPacket->writeData(14+16,&desIP32,4);

	Header *tcpHeader = new Header();
	tcpHeader->srcPort=htons(srcPort);
	tcpHeader->desPort=htons(desPort);
	tcpHeader->seqnum=0;
	tcpHeader->acknum=0;
	tcpHeader->len=(5<<4);
	tcpHeader->flags=SYN;
	tcpHeader->urg_ptr=0;
	tcpHeader->checksum=0;
	tcpHeader->checksum = htons(~(NetworkUtil::tcp_sum(srcIP32, desIP32, (uint8_t*)tcpHeader, 20)));

	newPacket->writeData(30+4,tcpHeader,20);
	context->state=SYNSENT;
	context->syscallID=syscallUUID;

	this->sendPacket("IPv4",newPacket);

	uint32_t src, des;

	newPacket->readData(14+12,&src,4);
	newPacket->readData(14+16,&des,4);
}

void TCPAssignment::syscall_listen(UUID syscallUUID, int pid, int sockfd, int backlog){
	printf("pid and fd in listen start is %d %d\n",pid,sockfd);

	SockContext *context= &mapfindbypid(pid,sockfd)->second;
	//not bound connection
	if(context->srcIP==-1||context->srcPort==-1){
		this->returnSystemCall(syscallUUID,-1);
	}
	
	context->backlog=backlog;
	context->state=LISTENS;
	this->returnSystemCall(syscallUUID,0);
}

void TCPAssignment::syscall_accept(UUID syscallUUID, int pid, int sockfd, struct sockaddr *addr, socklen_t *addrlen){
	printf("pid in accept start is %d\n",pid);
	int dupsockfd;
	SockContext *sockcontext=&mapfindbypid(pid,sockfd)->second;

	std::list<int> *dupsocklist=&(sockcontext->dupsocklist);

	//not bound connection
	if(sockcontext->srcIP==-1||sockcontext->srcPort==-1){
		this->returnSystemCall(syscallUUID,-1);
	}

	sockcontext->syscallID=syscallUUID;
	//printf("syscall is %d\n",syscallUUID);
	sockcontext->addrinfo=addr;

	if(dupsocklist->size()>0){
		dupsockfd= dupsocklist->front();
		dupsocklist->pop_front();

		SockContext *dupcontext= &mapfindbypid(pid,dupsockfd)->second;

		if(dupcontext->state==ESTAB){
			printf("estabbbbb!!!\n");

			struct sockaddr_in *ret=(struct sockaddr_in *)addr;	
			memset(ret,0,sizeof(ret));
		
			ret->sin_family = AF_INET;
			ret->sin_addr.s_addr=htonl(dupcontext->srcIP);
			ret->sin_port=htons(dupcontext->srcPort);
			this->returnSystemCall(syscallUUID,dupsockfd);
		}

	}
}

void TCPAssignment::syscall_bind(UUID syscallUUID, int pid, int sockfd, struct sockaddr *addr, socklen_t addrlen){
	auto itz=addrfdlist.begin();
	while(itz!=addrfdlist.end()){
		printf("addfdlist entry pid in bind is %d, fd is %d\n",itz->second.pid,itz->first);	
		itz++;
	}
	
	
	struct sockaddr_in *addr_int = (struct sockaddr_in *)addr;
	unsigned short int portnum= ntohs(addr_int->sin_port);
	uint32_t ipaddr=ntohl((addr_int->sin_addr).s_addr);
	auto it=addrfdlist.begin();
	printf("bind called by pid %d, sockfd %d\n",pid,sockfd);

	while(it!=addrfdlist.end()){
		int fdcmp = it->first;
		int ipcmp = it->second.srcIP;
		int portcmp = it->second.srcPort;
		int pidcmp= it->second.pid;
		//if(pid!=pidcmp){
			// SockContext *context= &mapfindbypid(pid,sockfd)->second;
			// context->srcIP=ipaddr;
			// context->srcPort=portnum;
			
		// }
		if(pid==pidcmp){
			if(portnum==portcmp){
				if(ipaddr==ipcmp)
					this->returnSystemCall(syscallUUID,-1);
				else if(ipaddr==INADDR_ANY||ipcmp==INADDR_ANY)
					this->returnSystemCall(syscallUUID,-1);
			}
			else if(sockfd==fdcmp){
				if(ipcmp!=-1){
					this->returnSystemCall(syscallUUID,-1);
				}
				// else{
				// 	//continue;
				// 	SockContext *context= &mapfindbypid(pid,sockfd)->second;
				// 	context->srcIP=ipaddr;
				// 	context->srcPort=portnum;
				// }
			}
		}		
		it++;
	}
	SockContext *context= &mapfindbypid(pid,sockfd)->second;
	context->srcIP=ipaddr;
	context->srcPort=portnum;
	//RE? multi interface case
	addr_int->sin_addr.s_addr=0;
	this->returnSystemCall(syscallUUID,0);
}

void TCPAssignment::syscall_getsockname(UUID syscallUUID, int pid, int sockfd, struct sockaddr *addr, socklen_t *addrlen){

	SockContext *context= &mapfindbypid(pid,sockfd)->second;
	printf("getsockname pid is %d\n",context->pid);

	// if(addrinfo.compare("-1")==0)
	// 	return -1;

	uint32_t ipaddr= context->srcIP;
	unsigned short int portnum = context->srcPort;
	struct sockaddr_in *ret=(struct sockaddr_in *)addr;	

	memset(ret,0,sizeof(ret));
	
	ret->sin_family = AF_INET;
	ret->sin_addr.s_addr=htonl(ipaddr);
	ret->sin_port=htons(portnum);

	this->returnSystemCall(syscallUUID,0);
}

void TCPAssignment::syscall_getpeername(UUID syscallUUID, int pid, int sockfd, struct sockaddr *addr, socklen_t *addrlen){
	SockContext *context= &mapfindbypid(pid,sockfd)->second;
	
	uint32_t ipaddr= context->desIP;
	
	unsigned short int portnum = context->desPort;
	struct sockaddr_in *ret=(struct sockaddr_in *)addr;	

	memset(ret,0,sizeof(ret));
	
	ret->sin_family = AF_INET;
	ret->sin_addr.s_addr=htonl(ipaddr);
	ret->sin_port=htons(portnum);

	this->returnSystemCall(syscallUUID,0);
}

void TCPAssignment::packetArrived(std::string fromModule, Packet* packet)
{
	uint8_t srcIP[4];
	uint8_t desIP[4];
	uint32_t srcIP32;
	uint32_t desIP32;
	uint16_t srcPort;
	uint16_t desPort;
	uint8_t flags;
	uint32_t seqnum;
	uint32_t acknum;
	uint16_t checksum;
	int sockfd=-1;
	SockContext *context;

	packet->readData(14+12,&srcIP32,4);
	packet->readData(14+16,&desIP32,4);

	srcIP32=ntohl(srcIP32);
	desIP32=ntohl(desIP32);

	Header *tcpHeader = new Header();
	packet->readData(30+4, tcpHeader,20);

	srcPort=ntohs(tcpHeader->srcPort);
	desPort=ntohs(tcpHeader->desPort);
	flags = tcpHeader->flags;
	seqnum=ntohl(tcpHeader->seqnum);
	acknum=ntohl(tcpHeader->acknum);
	//checksum=ntohs(tcpHeader->checksum);

	// uint16_t val=NetworkUtil::tcp_sum(srcIP32, desIP32, (uint8_t*)tcpHeader, 20);
	// printf("val of checksum is %d\n",val);

	//client case connect
	if(flags==SYN+ACK){
		auto it=addrfdlist.begin();

		while(it!=addrfdlist.end()){
			uint32_t srcIPcmp = it->second.srcIP;
			uint16_t srcPortcmp = it->second.srcPort;
			uint32_t desIPcmp = it->second.desIP;
			uint16_t desPortcmp = it->second.desPort;

			if(srcPort==desPortcmp&&srcIP32==desIPcmp&&desPort==srcPortcmp&&desIP32==srcIPcmp){
				sockfd=it->first;
				context = &(it->second);
				break;
			}
			it++;
		}

		if(sockfd==-1){
			this->returnSystemCall(context->syscallID,-1);
			printf("error");
		}

		//synnum of client past and acknum of sender -1 has to be equal
		//checksum check REEEEEEEEEEEE!

		Packet* myPacket = this->clonePacket(packet);

		desIP32=htonl(desIP32);
		srcIP32=htonl(srcIP32);

		myPacket->writeData(14+12,&desIP32,4);
		myPacket->writeData(14+16,&srcIP32,4);

		tcpHeader->srcPort=htons(desPort);
		tcpHeader->desPort=htons(srcPort);
		tcpHeader->flags=ACK;
		tcpHeader->seqnum=htonl(acknum);
		tcpHeader->acknum=htonl(seqnum+1);
		tcpHeader->checksum=0;
		tcpHeader->checksum= htons(~(NetworkUtil::tcp_sum(desIP32, srcIP32, (uint8_t*)tcpHeader, 20)));

		myPacket->writeData(30+4, tcpHeader,20);

		//state change
		context->state=ESTAB;
		this->sendPacket("IPv4",myPacket);
		this->freePacket(packet);

		this->returnSystemCall(context->syscallID,0);
	}

	//server incoming client
	else if(flags==SYN){
		//printf("synnnnnnnnnn\n");
		auto it=addrfdlist.begin();

		while(it!=addrfdlist.end()){
			uint32_t srcIPcmp= it->second.srcIP;
			uint16_t srcPortcmp = it->second.srcPort;
			printf("addr is %d\n",srcIPcmp);
			if(desPort==srcPortcmp){
				if(desIP32==srcIPcmp||srcIPcmp==0){
					sockfd=it->first;
					context=&(it->second);
					break;
				}
			}
			it++;
		}
		//synnum of client past and acknum of sender -1 has to be equal
		//checksum check REEEEEEEEEEEE!

		//SockContext *context = &&mapfindbypid(pid,sockfd)->second->second);

		if(context->state!=LISTENS){
			//this->returnSystemCall(context->syscallID,-1);
			printf("????\n");
		}

		//int dupsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		int dupsock=createFileDescriptor(context->pid);
		printf("dupsock pid is %d\n",context->pid);
		printf("dupsock in syn is %d\n",dupsock);

		std::list<int> *backloglist= &context->backloglist;
		std::list<int> *dupsocklist= &context->dupsocklist;
		printf("sockfd in syn is %d\n",sockfd);

		if(backloglist->size()>=context->backlog)
			return;

		backloglist->push_back(dupsock);
		printf("size after push is %d\n",(backloglist)->size());
		dupsocklist->push_back(dupsock);

		SockContext dupsockcxt= *new SockContext;
		dupsockcxt.pid=context->pid;
		addrfdlist.insert(std::pair<int, SockContext>(dupsock,dupsockcxt));
		

		//context->dupsockfd=dupsock;

		SockContext *dupcontext = &mapfindbypid(context->pid,dupsock)->second;
		dupcontext->desIP=srcIP32;
		dupcontext->desPort=srcPort;
		dupcontext->srcIP=desIP32;
		dupcontext->srcPort=desPort;
		dupcontext->state=SYNRCVD;

		Packet* myPacket = this->clonePacket(packet);

		desIP32=htonl(desIP32);
		srcIP32=htonl(srcIP32);

		myPacket->writeData(14+12,&desIP32,4);
		myPacket->writeData(14+16,&srcIP32,4);

		tcpHeader->srcPort=htons(desPort);
		tcpHeader->desPort=htons(srcPort);
		tcpHeader->flags=ACK+SYN;
		tcpHeader->seqnum=0;
		tcpHeader->acknum=htonl(seqnum+1);
		tcpHeader->checksum=0;
		tcpHeader->checksum= htons(~(NetworkUtil::tcp_sum(desIP32, srcIP32, (uint8_t*)tcpHeader, 20)));

		//printf("srcport client port should be 46218 %d\n",ntohs(tcpHeader->srcPort));
		myPacket->writeData(30+4, tcpHeader,20);

		//state change
		this->sendPacket("IPv4",myPacket);
		this->freePacket(packet);	
	}

	//server side after sending synack
	else if(flags==ACK){
		printf("ack enter\n");
		int listensockfd=-1;
		SockContext * liscontext;
		auto it=addrfdlist.begin();
		
		while(it!=addrfdlist.end()){
			uint16_t desPortcmp = it->second.desPort;
			uint32_t desIPcmp=it->second.desIP;
			uint16_t srcPortcmp=it->second.srcPort;
			uint32_t srcIPcmp=it->second.srcIP;
			int statecmp=it->second.state;
					printf("srcIPcmp is %d\n",srcIPcmp);
			if(srcPort==desPortcmp&&srcIP32==desIPcmp){
				sockfd=it->first;
				context=&(it->second);
			}
			else if(desPort==srcPortcmp&&statecmp==LISTENS){
				if(desIP32==srcIPcmp||srcIPcmp==0){
					listensockfd=it->first;
					liscontext=&(it->second);
				}
			}
			it++;
		}

		//synnum of client past and acknum of sender -1 has to be equal
		//checksum check REEEEEEEEEEEE!

		//SockContext *context = &&mapfindbypid(pid,sockfd)->second->second);
		//SockContext *liscontext= &mapfindbypid(pid,listens.secondockfd)->second); 

		if(sockfd==-1||listensockfd==-1){
			this->returnSystemCall(liscontext->syscallID,-1);
		}

		if(context->state!=SYNRCVD){
			printf("synrevd not error!!!\n");
			this->returnSystemCall(liscontext->syscallID,-1);
			//return;
		}

		printf("listsockfd is %d\n",listensockfd);
		printf("pid is %d\n",liscontext->pid);
		std::list<int> *backloglist= &liscontext->backloglist;
		printf("before pop backloglist size is %d\n",backloglist->size());
		backloglist->pop_front();
		printf("after pop\n");

		context->state=ESTAB;

		this->freePacket(packet);

		//case when accept was first called
		if(liscontext->syscallID!=-1){
			printf("addrinfo\n");

			struct sockaddr_in *ret=(struct sockaddr_in *)(liscontext->addrinfo);	
			memset(ret,0,sizeof(ret));
		
			printf("sockfd in ack is %d\n",sockfd);
			(ret)->sin_family = AF_INET;
			(ret)->sin_addr.s_addr=htonl(context->srcIP);
			srcIP[0]=context->srcIP>>24;
			srcIP[1]=(u_char)(context->srcIP>>16);
			srcIP[2]=(u_char)(context->srcIP>>8);
			srcIP[3]=(u_char)context->srcIP;
			printf("srcIP in ack is %u.%u.%u.%u\n",srcIP[0],srcIP[1],srcIP[2],srcIP[3]);

			(ret)->sin_port=htons(context->desPort);
			this->returnSystemCall(liscontext->syscallID,sockfd);
		}	
	}
}

void TCPAssignment::timerCallback(void* payload)
{

}


}
