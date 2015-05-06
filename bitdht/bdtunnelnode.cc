/*
 * bitdht/bdTunnelNode.cc
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

#include "bitdht/bdtunnelnode.h"

#include "bitdht/bencode.h"
#include "bitdht/bdmsgs.h"

#include "util/bdnet.h"
#include "util/bdlog.h"

#include <string.h>
#include <stdlib.h>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <atomic>


#define BITDHT_QUERY_START_PEERS			10
#define BITDHT_QUERY_NEIGHBOUR_PEERS		8
#define BITDHT_MAX_REMOTE_QUERY_AGE			10

/****
 * #define USE_HISTORY	1
 *
 * #define DEBUG_NODE_MULTIPEER 1
 * #define DEBUG_NODE_PARSE 1

 * #define DEBUG_NODE_MSGS 1
 * #define DEBUG_NODE_ACTIONS 1

 * #define DEBUG_NODE_MSGIN 1
 * #define DEBUG_NODE_MSGOUT 1
 ***/

// #define DEBUG_NODE_MSGS 1


bdTunnelNode::bdTunnelNode(const bdNodeId &ownId, bdDhtFunctions *fns) :
	mOwnId(ownId), mFns(fns)
{
}

void bdTunnelNode::getOwnId(bdNodeId *id)
{
	*id = mOwnId;
}

/***** Startup / Shutdown ******/
void bdTunnelNode::restartNode()
{
}

void bdTunnelNode::shutdownNode()
{
	/* clean up any outgoing messages */
	while(mOutgoingMsgs.size() > 0)
	{
		bdTunnelNodeNetMsg *msg = mOutgoingMsgs.front();
		mOutgoingMsgs.pop_front();

		/* cleanup message */
		delete msg;
	}
}

void bdTunnelNode::iterationOff()
{
	/* clean up any incoming messages */
	while(mIncomingMsgs.size() > 0)
	{
		bdTunnelNodeNetMsg *msg = mIncomingMsgs.front();
		mIncomingMsgs.pop_front();

		/* cleanup message */
		delete msg;
	}
}

void bdTunnelNode::iteration()
{
	/* process incoming msgs */
	while (mIncomingMsgs.size() > 0)
	{
		bdTunnelNodeNetMsg *msg = mIncomingMsgs.front();
		mIncomingMsgs.pop_front();

		recvPkt(msg->data, msg->mSize, msg->addr);

		/* cleanup message */
		delete msg;
	}

	/* allow each query to send up to one query... until maxMsgs has been reached */
	int numQueries = mTunnelRequests.size();
	int i = 0;

#ifdef DEBUG_NODE_MULTIPEER
	LOG.info("bdTunnelNode::iteration():%d", numQueries);
#endif

	/* iterate through queries */
	while(i < numQueries)
	{
#ifdef DEBUG_NODE_MULTIPEER
		LOG.info("bdTunnelNode::iteration():num %d, index.%d", numQueries, i);
#endif
		bdTunnelReq *query = mTunnelRequests.front();
		mTunnelRequests.pop_front();
		mTunnelRequests.push_back(query);

		/* push out query */
		bdToken transId;
		genNewTransId(&transId);

		msgout_newconn(&query->mId, &transId);
		i++;
	}
}

/************************************ Tunnel Details        *************************/
void bdTunnelNode::addTunnel(const bdId *id)
{
	std::list<bdTunnelReq *>::iterator it;
	for(it = mTunnelRequests.begin(); it != mTunnelRequests.end(); it++) {
		if ((*it)->mId == *id) {
#ifdef DEBUG_NODE_MULTIPEER
			LOG.info("bdTunnelNode::addTunnel(): skip");
#endif
			return;
		}
	}
#ifdef DEBUG_NODE_MULTIPEER
	LOG.info("bdTunnelNode::addTunnel(): add");
#endif
	bdTunnelReq *req = new bdTunnelReq(*id);
	mTunnelRequests.push_back(req);
}

void bdTunnelNode::clearTunnel(const bdId *rmId)
{
	std::list<bdTunnelReq *>::iterator it;
	for(it = mTunnelRequests.begin(); it != mTunnelRequests.end();)
	{
		if ((*it)->mId == *rmId)
		{
			bdTunnelReq *req = (*it);
			it = mTunnelRequests.erase(it);
			delete req;
		}
		else
		{
			it++;
		}
	}
}

