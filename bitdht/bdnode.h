#ifndef BITDHT_NODE_H
#define BITDHT_NODE_H

/*
 * bitdht/bdnode.h
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

/**********************************
 * Running a node....
 *
 * run().
 * loops through and checks out of date peers.
 * handles searches.
 * prints out dht Table.
 *

The node handles the i/o traffic from peers.
It 



ping, return
peers, return
hash store, return
hash get, return



respond queue.

query queue.





input -> call into recvFunction()
output -> call back to Udp().




 *********/

class bdNodeNetMsg
{

public:
	bdNodeNetMsg(char *data, int size, struct sockaddr_in *addr);
	~bdNodeNetMsg();

	void print(std::ostream &out);

	char *data;
	int mSize;
	struct sockaddr_in addr;

};

class bdNode
{
public:
	bdNode(bdNodeId *id,
			const std::string &dhtVersion,
			const std::string &bootfile,
			const std::string &whitelist,
			bdDhtFunctions *fns,
			PacketCallback *packetCallback);

	/* startup / shutdown node */
	void restartNode();
	void shutdownNode();

	// virtual so manager can do callback.
	// peer flags defined in bdiface.h
	virtual void addPeer(const bdId *id, uint32_t peerflags);

	void printState();
	void checkPotentialPeer(bdId *id);
	void addPotentialPeer(bdId *id);

	void addQuery(const bdNodeId *id, uint32_t qflags);
	void clearQuery(const bdNodeId *id);
	void QueryStatus(std::map<bdNodeId, bdQueryStatus> &statusMap);
	bool getIdFromQuery(const bdNodeId *id, std::list<bdPeer> &idList);

	void iterationOff();
	void iteration();
	void processRemoteQuery();
	void updateStore();

	void addConnReq(const bdNodeId &id);
	void broadcastPeers();

	/* interaction with outside world */
	int 	outgoingMsg(struct sockaddr_in *addr, char *msg, int *len);
	void 	incomingMsg(struct sockaddr_in *addr, char *msg, int len);

	/* internal interaction with network */
	void	sendPkt(char *msg, int len, struct sockaddr_in addr);
	void	recvPkt(char *msg, int len, struct sockaddr_in addr);

	/* output functions (send msg) */
	void msgout_ping(bdId *id, bdToken *transId);
	void msgout_pong(bdId *id, bdToken *transId);
	void msgout_find_node(bdId *id, bdToken *transId, bdNodeId *query);
	void msgout_reply_find_node(bdId *id, bdToken *transId, 
			std::list<bdId> &peers);
	void msgout_get_hash(bdId *id, bdToken *transId, bdNodeId *info_hash);
	void msgout_reply_hash(bdId *id, bdToken *transId, 
			bdToken *token, std::list<std::string> &values);
	void msgout_reply_nearest(bdId *id, bdToken *transId, 
			bdToken *token, std::list<bdId> &peers);

	void msgout_post_hash(bdId *id, bdToken *transId, bdNodeId *info_hash, 
			uint32_t port, bdToken *token);
	void msgout_reply_post(bdId *id, bdToken *transId);

	void msgout_ask_myip(const bdId *dhtId, bdToken *transId);
	void msgout_reply_ask_myip(bdId *tunnelId, bdToken *transId);

	void msgout_broadcast_conn(const bdId *id, bdToken *tid, bdNodeId *nodeId, bdNodeId *peerId);
	void msgout_ask_conn(const bdId *id, bdToken *tid, bdNodeId *nodeId, bdId *peerId);
	void msgout_reply_conn(const bdId *id, bdToken *tid, bdNodeId *nodeId, bool started);

	/* input functions (once mesg is parsed) */
	void msgin_ping(bdId *id, bdToken *token);
	void msgin_pong(bdId *id, bdToken *transId, bdToken *versionId);

	void msgin_find_node(bdId *id, bdToken *transId, bdNodeId *query);
	void msgin_reply_find_node(bdId *id, bdToken *transId, 
			std::list<bdId> &entries);

