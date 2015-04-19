
/*
 * util/bdnet.cc
 *
 * BitDHT: An Flexible DHT library.
 *
 * Copyright 2010 by Robert Fernie
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 3 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "bitdht@lunamutt.com".
 *
 */

#include "bdnet.h"

#include <iostream>
#include <stdlib.h>
#include <string.h>

/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#if defined(_WIN32) || defined(__MINGW32__)


/* error handling */
int bdnet_int_errno;

int bdnet_errno() 
{
	return bdnet_int_errno;
}

int bdnet_init() 
{ 
	LOG << log4cpp::Priority::INFO << "bdnet_init()" << std::endl;
	bdnet_int_errno = 0;

        // Windows Networking Init.
        WORD wVerReq = MAKEWORD(2,2);
        WSADATA wsaData;

        if (0 != WSAStartup(wVerReq, &wsaData))
        {
                LOG << log4cpp::Priority::INFO << "Failed to Startup Windows Networking";
                LOG << log4cpp::Priority::INFO << std::endl;
        }
        else
        {
                LOG << log4cpp::Priority::INFO << "Started Windows Networking";
                LOG << log4cpp::Priority::INFO << std::endl;
        }

	return 0; 
}

/* check if we can modify the TTL on a UDP packet */
int bdnet_checkTTL(int fd) 
{
	LOG << log4cpp::Priority::INFO << "bdnet_checkTTL()" << std::endl;
	int optlen = 4;
	char optval[optlen];

	int ret = getsockopt(fd, IPPROTO_IP, IP_TTL, optval, &optlen);
	//int ret = getsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, optval, &optlen);
	if (ret == SOCKET_ERROR)
	{
		ret = -1;
		bdnet_int_errno = bdnet_w2u_errno(WSAGetLastError());
		LOG << log4cpp::Priority::INFO << "bdnet_checkTTL() Failed!";
		LOG << log4cpp::Priority::INFO << std::endl;
	}
	else
	{
		LOG << log4cpp::Priority::INFO << "bdnet_checkTTL() :";
		LOG << log4cpp::Priority::INFO << (int) optval[0] << ":";
		LOG << log4cpp::Priority::INFO << (int) optval[1] << ":";
		LOG << log4cpp::Priority::INFO << (int) optval[2] << ":";
		LOG << log4cpp::Priority::INFO << (int) optval[3] << ": RET: ";
		LOG << log4cpp::Priority::INFO << ret << ":";
		LOG << log4cpp::Priority::INFO << std::endl;
	}
	return ret;
}

int bdnet_close(int fd) 
{ 
	LOG << log4cpp::Priority::INFO << "bdnet_close()" << std::endl;
	return closesocket(fd); 
}

int bdnet_socket(int domain, int type, int protocol)
{
        int osock = socket(domain, type, protocol);
	LOG << log4cpp::Priority::INFO << "bdnet_socket()" << std::endl;

	if ((unsigned) osock == INVALID_SOCKET)
	{
		// Invalidate socket Unix style.
		osock = -1;
	        bdnet_int_errno = bdnet_w2u_errno(WSAGetLastError());
	}
	bdnet_checkTTL(osock);
	return osock;
}

int bdnet_bind(int  sockfd,  const  struct  sockaddr  *my_addr,  socklen_t addrlen)
{
	LOG << log4cpp::Priority::INFO << "bdnet_bind()" << std::endl;
	int ret = bind(sockfd,my_addr,addrlen);
	if (ret != 0)
	{
		/* store unix-style error
		 */

		ret = -1;
		bdnet_int_errno = bdnet_w2u_errno(WSAGetLastError());
	}
	return ret;
}

int bdnet_fcntl(int fd, int cmd, long arg)
{
        int ret;
	
	unsigned long int on = 1;
	LOG << log4cpp::Priority::INFO << "bdnet_fcntl()" << std::endl;

	/* can only do NONBLOCK at the moment */
	if ((cmd != F_SETFL) || (arg != O_NONBLOCK))
	{
		LOG << log4cpp::Priority::INFO << "bdnet_fcntl() limited to fcntl(fd, F_SETFL, O_NONBLOCK)";
		LOG << log4cpp::Priority::INFO << std::endl;
		bdnet_int_errno =  EOPNOTSUPP;
		return -1;
	}

	ret = ioctlsocket(fd, FIONBIO, &on);

	if (ret != 0)
	{
		/* store unix-style error
		 */

		ret = -1;
		bdnet_int_errno = bdnet_w2u_errno(WSAGetLastError());
	}
	return ret;
}

int bdnet_setsockopt(int s, int level, int optname, 
				const void *optval, socklen_t optlen)
{
	LOG << log4cpp::Priority::INFO << "bdnet_setsockopt() val:" << *((int *) optval) << std::endl;
	LOG << log4cpp::Priority::INFO << "bdnet_setsockopt() len:" << optlen << std::endl;
	if ((level != IPPROTO_IP) || (optname != IP_TTL))
	{
		LOG << log4cpp::Priority::INFO << "bdnet_setsockopt() limited to ";
		LOG << log4cpp::Priority::INFO << "setsockopt(fd, IPPROTO_IP, IP_TTL, ....)";
		LOG << log4cpp::Priority::INFO << std::endl;
		bdnet_int_errno =  EOPNOTSUPP;
		return -1;
	}

	int ret = setsockopt(s, level, optname, (const char *) optval, optlen);
	//int ret = setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, (const char *) optval, optlen);
	if (ret == SOCKET_ERROR)
	{
		ret = -1;
		bdnet_int_errno = bdnet_w2u_errno(WSAGetLastError());
	}
	bdnet_checkTTL(s);
	return ret;
}

