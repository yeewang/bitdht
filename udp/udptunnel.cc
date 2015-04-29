/*
 * bitdht/UdpTunnel.cc
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


#include "udp/udptunnel.h"
#include "bitdht/bdpeer.h"
#include "bitdht/bdstore.h"
#include "bitdht/bdmsgs.h"
#include "bitdht/bencode.h"
#include "bitdht/bdtunnelmanager.h"

#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#if !defined(_WIN32) && !defined(__MINGW32__)
#include <unistd.h>
#endif
#include <string.h>
#include "util/bdnet.h"

/*
 * #define DEBUG_UDP_BITDHT 1
 *
 * #define BITDHT_VERSION_ANONYMOUS	1
 */

//#define DEBUG_UDP_BITDHT 1

//#define BITDHT_VERSION_IDENTIFER	1
/*************************************/

UdpTunnel::UdpTunnel(UdpPublisher *pub, bdDhtFunctions *fns) :
		UdpSubReceiver(pub), mFns(fns)
{
	mTunnelManager = new bdTunnelManager(fns);
}

UdpTunnel::~UdpTunnel()
{ 
	return; 
}

/*********** External Interface to the World ************/

/***** Functions to Call down to bdNodeManager ****/

void UdpTunnel::connectNode(const bdId *id)
{
	bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/

	startTunnel();

	mTunnelManager->connectNode(id);
}

void UdpTunnel::disconnectNode(const bdId *id)
{
	bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/

	mTunnelManager->disconnectNode(id);
}

/***** Add / Remove Callback Clients *****/
void UdpTunnel::addCallback(BitDhtCallback *cb)
{
	bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/

	mTunnelManager->addCallback(cb);
}

void UdpTunnel::removeCallback(BitDhtCallback *cb)
{
	bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/

	mTunnelManager->removeCallback(cb);
}

/* stats and Dht state */
int UdpTunnel::startTunnel()
{
	bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/

	return mTunnelManager->startTunnel();
}

int UdpTunnel:: stopTunnel()
{
	bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/

	return mTunnelManager->stopTunnel();
}

int UdpTunnel::stateDht()
{
	bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/

	return mTunnelManager->stateDht();
}

/******************* Internals *************************/

/***** Iteration / Loop Management *****/

/*** Overloaded from UdpSubReceiver ***/
int UdpTunnel::recvPkt(void *data, int size, struct sockaddr_in &from)
{
	/* pass onto bitdht */
	bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/

	mTunnelManager->incomingMsg(&from, (char *) data, size);

	return 0;
}

int UdpTunnel::status(std::ostream &out)
{
	out << "UdpTunnel::status()" << std::endl;

	return 1;
}

/*** Overloaded from iThread ***/
#define MAX_MSG_PER_TICK	100
#define TICK_PAUSE_USEC		20000  /* 20ms secs .. max messages = 50 x 100 = 5000 */

void UdpTunnel::run()
{
	while(1)
	{
		while(tick())
		{
			usleep(TICK_PAUSE_USEC);
		}

		mTunnelManager->iteration();
		sleep(1);
	}
}

int UdpTunnel::tick()
{
	bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/

	/* pass on messages from the node */
	int i = 0;
	char data[BITDHT_MAX_PKTSIZE];
	struct sockaddr_in toAddr;
	int size = BITDHT_MAX_PKTSIZE;

	while((i < MAX_MSG_PER_TICK) && (mTunnelManager->outgoingMsg(&toAddr, data, &size)))
	{
#ifdef DEBUG_UDP_BITDHT 
		LOG << log4cpp::Priority::INFO << "UdpTunnel::tick() outgoing msg(" << size << ") to " << toAddr;
		LOG << log4cpp::Priority::INFO << std::endl;
#endif

		sendPkt(data, size, toAddr, BITDHT_TTL);

		// iterate
		i++;
		size = BITDHT_MAX_PKTSIZE; // reset msg size!
	}

	if (i == MAX_MSG_PER_TICK)
	{
		return 1; /* keep on ticking */
	}
	return 0;
}