	void msgin_get_hash(bdId *id, bdToken *transId, bdNodeId *nodeid);
	void msgin_reply_hash(bdId *id, bdToken *transId, 
			bdToken *token, std::list<std::string> &values);
	void msgin_reply_nearest(bdId *id, bdToken *transId, 
			bdToken *token, std::list<bdId> &nodes);

	void msgin_post_hash(bdId *id,  bdToken *transId,  
			bdNodeId *info_hash,  uint32_t port, bdToken *token);
	void msgin_reply_post(bdId *id, bdToken *transId);

	void msgin_ask_myip(bdId *tunnelId, bdToken *transId);
	void msgin_reply_ask_myip(bdId *tunnelId, bdToken *transId);

	void msgin_broadcast_conn(bdId *id, bdToken *tid, bdNodeId *nodeId, bdNodeId *peerId);
	void msgin_ask_conn(bdId *id, bdToken *tid, bdNodeId *nodeId, bdId *peerId);
	void msgin_reply_conn(bdId *id, bdToken *tid, bdNodeId *nodeId, bool started);

	/* token handling */
	void genNewToken(bdToken *token);
	int queueQuery(bdId *id, bdNodeId *query, bdToken *transId, uint32_t query_type);

	/* transId handling */
	void genNewTransId(bdToken *token);
	void registerOutgoingMsg(bdId *id, bdToken *transId, uint32_t msgType);
	uint32_t checkIncomingMsg(bdId *id, bdToken *transId, uint32_t msgType);
	void cleanupTransIdRegister();

	void getOwnId(bdNodeId *id);

	void doStats();
	void printStats();
	void printQueries();

	void resetCounters();
	void resetStats();

protected:
	bool isMemberOfBlackList(sockaddr_in &blackAddr);
	void addBlackList(sockaddr_in &blackAddr);
	void removeBlackList(sockaddr_in &blackAddr);

protected:
	bdNodeId mOwnId;
	std::list<sockaddr_in> mMyIPs;
	bdSpace mNodeSpace;

private:
	uint32_t getRandomToken();
	bool isUsedToken(uint32_t);
	void punching(int times);

private:
	bdStore mStore;
	bdStore mWhiteNodes;
	std::list<sockaddr_in> mBlackNodes;
	std::string mDhtVersion;

	bdDhtFunctions *mFns;
	PacketCallback *mPacketCallback;

	bdHashSpace mHashSpace;

	bdHistory mHistory; /* for understanding the DHT */

	std::list<bdQuery *> mLocalQueries;
	std::list<bdRemoteQuery> mRemoteQueries;
	std::list<bdId> mPotentialPeers;

	std::map<bdNodeId, bdNodeId> mConnectRequests;
	std::map<bdNodeId, struct sockaddr_in> mPeerAddrs;
	std::list<bdId> mPunching;

	std::list<bdNodeNetMsg *> mOutgoingMsgs;
	std::list<bdNodeNetMsg *> mIncomingMsgs;

	std::vector<uint32_t> mRandomTokenArray;

	// Statistics.
	double mCounterOutOfDatePing;
	double mCounterPings;
	double mCounterPongs;
	double mCounterQueryNode;
	double mCounterQueryHash;
	double mCounterReplyFindNode;
	double mCounterReplyQueryHash;

	double mCounterRecvPing;
	double mCounterRecvPong;
	double mCounterRecvQueryNode;
	double mCounterRecvQueryHash;
	double mCounterRecvReplyFindNode;
	double mCounterRecvReplyQueryHash;

	double mLpfOutOfDatePing;
	double mLpfPings;
	double mLpfPongs;
	double mLpfQueryNode;
	double mLpfQueryHash;
	double mLpfReplyFindNode;
	double mLpfReplyQueryHash;

	double mLpfRecvPing;
	double mLpfRecvPong;
	double mLpfRecvQueryNode;
	double mLpfRecvQueryHash;
	double mLpfRecvReplyFindNode;
	double mLpfRecvReplyQueryHash;
};

#endif // BITDHT_NODE_H