ssize_t bdnet_recvfrom(int s, void *buf, size_t len, int flags,
                              struct sockaddr *from, socklen_t *fromlen)
{
	LOG << log4cpp::Priority::INFO << "bdnet_recvfrom()" << std::endl;
	int ret = recvfrom(s, (char *) buf, len, flags, from, fromlen);
	if (ret == SOCKET_ERROR)
	{
		ret = -1;
		bdnet_int_errno = bdnet_w2u_errno(WSAGetLastError());
	}
	return ret;
}

ssize_t bdnet_sendto(int s, const void *buf, size_t len, int flags, 
 				const struct sockaddr *to, socklen_t tolen)
{
	LOG << log4cpp::Priority::INFO << "bdnet_sendto()" << std::endl;
	int ret = sendto(s, (const char *) buf, len, flags, to, tolen);
	if (ret == SOCKET_ERROR)
	{
		ret = -1;
		bdnet_int_errno = bdnet_w2u_errno(WSAGetLastError());
	}
	return ret;
}

int bdnet_w2u_errno(int err)
{
	/* switch */
	LOG << log4cpp::Priority::INFO << "tou_net_w2u_errno(" << err << ")" << std::endl;
	switch(err)
	{
		case WSAEINPROGRESS:
			return EINPROGRESS;
			break;
		case WSAEWOULDBLOCK:
			return EINPROGRESS;
			break;
		case WSAENETUNREACH:
			return ENETUNREACH;
			break;
		case WSAETIMEDOUT:
			return ETIMEDOUT;
			break;
		case WSAEHOSTDOWN:
			return EHOSTDOWN;
			break;
		case WSAECONNREFUSED:
			return ECONNREFUSED;
			break;
		case WSAEADDRINUSE:
			return EADDRINUSE;
			break;
		case WSAEUSERS:
			return EUSERS;
			break;
		/* This one is returned for UDP recvfrom, when nothing there
		 * but not a real error... translate into EINPROGRESS
		 */
		case WSAECONNRESET:
			LOG << log4cpp::Priority::INFO << "tou_net_w2u_errno(" << err << ")";
			LOG << log4cpp::Priority::INFO << " = WSAECONNRESET ---> EINPROGRESS";
	 		LOG << log4cpp::Priority::INFO << std::endl;
			return EINPROGRESS;
			break;
		/***
		 *
		case WSAECONNRESET:
			return ECONNRESET;
			break;
		 *
		 ***/
		
		default:
			LOG << log4cpp::Priority::INFO << "tou_net_w2u_errno(" << err << ") Unknown";
			LOG << log4cpp::Priority::INFO << std::endl;
			break;
	}

	return ECONNREFUSED; /* sensible default? */
}

int bdnet_inet_aton(const char *name, struct in_addr *addr)
{
        return (((*addr).s_addr = inet_addr(name)) != INADDR_NONE);
}



int sleep(unsigned int sec)
{ 
	Sleep(sec * 1000); 
	return 0;
}

int usleep(unsigned int usec)
{ 
	Sleep(usec / 1000); 
	return 0;
}

/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#else // UNIX

/* Unix Version is easy -> just call the unix fn
 */

#include <unistd.h> /* for close definition */

/* the universal interface */
int bdnet_init() { return 0; }
int bdnet_errno() { return errno; }


/* check if we can modify the TTL on a UDP packet */
int bdnet_checkTTL(int /*fd*/) { return 1;}


int bdnet_inet_aton(const char *name, struct in_addr *addr)
{
        return inet_aton(name, addr);
}

int bdnet_close(int fd) { return close(fd); }

int bdnet_socket(int domain, int type, int protocol)
{
	return socket(domain, type, protocol);
}

int bdnet_bind(int  sockfd,  const  struct  sockaddr  *my_addr,  socklen_t addrlen)
{
	return bind(sockfd,my_addr,addrlen);
}

int bdnet_fcntl(int fd, int cmd, long arg)
{
	return fcntl(fd, cmd, arg);
}

int bdnet_setsockopt(int s, int level, int optname, 
				const void *optval, socklen_t optlen)
{
	return setsockopt(s, level, optname,  optval, optlen);
}

ssize_t bdnet_recvfrom(int s, void *buf, size_t len, int flags,
                              struct sockaddr *from, socklen_t *fromlen)
{
	return recvfrom(s, buf, len, flags, from, fromlen);
}

ssize_t bdnet_sendto(int s, const void *buf, size_t len, int flags, 
 				const struct sockaddr *to, socklen_t tolen)
{
	return sendto(s, buf, len, flags, to, tolen);
}


#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/