void bdTunnelNode::tunnelStatus(std::map<bdId, bdTunnelReqStatus> &statusMap)
{
	std::list<bdTunnelReq *>::iterator it;
	for(it = mTunnelRequests.begin(); it != mTunnelRequests.end(); it++)
	{
		bdTunnelReqStatus status;
		// TODO::
	}
}

/************************************ Message Buffering ****************************/

/* interaction with outside world */
int bdTunnelNode::outgoingMsg(struct sockaddr_in *addr, char *msg, int *len)
{
	if (mOutgoingMsgs.size() > 0)
	{
		bdTunnelNodeNetMsg *bdmsg = mOutgoingMsgs.front();
		//bdmsg->print(LOG << log4cpp::Priority::INFO);
		mOutgoingMsgs.pop_front();
		//bdmsg->print(LOG << log4cpp::Priority::INFO);

		/* truncate if necessary */
		if (bdmsg->mSize < *len)
		{
			//LOG.info("bdTunnelNode::outgoingMsg space(" << *len << ") msgsize(" << bdmsg->mSize << ")";
			//LOG.info(std::endl;
			*len = bdmsg->mSize;
		}
		else
		{
			//LOG.info("bdTunnelNode::outgoingMsg space(" << *len << ") small - trunc from: "
			//<< bdmsg->mSize;
			//LOG.info(std::endl;
		}


		memcpy(msg, bdmsg->data, *len);
		*addr = bdmsg->addr;

		//bdmsg->print(LOG << log4cpp::Priority::INFO);

		delete bdmsg;
		return 1;
	}
	return 0;
}

void bdTunnelNode::incomingMsg(struct sockaddr_in *addr, char *msg, int len)
{
	LOG.info("bdTunnelNode::incomingMsg() ******************************* Address:%s:%d",
			inet_ntoa(addr->sin_addr), htons(addr->sin_port));

	bdTunnelNodeNetMsg *bdmsg = new bdTunnelNodeNetMsg(msg, len, addr);
	mIncomingMsgs.push_back(bdmsg);
}

/************************************ Message Handling *****************************/


void bdTunnelNode::msgin_newconn(bdId *tunnelId, bdToken *transId)
{
	msgout_reply_newconn(tunnelId, transId);
}

void bdTunnelNode::msgin_reply_newconn(bdId *tunnelId, bdToken *transId)
{

}

/* Outgoing Messages */

void bdTunnelNode::msgout_newconn(bdId *dhtId, bdToken *transId)
{
#ifdef DEBUG_NODE_MSGOUT
	std::ostringstream ss;
	bdPrintTransId(ss, transId);
	LOG.info("bdTunnelNode::msgout_newconn() TransId: %s To: %s",
			ss.str().c_str(), mFns->bdPrintId(dhtId).c_str());
#endif

	/* create string */
	char msg[10240];
	int avail = 10240;

	int blen = bitdht_new_conn_msg(transId, &(mOwnId), msg, avail-1);
	sendPkt(msg, blen, dhtId->addr);
}

void bdTunnelNode::msgout_reply_newconn(bdId *tunnelId, bdToken *transId)
{
	// #ifdef DEBUG_NODE_MSGOUT
	std::ostringstream ss;
	bdPrintTransId(ss, transId);
	LOG.info("BUGBUG: bdTunnelNode::msgout_reply_newconn() TransId: %s To: %s",
			ss.str().c_str(), mFns->bdPrintId(tunnelId).c_str());
	// #endif

	/* create string */
	char msg[10240];
	int avail = 10240;

	int blen = bitdht_reply_new_conn_msg(transId, &(mOwnId), tunnelId,
			msg, true, avail-1);
	sendPkt(msg, blen, tunnelId->addr);
}

void bdTunnelNode::sendPkt(char *msg, int len, struct sockaddr_in addr)
{
	bdTunnelNodeNetMsg *bdmsg = new bdTunnelNodeNetMsg(msg, len, &addr);
	mOutgoingMsgs.push_back(bdmsg);

	return;
}

/********************* Incoming Messages *************************/
/*
 * These functions are holding up udp queue -> so quickly
 * parse message, and get on with it!
 */

