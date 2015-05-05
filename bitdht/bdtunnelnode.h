#ifndef BITDHT_TUNNEL_NODE_H
#define BITDHT_TUNNEL_NODE_H

/*
 * bitdht/bdTunnelNode.h
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


#include "bitdht/bdpeer.h"
#include "bitdht/bdquery.h"
#include "bitdht/bdstore.h"
#include "bitdht/bdobj.h"
#include "bitdht/bdhash.h"
#include "bitdht/bdhistory.h"


#define BD_QUERY_NEIGHBOURS		1
#define BD_QUERY_HASH			2

class bdTunnelNodeNetMsg
{
public:
	bdTunnelNodeNetMsg(char *data, int size, struct sockaddr_in *addr);
	~bdTunnelNodeNetMsg();

	void print(std::ostream &out);

	char *data;
	int mSize;
	struct sockaddr_in addr;
};

class bdTunnelReq {
public:
	bdTunnelReq(const bdId &id) : mId(id) {};

	bdId mId;
};

class bdTunnelReqStatus
{
public:
	uint32_t mStatus;
	uint32_t mQFlags;
	std::list<bdId> mResults;
};

class bdTunnelNode
{
public:
	bdTunnelNode(bdDhtFunctions *fns);

	/* startup / shutdown node */
	void restartNode();
	void shutdownNode();

	void printState();
	void checkPotentialPeer(bdId *id);
	void addPotentialPeer(bdId *id);

	void addTunnel(const bdId *id);
	void clearTunnel(const bdId *id);
	void tunnelStatus(std::map<bdId, bdTunnelReqStatus> &statusMap);

	void iterationOff();
	void iteration();

	/* interaction with outside world */
	int outgoingMsg(struct sockaddr_in *addr, char *msg, int *len);
	void incomingMsg(struct sockaddr_in *addr, char *msg, int len);

	/* internal interaction with network */
	void sendPkt(char *msg, int len, struct sockaddr_in addr);
	void recvPkt(char *msg, int len, struct sockaddr_in addr);

	void msgin_newconn(bdId *tunnelId, bdToken *transId);
	void msgin_reply_newconn(bdId *tunnelId, bdToken *transId);

	/* output functions (send msg) */
	void msgout_newconn(bdId *dhtId, bdToken *transId);
	void msgout_reply_newconn(bdId *tunnelId, bdToken *transId);

	/* token handling */
	void genNewToken(bdToken *token);
	int queueQuery(bdId *id, bdNodeId *query, bdToken *transId, uint32_t query_type);

	/* transId handling */
	void genNewTransId(bdToken *token);
	uint32_t checkIncomingMsg(bdId *id, bdToken *transId, uint32_t msgType);
	void cleanupTransIdRegister();

	void getOwnId(bdNodeId *id);

	void doStats();

protected:
	bdNodeId 	mOwnId;
	bdId 		mLikelyOwnId; // Try to workout own id address.

private:
	bdDhtFunctions *mFns;
	std::list<bdTunnelReq *> mTunnelRequests;
	std::list<bdTunnelNodeNetMsg *> mOutgoingMsgs;
	std::list<bdTunnelNodeNetMsg *> mIncomingMsgs;
};

#endif // BITDHT_TUNNEL_NODE_H
