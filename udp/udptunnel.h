#ifndef UDP_TUNNEL_CLASS_H
#define UDP_TUNNEL_CLASS_H

/*
 * bitdht/UdpTunnel.h
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


#include <iosfwd>
#include <map>
#include <string>

#include "udp/udpstack.h"
#include "bitdht/bdiface.h"
#include "bitdht/bdtunnelmanager.h"

/* 
 * This implements a UdpSubReceiver class to allow the DHT to talk to the network.
 * The parser is very strict - and will try to not pick up anyone else's messages.
 *
 * Mutexes are implemented at this level protecting the whole of the DHT code.
 * This class is also a thread - enabling it to do callback etc.
 */

// class BitDhtCallback defined in bdiface.h 

class UdpTunnel: public UdpSubReceiver, public bdThread
{
public:

	UdpTunnel(const bdNodeId &ownId, UdpPublisher *pub, bdDhtFunctions *fns);
	virtual ~UdpTunnel();

	/*********** External Interface to the World (BitDhtInterface) ************/

	/***** Functions to Call down to bdTunnelManager ****/
	void ask(const bdId *id);

	/* Request DHT Peer Lookup */
	/* Request Keyword Lookup */
	virtual	void connectNode(const bdId *id);
	virtual	void disconnectNode(const bdId *id);

	/***** Add / Remove Callback Clients *****/
	virtual	void addCallback(BitDhtCallback *cb);
	virtual	void removeCallback(BitDhtCallback *cb);

	/* stats and Dht state */
	virtual int startTunnel();
	virtual int stopTunnel();
	virtual int stateDht();

	/******************* Internals *************************/
	/***** Iteration / Loop Management *****/

	/*** Overloaded from UdpSubReceiver ***/
	virtual int recvPkt(void *data, int size, struct sockaddr_in &from);
	virtual int status(std::ostream &out);


	/*** Overloaded from iThread ***/
	virtual void run();

	/**** do whats to be done ***/
	int tick();

private:

	bdMutex dhtMtx; /* for all class data (below) */
	bdTunnelManager *mTunnelManager;
	bdDhtFunctions *mFns;
};

#endif