void bdTunnelNode::recvPkt(char *msg, int len, struct sockaddr_in addr)
{
	LOG.info("bdTunnelNode::recvPkt() ******************************* Address:%s:%d",
			inet_ntoa(addr.sin_addr), htons(addr.sin_port));

#ifdef DEBUG_NODE_PARSE
	LOG.info("bdTunnelNode::recvPkt() msg[" << len << "] = ";
	for(int i = 0; i < len; i++)
	{
		if ((msg[i] > 31) && (msg[i] < 127))
		{
			LOG.info(msg[i];
		}
		else
		{
			LOG.info("[" << (int) msg[i] << "]";
		}
	}
	LOG.info(std::endl;
#endif

	/* convert to a be_node */
	be_node *node = be_decoden(msg, len);
	if (!node)
	{
		/* invalid decode */
#ifdef DEBUG_NODE_PARSE
		LOG.info("bdTunnelNode::recvPkt() Failure to decode. Dropping Msg";
		LOG.info(std::endl;
		LOG.info("message length: " << len;
		LOG.info(std::endl;
		LOG.info("msg[] = ";
		for(int i = 0; i < len; i++)
		{
			if ((msg[i] > 31) && (msg[i] < 127))
			{
				LOG.info(msg[i];
			}
			else
			{
				LOG.info("[" << (int) msg[i] << "]";
			}
		}
		LOG.info(std::endl;
#endif
		return;
	}

	/* find message type */
	uint32_t beType = beMsgType(node);
	bool beQuery = (BE_Y_Q == beMsgGetY(node));

	if (!beType)
	{
#ifdef DEBUG_NODE_PARSE
		LOG.info("bdTunnelNode::recvPkt() Invalid Message Type. Dropping Msg";
		LOG.info(std::endl;
#endif
		/* invalid message */
		be_free(node);
		return;
	}

	/************************* handle token (all) **************************/
	be_node *be_transId = beMsgGetDictNode(node, "t");
	bdToken transId;
	if (be_transId)
	{
		beMsgGetToken(be_transId, transId);
	}
	else
	{
#ifdef DEBUG_NODE_PARSE
		LOG.info("bdTunnelNode::recvPkt() TransId Failure. Dropping Msg";
		LOG.info(std::endl;
#endif
		be_free(node);
		return;
	}

	/************************* handle data  (all) **************************/

	/* extract common features */
	char dictkey[2] = "r";
	if (beQuery)
	{
		dictkey[0] = 'a';
	}

	be_node *be_data = beMsgGetDictNode(node, dictkey);
	if (!be_data)
	{
#ifdef DEBUG_NODE_PARSE
		LOG.info("bdTunnelNode::recvPkt() Missing Data Body. Dropping Msg";
		LOG.info(std::endl;
#endif
		be_free(node);
		return;
	}

	/************************** handle id (all) ***************************/
	be_node *be_id = beMsgGetDictNode(be_data, "id");
	bdNodeId id;
	if (be_id)
	{
		beMsgGetNodeId(be_id, id);
	}
	else
	{
#ifdef DEBUG_NODE_PARSE
		LOG.info("bdTunnelNode::recvPkt() Missing Peer Id. Dropping Msg";
		LOG.info(std::endl;
#endif
		be_free(node);
		return;
	}

	/************************ handle version (optional:pong) **************/
	be_node *be_version = NULL;
	bdToken versionId;
	if (beType == BITDHT_MSG_TYPE_PONG)
	{
		be_version = beMsgGetDictNode(node, "v");
		if (!be_version)
		{
#ifdef DEBUG_NODE_PARSE
			LOG.info("bdTunnelNode::recvPkt() NOTE: PONG missing Optional Version.";
			LOG.info(std::endl;
#endif
		}
	}

	if (be_version)
	{
		beMsgGetToken(be_version, versionId);
	}

	/*********** handle target (query) or info_hash (get_hash) ************/
	bdNodeId target_info_hash;
	be_node  *be_target = NULL;
	if (beType == BITDHT_MSG_TYPE_FIND_NODE)
	{
		be_target = beMsgGetDictNode(be_data, "target");
		if (!be_target)
		{
#ifdef DEBUG_NODE_PARSE
			LOG.info("bdTunnelNode::recvPkt() Missing Target / Info_Hash. Dropping Msg";
			LOG.info(std::endl;
#endif
			be_free(node);
			return;
		}
	}
	else if ((beType == BITDHT_MSG_TYPE_GET_HASH) ||
			(beType == BITDHT_MSG_TYPE_POST_HASH))
	{
		be_target = beMsgGetDictNode(be_data, "info_hash");
		if (!be_target)
		{
#ifdef DEBUG_NODE_PARSE
			LOG.info("bdTunnelNode::recvPkt() Missing Target / Info_Hash. Dropping Msg";
			LOG.info(std::endl;
#endif
			be_free(node);
			return;
		}
	}

	if (be_target)
	{
		beMsgGetNodeId(be_target, target_info_hash);
	}

	/*********** handle nodes (reply_query or reply_near) *****************/
	std::list<bdId> nodes;
	be_node  *be_nodes = NULL;
	if ((beType == BITDHT_MSG_TYPE_REPLY_NODE) ||
			(beType == BITDHT_MSG_TYPE_REPLY_NEAR))
	{
		be_nodes = beMsgGetDictNode(be_data, "nodes");
		if (!be_nodes)
		{
#ifdef DEBUG_NODE_PARSE
			LOG.info("bdTunnelNode::recvPkt() Missing Nodes. Dropping Msg";
			LOG.info(std::endl;
#endif
			be_free(node);
			return;
		}
	}

	if (be_nodes)
	{
		beMsgGetListBdIds(be_nodes, nodes);
	}

	/******************* handle values (reply_hash) ***********************/
	std::list<std::string> values;
	be_node  *be_values = NULL;
	if (beType == BITDHT_MSG_TYPE_REPLY_HASH)
	{
		be_values = beMsgGetDictNode(be_data, "values");
		if (!be_values)
		{
#ifdef DEBUG_NODE_PARSE
			LOG.info("bdTunnelNode::recvPkt() Missing Values. Dropping Msg";
			LOG.info(std::endl;
#endif
			be_free(node);
			return;
		}
	}

	if (be_values)
	{
		beMsgGetListStrings(be_values, values);
	}

	/************ handle token (reply_hash, reply_near, post hash) ********/
	bdToken token;
	be_node  *be_token = NULL;
	if ((beType == BITDHT_MSG_TYPE_REPLY_HASH) ||
			(beType == BITDHT_MSG_TYPE_REPLY_NEAR) ||
			(beType == BITDHT_MSG_TYPE_POST_HASH))
	{
		be_token = beMsgGetDictNode(be_data, "token");
		if (!be_token)
		{
#ifdef DEBUG_NODE_PARSE
			LOG.info("bdTunnelNode::recvPkt() Missing Token. Dropping Msg";
			LOG.info(std::endl;
#endif
			be_free(node);
			return;
		}
	}

	if (be_token)
	{
		beMsgGetToken(be_transId, transId);
	}

	/****************** handle port (post hash) ***************************/
	uint32_t port;
	be_node  *be_port = NULL;
	if (beType == BITDHT_MSG_TYPE_POST_HASH)
	{
		be_port = beMsgGetDictNode(be_data, "port");
		if (!be_port)
		{
#ifdef DEBUG_NODE_PARSE
			LOG.info("bdTunnelNode::recvPkt() POST_HASH Missing Port. Dropping Msg";
			LOG.info(std::endl;
#endif
			be_free(node);
			return;
		}
	}

	if (be_port)
	{
		beMsgGetUInt32(be_port, &port);
	}

	/****************** handle reply newconn ***************************/

	bdId peerId;
	if (beType == BITDHT_MSG_TYPE_REPLY_NEWCONN)
	{
		be_node  *be_newconn = NULL;
		be_node  *be_pid = NULL;

		be_newconn = beMsgGetDictNode(be_data, "newconn");
		if (!be_newconn)
		{
#ifdef DEBUG_NODE_PARSE
			LOG.info("bdTunnelNode::recvPkt() REPLY_NEWCONN Missing newconn. Dropping Msg");
#endif
			be_free(node);
			return;
		}

		be_pid = beMsgGetDictNode(be_data, "pid");
		if (!be_pid)
		{
#ifdef DEBUG_NODE_PARSE
			LOG.info("bdTunnelNode::recvPkt() REPLY_NEWCONN Missing pid. Dropping Msg");
#endif
			be_free(node);
			return;
		}

		if (!beMsgGetBdId(be_pid, peerId)) {
#ifdef DEBUG_NODE_PARSE
			LOG.info("bdTunnelNode::recvPkt() REPLY_NEWCONN decode pid fail. Dropping Msg");
#endif
			be_free(node);
			return;
		}
	}

	/****************** Bits Parsed Ok. Process Msg ***********************/
	/* Construct Source Id */
	bdId srcId(id, addr);

	checkIncomingMsg(&srcId, &transId, beType);
	if (beType == BITDHT_MSG_TYPE_NEWCONN) {
		//#ifdef DEBUG_NODE_MSGS
		LOG.info("bdTunnelNode::recvPkt() NewConn from: %s",
				mFns->bdPrintId(&srcId).c_str());
		//#endif

		msgin_newconn(&srcId, &transId);
	}
	else if (beType == BITDHT_MSG_TYPE_REPLY_NEWCONN) {
		//#ifdef DEBUG_NODE_MSGS
		LOG.info("bdTunnelNode::recvPkt() Reply NewConn from: %s for: %s",
				mFns->bdPrintId(&srcId).c_str(), mFns->bdPrintId(&peerId).c_str());
		//#endif

		msgin_reply_newconn(&srcId, &transId);
	}
	else {
		LOG.info("bdTunnelNode::recvPkt() unhandled Type %d from: %s",
				beType, mFns->bdPrintId(&srcId).c_str());
	}

	be_free(node);
	return;
}

/****************** Other Functions ******************/

void bdTunnelNode::genNewToken(bdToken *token)
{
#ifdef DEBUG_NODE_ACTIONS 
	LOG.info("bdTunnelNode::genNewToken()");
#endif

	std::ostringstream out;
	out << std::setw(4) << std::setfill('0') << rand() << std::setw(4) << std::setfill('0') << rand();
	std::string num = out.str();
	int len = num.size();
	if (len > BITDHT_TOKEN_MAX_LEN)
		len = BITDHT_TOKEN_MAX_LEN;

	for(int i = 0; i < len; i++)
	{
		token->data[i] = num[i];
	}
	token->len = len;
}

extern std::atomic_ulong transIdCounter;

void bdTunnelNode::genNewTransId(bdToken *token)
{
	/* generate message, send to udp */
#ifdef DEBUG_NODE_ACTIONS 
	LOG.info("bdTunnelNode::genNewTransId()");
#endif

	std::ostringstream out;
	out << std::setw(2) << std::setfill('0') << transIdCounter++;
	std::string num = out.str();
	int len = num.size();
	if (len > BITDHT_TOKEN_MAX_LEN)
		len = BITDHT_TOKEN_MAX_LEN;

	for(int i = 0; i < len; i++)
	{
		token->data[i] = num[i];
	}
	token->len = len;
}

uint32_t bdTunnelNode::checkIncomingMsg(bdId *id, bdToken *transId, uint32_t msgType)
{

#ifdef DEBUG_MSG_CHECKS
	LOG.info("bdTunnelNode::checkIncomingMsg(%s, %d)",
			mFns->bdPrintId(id).c_str(), msgType);
#else
	(void) id;
	(void) msgType;
#endif

#ifdef USE_HISTORY
	mHistory.addMsg(id, transId, msgType, true);
#else
	(void) transId;
#endif

	return 0;
}

void bdTunnelNode::cleanupTransIdRegister()
{
	return;
}

/*************** Internal Msg Storage *****************/

bdTunnelNodeNetMsg::bdTunnelNodeNetMsg(char *msg, int len, struct sockaddr_in *in_addr)
:data(NULL), mSize(len), addr(*in_addr)
{
	data = (char *) malloc(len);
	memcpy(data, msg, len);
	//print(LOG << log4cpp::Priority::INFO);
}

void bdTunnelNodeNetMsg::print(std::ostream &out)
{
	out << "bdTunnelNodeNetMsg::print(" << mSize << ") to "
			<< inet_ntoa(addr.sin_addr) << ":" << htons(addr.sin_port);
	out << std::endl;
}


bdTunnelNodeNetMsg::~bdTunnelNodeNetMsg()
{
	free(data);
}
