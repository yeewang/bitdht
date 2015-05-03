/*
 * bitdht/bdnode.cc
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

#include "bitdht/bdnode.h"

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


#define BITDHT_QUERY_START_PEERS    10
#define BITDHT_QUERY_NEIGHBOUR_PEERS    8
#define BITDHT_MAX_REMOTE_QUERY_AGE	10

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


bdNode::bdNode(bdNodeId *ownId,
		std::string dhtVersion,
		std::string bootfile,
		bdDhtFunctions *fns,
		PacketCallback *packetCallback) :
		mOwnId(*ownId), mNodeSpace(ownId, fns), mStore(bootfile, fns),
		mDhtVersion(dhtVersion), mFns(fns), mPacketCallback(packetCallback)
{
	resetStats();
}

void bdNode::getOwnId(bdNodeId *id)
{
	*id = mOwnId;
}

/***** Startup / Shutdown ******/
void bdNode::restartNode()
{
	resetStats();

	mStore.reloadFromStore();

	/* setup */
	bdPeer peer;
	while(mStore.getPeer(&peer))
	{
		addPotentialPeer(&(peer.mPeerId));
	}
}

void bdNode::shutdownNode()
{
	/* clear the queries */
	mLocalQueries.clear();
	mRemoteQueries.clear();

	/* clear the space */
	mNodeSpace.clear();
	mHashSpace.clear();

	/* clear other stuff */
	mPotentialPeers.clear();
	mStore.clear();

	/* clean up any outgoing messages */
	while(mOutgoingMsgs.size() > 0)
	{
		bdNodeNetMsg *msg = mOutgoingMsgs.front();
		mOutgoingMsgs.pop_front();

		/* cleanup message */
		delete msg;
	}
}

/* Crappy initial store... use bdspace as answer */
void bdNode::updateStore()
{
	mStore.writeStore();
}

void bdNode::printState()
{
	LOG.info("bdNode::printState() for Peer: " +
			mFns->bdPrintNodeId(&mOwnId));
	mNodeSpace.printDHT();

	printQueries();

#ifdef USE_HISTORY
	mHistory.printMsgs();
#endif

	printStats();
}

void bdNode::printQueries()
{
	LOG.info("bdNode::printQueries() for Peer: " +
			mFns->bdPrintNodeId(&mOwnId));

	int i = 0;
	std::list<bdQuery *>::iterator it;
	for(it = mLocalQueries.begin(); it != mLocalQueries.end(); it++, i++)
	{
		LOG.info("Query #%d:", i);
		(*it)->printQuery();
	}
}

void bdNode::iterationOff()
{
	/* clean up any incoming messages */
	while(mIncomingMsgs.size() > 0)
	{
		bdNodeNetMsg *msg = mIncomingMsgs.front();
		mIncomingMsgs.pop_front();

		/* cleanup message */
		delete msg;
	}
}

void bdNode::iteration()
{
#ifdef DEBUG_NODE_MULTIPEER 
	LOG.info("bdNode::iteration() of Peer: %s",
			mFns->bdPrintNodeId(&mOwnId).c_str());
#endif
	/* iterate through queries */

	bdId id;
	bdNodeId targetNodeId;
	std::list<bdQuery>::iterator it;
	//	std::list<bdId>::iterator bit;

	/* process incoming msgs */
	while(mIncomingMsgs.size() > 0)
	{
		bdNodeNetMsg *msg = mIncomingMsgs.front();
		mIncomingMsgs.pop_front();

		recvPkt(msg->data, msg->mSize, msg->addr);

		/* cleanup message */
		delete msg;
	}

	/* assume that this is called once per second... limit the messages 
	 * in theory, a query can generate up to 10 peers (which will all require a ping!).
	 * we want to handle all the pings we can... so we don't hold up the process.
	 * but we also want enough queries to keep things moving.
	 * so allow up to 90% of messages to be pings.
	 *
	 * ignore responses to other peers... as the number is very small generally
	 */

#define BDNODE_MESSAGE_RATE_HIGH 	1
#define BDNODE_MESSAGE_RATE_MED 	2
#define BDNODE_MESSAGE_RATE_LOW 	3
#define BDNODE_MESSAGE_RATE_TRICKLE 	4

#define BDNODE_HIGH_MSG_RATE	100
#define BDNODE_MED_MSG_RATE	50
#define BDNODE_LOW_MSG_RATE	20
#define BDNODE_TRICKLE_MSG_RATE	5

	int maxMsgs = BDNODE_MED_MSG_RATE;
	int mAllowedMsgRate = BDNODE_MESSAGE_RATE_MED;

	switch(mAllowedMsgRate)
	{
	case BDNODE_MESSAGE_RATE_HIGH:
		maxMsgs = BDNODE_HIGH_MSG_RATE;
		break;

	case BDNODE_MESSAGE_RATE_MED:
		maxMsgs = BDNODE_MED_MSG_RATE;
		break;

	case BDNODE_MESSAGE_RATE_LOW:
		maxMsgs = BDNODE_LOW_MSG_RATE;
		break;

	case BDNODE_MESSAGE_RATE_TRICKLE:
		maxMsgs = BDNODE_TRICKLE_MSG_RATE;
		break;

	default:
		break;

	}

	int allowedPings = 0.9 * maxMsgs;
	int sentMsgs = 0;
	int sentPings = 0;

#if 0
	int ilim = mLocalQueries.size() * 15;
	if (ilim < 20)
	{
		ilim = 20;
	}
	if (ilim > 500)
	{
		ilim = 500;
	}
#endif

	while((mPotentialPeers.size() > 0) && (sentMsgs < allowedPings))
	{
		/* check history ... is we have pinged them already...
		 * then simulate / pretend we have received a pong,
		 * and don't bother sending another ping.
		 */

		bdId pid = mPotentialPeers.front();	
		mPotentialPeers.pop_front();


		/* don't send too many queries ... check history first */
#ifdef USE_HISTORY
		if (mHistory.validPeer(&pid))
		{
			/* just add as peer */

#ifdef DEBUG_NODE_MSGS 
			LOG.info("bdNode::iteration() Pinging Known Potential Peer : ";
			mFns->bdPrintId(LOG << log4cpp::Priority::INFO, &pid);
			LOG.info(std::endl;
#endif

		}
#endif

		/**** TEMP ****/

		{
			bdToken transId;
			genNewTransId(&transId);
			//registerOutgoingMsg(&pid, &transId, BITDHT_MSG_TYPE_PING);
			msgout_ping(&pid, &transId);

			genNewTransId(&transId);
			msgout_newconn(&pid, &transId);

			sentMsgs++;
			sentPings++;

#ifdef DEBUG_NODE_MSGS 
			LOG.info("bdNode::iteration() Pinging Potential Peer : %s",
					mFns->bdPrintId(&pid).c_str());
#endif

			mCounterPings++;
		}
	}

	/* allow each query to send up to one query... until maxMsgs has been reached */
	int numQueries = mLocalQueries.size();
	int sentQueries = 0;
	int i = 0;
	while((i < numQueries) && (sentMsgs < maxMsgs))
	{
		bdQuery *query = mLocalQueries.front();
		mLocalQueries.pop_front();
		mLocalQueries.push_back(query);

		/* go through the possible queries */
		if (query->nextQuery(id, targetNodeId))
		{
			/* push out query */
			bdToken transId;
			genNewTransId(&transId);
			//registerOutgoingMsg(&id, &transId, BITDHT_MSG_TYPE_FIND_NODE);

			msgout_find_node(&id, &transId, &targetNodeId);

			// invoke callback for this bdId
			// mPacketCallback->onRecvCallback();


#ifdef DEBUG_NODE_MSGS 
			LOG.info("bdNode::iteration() Find Node Req for : %s searching for : %s",
					mFns->bdPrintId(&id).c_str(),
					mFns->bdPrintNodeId(&targetNodeId).c_str());
#endif
			mCounterQueryNode++;
			sentMsgs++;
			sentQueries++;
		}
		i++;
	}

#ifdef DEBUG_NODE_ACTIONS 
	LOG.info("bdNode::iteration() maxMsgs: %d sentPings: %d /  sentQueries: %d / %d",
			maxMsgs, sentPings, allowedPings, sentQueries, numQueries);
#endif

	/* process remote query too */
	processRemoteQuery();

	while(mNodeSpace.out_of_date_peer(id))
	{
		/* push out ping */
		bdToken transId;
		genNewTransId(&transId);
		//registerOutgoingMsg(&id, &transId, BITDHT_MSG_TYPE_PING);
		msgout_ping(&id, &transId);

#ifdef DEBUG_NODE_MSGS 
		LOG.info("bdNode::iteration() Pinging Out-Of-Date Peer: %s",
				mFns->bdPrintId(&id).c_str());
#endif

		mCounterOutOfDatePing++;

		//registerOutgoingMsg(&id, &transId, BITDHT_MSG_TYPE_FIND_NODE);
		//msgout_find_node(&id, &transId, &(id.id));
	}

	doStats();

	//printStats(LOG << log4cpp::Priority::INFO);

	//printQueries();
}

#define LPF_FACTOR  (0.90)

void bdNode::doStats()
{
	mLpfOutOfDatePing *= (LPF_FACTOR) ;
	mLpfOutOfDatePing += (1.0 - LPF_FACTOR) * mCounterOutOfDatePing;	
	mLpfPings *= (LPF_FACTOR);  	
	mLpfPings += (1.0 - LPF_FACTOR) * mCounterPings;	
	mLpfPongs *= (LPF_FACTOR);  	
	mLpfPongs += (1.0 - LPF_FACTOR) * mCounterPongs;	
	mLpfQueryNode *= (LPF_FACTOR);  	
	mLpfQueryNode += (1.0 - LPF_FACTOR) * mCounterQueryNode;	
	mLpfQueryHash *= (LPF_FACTOR);  	
	mLpfQueryHash += (1.0 - LPF_FACTOR) * mCounterQueryHash;	
	mLpfReplyFindNode *= (LPF_FACTOR);  	
	mLpfReplyFindNode += (1.0 - LPF_FACTOR) * mCounterReplyFindNode;	
	mLpfReplyQueryHash *= (LPF_FACTOR);  	
	mLpfReplyQueryHash += (1.0 - LPF_FACTOR) * mCounterReplyQueryHash;	

	mLpfRecvPing *= (LPF_FACTOR);  	
	mLpfRecvPing += (1.0 - LPF_FACTOR) * mCounterRecvPing;	
	mLpfRecvPong *= (LPF_FACTOR);  	
	mLpfRecvPong += (1.0 - LPF_FACTOR) * mCounterRecvPong;	
	mLpfRecvQueryNode *= (LPF_FACTOR);  	
	mLpfRecvQueryNode += (1.0 - LPF_FACTOR) * mCounterRecvQueryNode;	
	mLpfRecvQueryHash *= (LPF_FACTOR);  	
	mLpfRecvQueryHash += (1.0 - LPF_FACTOR) * mCounterRecvQueryHash;	
	mLpfRecvReplyFindNode *= (LPF_FACTOR);  	
	mLpfRecvReplyFindNode += (1.0 - LPF_FACTOR) * mCounterRecvReplyFindNode;	
	mLpfRecvReplyQueryHash *= (LPF_FACTOR);  	
	mLpfRecvReplyQueryHash += (1.0 - LPF_FACTOR) * mCounterRecvReplyQueryHash;	

	resetCounters();
}

void bdNode::printStats()
{
	LOG.info("bdNode::printStats()");
	LOG.info("  Send                                                 Recv:");
	LOG.info("  mLpfOutOfDatePing      : %10lf", mLpfOutOfDatePing);
	LOG.info("  mLpfPings              : %10lf  mLpfRecvPongs          : %10lf", mLpfPings, mLpfRecvPong);
	LOG.info("  mLpfPongs              : %10lf  mLpfRecvPings          : %10lf", mLpfPongs, mLpfRecvPing);
	LOG.info("  mLpfQueryNode          : %10lf  mLpfRecvReplyFindNode  : %10lf", mLpfQueryNode, mLpfRecvReplyFindNode);
	LOG.info("  mLpfQueryHash          : %10lf  mLpfRecvReplyQueryHash : %10lf", mLpfQueryHash, mLpfRecvReplyQueryHash);
	LOG.info("  mLpfReplyFindNode      : %10lf  mLpfRecvQueryNode      : %10lf", mLpfReplyFindNode, mLpfRecvQueryNode);
	LOG.info("  mLpfReplyQueryHash/sec : %10lf  mLpfRecvQueryHash/sec  : %10lf", mLpfReplyQueryHash, mLpfRecvQueryHash);
}

void bdNode::resetCounters()
{
	mCounterOutOfDatePing = 0;
	mCounterPings = 0;
	mCounterPongs = 0;
	mCounterQueryNode = 0;
	mCounterQueryHash = 0;
	mCounterReplyFindNode = 0;
	mCounterReplyQueryHash = 0;

	mCounterRecvPing = 0;
	mCounterRecvPong = 0;
	mCounterRecvQueryNode = 0;
	mCounterRecvQueryHash = 0;
	mCounterRecvReplyFindNode = 0;
	mCounterRecvReplyQueryHash = 0;
}

void bdNode::resetStats()
{
	mLpfOutOfDatePing = 0;
	mLpfPings = 0;
	mLpfPongs = 0;
	mLpfQueryNode = 0;
	mLpfQueryHash = 0;
	mLpfReplyFindNode = 0;
	mLpfReplyQueryHash = 0;

	mLpfRecvPing = 0;
	mLpfRecvPong = 0;
	mLpfRecvQueryNode = 0;
	mLpfRecvQueryHash = 0;
	mLpfRecvReplyFindNode = 0;
	mLpfRecvReplyQueryHash = 0;

	resetCounters();
}


void bdNode::checkPotentialPeer(bdId *id)
{
	bool isWorthyPeer = false;
	/* also push to queries */
	std::list<bdQuery *>::iterator it;
	for(it = mLocalQueries.begin(); it != mLocalQueries.end(); it++)
	{
		if ((*it)->addPotentialPeer(id, 0))
		{
			isWorthyPeer = true;
		}
	}

	if (isWorthyPeer)
	{
		addPotentialPeer(id);
	}
}


void bdNode::addPotentialPeer(bdId *id)
{
	mPotentialPeers.push_back(*id);
}



// virtual so manager can do callback.
// peer flags defined in bdiface.h
void bdNode::addPeer(const bdId *id, uint32_t peerflags)
{
#ifdef DEBUG_NODE_ACTIONS 
	LOG.info("bdNode::addPeer(%s)",
			mFns->bdPrintId(id).c_str());
#endif

	/* iterate through queries */
	std::list<bdQuery *>::iterator it;
	for(it = mLocalQueries.begin(); it != mLocalQueries.end(); it++)
	{
		(*it)->addPeer(id, peerflags);
	}

	mNodeSpace.add_peer(id, peerflags);

	bdPeer peer;
	peer.mPeerId = *id;
	peer.mPeerFlags = peerflags;
	peer.mLastRecvTime = time(NULL);
	mStore.addStore(&peer);
}


#if 0
// virtual so manager can do callback.
// peer flags defined in bdiface.h
void bdNode::PeerResponse(const bdId *id, const bdNodeId *target, uint32_t peerflags)
{

#ifdef DEBUG_NODE_ACTIONS 
	LOG.info("bdNode::PeerResponse(";
	mFns->bdPrintId(LOG << log4cpp::Priority::INFO, id);
	LOG.info(", target: ";
	mFns->bdPrintNodeId(LOG << log4cpp::Priority::INFO, target);
	LOG.info(")\n");
#endif

	/* iterate through queries */
	std::list<bdQuery>::iterator it;
	for(it = mLocalQueries.begin(); it != mLocalQueries.end(); it++)
	{
		it->PeerResponse(id, target, peerflags);
	}

	mNodeSpace.add_peer(id, peerflags);

	bdPeer peer;
	peer.mPeerId = *id;
	peer.mPeerFlags = peerflags;
	peer.mLastRecvTime = time(NULL);
	mStore.addStore(&peer);
}

#endif

/************************************ Query Details        *************************/
void bdNode::addQuery(const bdNodeId *id, uint32_t qflags)
{

	std::list<bdId> startList;
	std::multimap<bdMetric, bdId> nearest;
	std::multimap<bdMetric, bdId>::iterator it;

	mNodeSpace.find_nearest_nodes(id, BITDHT_QUERY_START_PEERS, startList, nearest);

	LOG.info("bdNode::addQuery(" + mFns->bdPrintNodeId(id) + ")\n");

	for(it = nearest.begin(); it != nearest.end(); it++)
	{
		startList.push_back(it->second);
	}

	bdQuery *query = new bdQuery(id, startList, qflags, mFns);
	mLocalQueries.push_back(query);
}


void bdNode::clearQuery(const bdNodeId *rmId)
{
	std::list<bdQuery *>::iterator it;
	for(it = mLocalQueries.begin(); it != mLocalQueries.end();)
	{
		if ((*it)->mId == *rmId)
		{
			bdQuery *query = (*it);
			it = mLocalQueries.erase(it);
			delete query;
		}
		else
		{
			it++;
		}
	}
}

void bdNode::QueryStatus(std::map<bdNodeId, bdQueryStatus> &statusMap)
{
	std::list<bdQuery *>::iterator it;
	for(it = mLocalQueries.begin(); it != mLocalQueries.end(); it++)
	{
		bdQueryStatus status;
		status.mStatus = (*it)->mState;
		status.mQFlags = (*it)->mQueryFlags;
		(*it)->result(status.mResults);
		statusMap[(*it)->mId] = status;
	}
}

bool bdNode::getIdFromQuery(const bdNodeId *id, std::list<bdPeer> &idList)
{
#ifdef DEBUG_NODE_MSGS
	LOG.info("bdNode::getIdFromQuery(" + mFns->bdPrintNodeId(id) + ")\n");
#endif

	int i = 0;
	std::list<bdQuery *>::iterator it;
	for(it = mLocalQueries.begin(); it != mLocalQueries.end(); it++, i++)
	{
		if ((*it)->mId == *id) {
			(*it)->matchResult(idList);
			return true;
		}
	}

	return false;
}

/************************************ Process Remote Query *************************/
void bdNode::processRemoteQuery()
{
	bool processed = false;
	time_t oldTS = time(NULL) - BITDHT_MAX_REMOTE_QUERY_AGE;
	while(!processed)
	{
		/* extra exit clause */
		if (mRemoteQueries.size() < 1) return;

		bdRemoteQuery &query = mRemoteQueries.front();

		/* discard older ones (stops queue getting overloaded) */
		if (query.mQueryTS > oldTS)
		{
			/* recent enough to process! */
			processed = true;

			switch(query.mQueryType)
			{
			case BD_QUERY_NEIGHBOURS:
			{
				/* search bdSpace for neighbours */
				std::list<bdId> excludeList;
				std::list<bdId> nearList;
				std::multimap<bdMetric, bdId> nearest;
				std::multimap<bdMetric, bdId>::iterator it;

				mNodeSpace.find_nearest_nodes(&(query.mQuery), BITDHT_QUERY_NEIGHBOUR_PEERS, excludeList, nearest);

				for(it = nearest.begin(); it != nearest.end(); it++)
				{
					nearList.push_back(it->second);
				}
				msgout_reply_find_node(&(query.mId), &(query.mTransId), nearList);
#ifdef DEBUG_NODE_MSGS 
				LOG.info("bdNode::processRemoteQuery() Reply to Find Node: %s searching for : %s , found %d nodes ",
						mFns->bdPrintId(&(query.mId)).c_str(),
						mFns->bdPrintNodeId(&(query.mQuery)).c_str(), nearest.size());
#endif

				mCounterReplyFindNode++;

				break;
			}
			case BD_QUERY_HASH:
			{
#ifdef DEBUG_NODE_MSGS 
				LOG.info("bdNode::processRemoteQuery() Reply to Query Node: %s TODO",
						mFns->bdPrintId(&(query.mId)).c_str());
#endif
				mCounterReplyQueryHash++;


				/* TODO */
				break;
			}
			default:
			{
				/* drop */
				/* unprocess! */
				processed = false;
				break;
			}
			}



		}
		else
		{
#ifdef DEBUG_NODE_MSGS 
			LOG.info("bdNode::processRemoteQuery() Query Too Old: Discarding: %s",
					mFns->bdPrintId(&(query.mId)).c_str());
#endif
		}


		mRemoteQueries.pop_front();
	}
}

/************************************ Message Buffering ****************************/

/* interaction with outside world */
int bdNode::outgoingMsg(struct sockaddr_in *addr, char *msg, int *len)
{
	if (mOutgoingMsgs.size() > 0)
	{
		bdNodeNetMsg *bdmsg = mOutgoingMsgs.front();
		//bdmsg->print(LOG << log4cpp::Priority::INFO);
		mOutgoingMsgs.pop_front();
		//bdmsg->print(LOG << log4cpp::Priority::INFO);

		/* truncate if necessary */
		if (bdmsg->mSize < *len)
		{
			//LOG.info("bdNode::outgoingMsg space(" << *len << ") msgsize(" << bdmsg->mSize << ")";
			//LOG.info(std::endl;
			*len = bdmsg->mSize;
		}
		else
		{
			//LOG.info("bdNode::outgoingMsg space(" << *len << ") small - trunc from: "
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

void bdNode::incomingMsg(struct sockaddr_in *addr, char *msg, int len)
{
	bdNodeNetMsg *bdmsg = new bdNodeNetMsg(msg, len, addr);
	mIncomingMsgs.push_back(bdmsg);
}

/************************************ Message Handling *****************************/

/* Outgoing Messages */

void bdNode::msgout_ping(bdId *id, bdToken *transId)
{
#ifdef DEBUG_NODE_MSGOUT
	LOG.info("bdNode::msgout_ping() TransId: ";
	bdPrintTransId(LOG << log4cpp::Priority::INFO, transId);
	LOG.info(" To: ";
	mFns->bdPrintId(LOG << log4cpp::Priority::INFO, id);
	LOG.info(std::endl;
#endif

	registerOutgoingMsg(id, transId, BITDHT_MSG_TYPE_PING);


	/* create string */
	char msg[10240];
	int avail = 10240;

	int blen = bitdht_create_ping_msg(transId, &(mOwnId), msg, avail-1);
	sendPkt(msg, blen, id->addr);
}


void bdNode::msgout_pong(bdId *id, bdToken *transId)
{
#ifdef DEBUG_NODE_MSGOUT
	LOG.info("bdNode::msgout_pong() TransId: ";
	bdPrintTransId(LOG << log4cpp::Priority::INFO, transId);
	LOG.info(" Version: " << version;
	LOG.info(" To: ";
	mFns->bdPrintId(LOG << log4cpp::Priority::INFO, id);
	LOG.info(std::endl;
#endif

	registerOutgoingMsg(id, transId, BITDHT_MSG_TYPE_PONG);

	/* generate message, send to udp */
	bdToken vid;
	uint32_t vlen = BITDHT_TOKEN_MAX_LEN;
	if (mDhtVersion.size() < vlen)
	{
		vlen = mDhtVersion.size();
	}
	memcpy(vid.data, mDhtVersion.c_str(), vlen);	
	vid.len = vlen;

	char msg[10240];
	int avail = 10240;

	int blen = bitdht_response_ping_msg(transId, &(mOwnId), &vid, msg, avail-1);

	sendPkt(msg, blen, id->addr);

}


void bdNode::msgout_find_node(bdId *id, bdToken *transId, bdNodeId *query)
{
#ifdef DEBUG_NODE_MSGOUT
	LOG.info("bdNode::msgout_find_node() TransId: ";
	bdPrintTransId(LOG << log4cpp::Priority::INFO, transId);
	LOG.info(" To: ";
	mFns->bdPrintId(LOG << log4cpp::Priority::INFO, id);
	LOG.info(" Query: ";
	mFns->bdPrintNodeId(LOG << log4cpp::Priority::INFO, query);
	LOG.info(std::endl;
#endif

	registerOutgoingMsg(id, transId, BITDHT_MSG_TYPE_FIND_NODE);

	char msg[10240];
	int avail = 10240;

	int blen = bitdht_find_node_msg(transId, &(mOwnId), query, msg, avail-1);

	sendPkt(msg, blen, id->addr);
}

void bdNode::msgout_reply_find_node(bdId *id, bdToken *transId, std::list<bdId> &peers)
{
	char msg[10240];
	int avail = 10240;

	registerOutgoingMsg(id, transId, BITDHT_MSG_TYPE_REPLY_NODE);


	int blen = bitdht_resp_node_msg(transId, &(mOwnId), peers, msg, avail-1);

	sendPkt(msg, blen, id->addr);

#ifdef DEBUG_NODE_MSGOUT
	LOG.info("bdNode::msgout_reply_find_node() TransId: ";
	bdPrintTransId(LOG << log4cpp::Priority::INFO, transId);
	LOG.info(" To: ";
	mFns->bdPrintId(LOG << log4cpp::Priority::INFO, id);
	LOG.info(" Peers:";
	std::list<bdId>::iterator it;
	for(it = peers.begin(); it != peers.end(); it++)
	{
		LOG.info(" ";
		mFns->bdPrintId(LOG << log4cpp::Priority::INFO, &(*it));
	}
	LOG.info(std::endl;
#endif
}

/*****************
 * SECOND HALF
 *
 *****/

void bdNode::msgout_get_hash(bdId *id, bdToken *transId, bdNodeId *info_hash)
{
#ifdef DEBUG_NODE_MSGOUT
	LOG.info("bdNode::msgout_get_hash() TransId: ";
	bdPrintTransId(LOG << log4cpp::Priority::INFO, transId);
	LOG.info(" To: ";
	mFns->bdPrintId(LOG << log4cpp::Priority::INFO, id);
	LOG.info(" InfoHash: ";
	mFns->bdPrintNodeId(LOG << log4cpp::Priority::INFO, info_hash);
	LOG.info(std::endl;
#endif

	char msg[10240];
	int avail = 10240;

	registerOutgoingMsg(id, transId, BITDHT_MSG_TYPE_GET_HASH);


	int blen = bitdht_get_peers_msg(transId, &(mOwnId), info_hash, msg, avail-1);

	sendPkt(msg, blen, id->addr);
}

void bdNode::msgout_reply_hash(bdId *id, bdToken *transId, bdToken *token, std::list<std::string> &values)
{
#ifdef DEBUG_NODE_MSGOUT
	LOG.info("bdNode::msgout_reply_hash() TransId: ";
	bdPrintTransId(LOG << log4cpp::Priority::INFO, transId);
	LOG.info(" To: ";
	mFns->bdPrintId(LOG << log4cpp::Priority::INFO, id);
	LOG.info(" Token: ";
	bdPrintToken(LOG << log4cpp::Priority::INFO, token);

	LOG.info(" Peers: ";
	std::list<std::string>::iterator it;
	for(it = values.begin(); it != values.end(); it++)
	{
		LOG.info(" ";
		bdPrintCompactPeerId(LOG << log4cpp::Priority::INFO, *it);
	}
	LOG.info(std::endl;
#endif

	char msg[10240];
	int avail = 10240;

	registerOutgoingMsg(id, transId, BITDHT_MSG_TYPE_REPLY_HASH);

	int blen = bitdht_peers_reply_hash_msg(transId, &(mOwnId), token, values, msg, avail-1);

	sendPkt(msg, blen, id->addr);
}

void bdNode::msgout_reply_nearest(bdId *id, bdToken *transId, bdToken *token, std::list<bdId> &nodes)
{
#ifdef DEBUG_NODE_MSGOUT
	LOG.info("bdNode::msgout_reply_nearest() TransId: ";
	bdPrintTransId(LOG << log4cpp::Priority::INFO, transId);
	LOG.info(" To: ";
	mFns->bdPrintId(LOG << log4cpp::Priority::INFO, id);
	LOG.info(" Token: ";
	bdPrintToken(LOG << log4cpp::Priority::INFO, token);
	LOG.info(" Nodes:";

	std::list<bdId>::iterator it;
	for(it = nodes.begin(); it != nodes.end(); it++)
	{
		LOG.info(" ";
		mFns->bdPrintId(LOG << log4cpp::Priority::INFO, &(*it));
	}
	LOG.info(std::endl;
#endif

	char msg[10240];
	int avail = 10240;

	registerOutgoingMsg(id, transId, BITDHT_MSG_TYPE_REPLY_NEAR);

	int blen = bitdht_peers_reply_closest_msg(transId, &(mOwnId), token, nodes, msg, avail-1);

	sendPkt(msg, blen, id->addr);
}

void bdNode::msgout_post_hash(bdId *id, bdToken *transId, bdNodeId *info_hash, uint32_t port, bdToken *token)
{
#ifdef DEBUG_NODE_MSGOUT
	LOG.info("bdNode::msgout_post_hash() TransId: ";
	bdPrintTransId(LOG << log4cpp::Priority::INFO, transId);
	LOG.info(" To: ";
	mFns->bdPrintId(LOG << log4cpp::Priority::INFO, id);
	LOG.info(" Info_Hash: ";
	mFns->bdPrintNodeId(LOG << log4cpp::Priority::INFO, info_hash);
	LOG.info(" Port: " << port;
	LOG.info(" Token: ";
	bdPrintToken(LOG << log4cpp::Priority::INFO, token);
	LOG.info(std::endl;
#endif

	char msg[10240];
	int avail = 10240;

	registerOutgoingMsg(id, transId, BITDHT_MSG_TYPE_POST_HASH);


	int blen = bitdht_announce_peers_msg(transId,&(mOwnId),info_hash,port,token,msg,avail-1);

	sendPkt(msg, blen, id->addr);

}

void bdNode::msgout_reply_post(bdId *id, bdToken *transId)
{
#ifdef DEBUG_NODE_MSGOUT
	LOG.info("bdNode::msgout_reply_post() TransId: ";
	bdPrintTransId(LOG << log4cpp::Priority::INFO, transId);
	LOG.info(" To: ";
	mFns->bdPrintId(LOG << log4cpp::Priority::INFO, id);
	LOG.info(std::endl;
#endif

	/* generate message, send to udp */
	char msg[10240];
	int avail = 10240;

	registerOutgoingMsg(id, transId, BITDHT_MSG_TYPE_REPLY_POST);

	int blen = bitdht_reply_announce_msg(transId, &(mOwnId), msg, avail-1);

	sendPkt(msg, blen, id->addr);
}

void bdNode::msgout_newconn(bdId *dhtId, bdToken *transId)
{
	// #ifdef DEBUG_NODE_MSGOUT
	std::ostringstream ss;
	bdPrintTransId(ss, transId);
	LOG.info("bdTunnelNode::msgout_newconn() TransId: %s To: %s",
			ss.str().c_str(), mFns->bdPrintId(dhtId).c_str());
	// #endif

	/* create string */
	char msg[10240];
	int avail = 10240;

	int blen = bitdht_new_conn_msg(transId, &(mOwnId), msg, avail-1);
	sendPkt(msg, blen, dhtId->addr);
}

void bdNode::msgout_reply_newconn(bdId *tunnelId, bdId *dhtId, bdToken *transId)
{
	// #ifdef DEBUG_NODE_MSGOUT
	std::ostringstream ss;
	bdPrintTransId(ss, transId);
	LOG.info("bdNode::msgout_reply_newconn() TransId: %s To: %s",
			ss.str().c_str(), mFns->bdPrintId(dhtId).c_str());
	// #endif

	registerOutgoingMsg(dhtId, transId, BITDHT_MSG_TYPE_NEWCONN);

	/* create string */
	char msg[10240];
	int avail = 10240;

	int blen = bitdht_reply_new_conn_msg(transId, &(mOwnId), msg, true, avail-1);
	sendPkt(msg, blen, dhtId->addr);
}

void bdNode::sendPkt(char *msg, int len, struct sockaddr_in addr)
{
	//LOG.info("bdNode::sendPkt(%d) to %s:%d\n", 
	//		len, inet_ntoa(addr.sin_addr), htons(addr.sin_port));

	bdNodeNetMsg *bdmsg = new bdNodeNetMsg(msg, len, &addr);
	//bdmsg->print(LOG << log4cpp::Priority::INFO);
	mOutgoingMsgs.push_back(bdmsg);
	//bdmsg->print(LOG << log4cpp::Priority::INFO);

	return;
}

/********************* Incoming Messages *************************/
/*
 * These functions are holding up udp queue -> so quickly
 * parse message, and get on with it!
 */

void bdNode::recvPkt(char *msg, int len, struct sockaddr_in addr)
{
#ifdef DEBUG_NODE_PARSE
	std::ostringstream ss;
	char buf[100];
	snprintf(buf, sizeof(buf) - 1, "bdNode::recvPkt() msg[%d] = ", len);
	ss << buf;
	for(int i = 0; i < len; i++)
	{
		if ((msg[i] > 31) && (msg[i] < 127))
		{
			ss << (char)msg[i];
		}
		else
		{
			ss << "[" << (int) msg[i] << "]";
		}
	}
	LOG.info(ss.str().c_str());
#endif

	/* convert to a be_node */
	be_node *node = be_decoden(msg, len);
	if (!node)
	{LOG.info("bdNode::recvPkt() Failure to decode. Dropping Msg");
		/* invalid decode */
#ifdef DEBUG_NODE_PARSE
		LOG.info("bdNode::recvPkt() Failure to decode. Dropping Msg");
		LOG.info("message length: %d", len);
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
	bool     beQuery = (BE_Y_Q == beMsgGetY(node));

	if (!beType)
	{
#ifdef DEBUG_NODE_PARSE
		LOG.info("bdNode::recvPkt() Invalid Message Type. Dropping Msg";
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
		LOG.info("bdNode::recvPkt() TransId Failure. Dropping Msg";
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
		LOG.info("bdNode::recvPkt() Missing Data Body. Dropping Msg";
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
		LOG.info("bdNode::recvPkt() Missing Peer Id. Dropping Msg";
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
			LOG.info("bdNode::recvPkt() NOTE: PONG missing Optional Version.";
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
			LOG.info("bdNode::recvPkt() Missing Target / Info_Hash. Dropping Msg";
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
			LOG.info("bdNode::recvPkt() Missing Target / Info_Hash. Dropping Msg";
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
			LOG.info("bdNode::recvPkt() Missing Nodes. Dropping Msg";
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
			LOG.info("bdNode::recvPkt() Missing Values. Dropping Msg";
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
			LOG.info("bdNode::recvPkt() Missing Token. Dropping Msg";
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
			LOG.info("bdNode::recvPkt() POST_HASH Missing Port. Dropping Msg";
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

	/****************** Bits Parsed Ok. Process Msg ***********************/
	/* Construct Source Id */
	bdId srcId(id, addr);

	checkIncomingMsg(&srcId, &transId, beType);
	switch(beType)
	{
	case BITDHT_MSG_TYPE_PING:  /* a: id, transId */
	{
#ifdef DEBUG_NODE_MSGS 
		LOG.info("bdNode::recvPkt() Responding to Ping : %s",
				mFns->bdPrintId(&srcId).c_str());
#endif
		msgin_ping(&srcId, &transId);
		break;
	}
	case BITDHT_MSG_TYPE_PONG:  /* r: id, transId */
	{
#ifdef DEBUG_NODE_MSGS 
		LOG.info("bdNode::recvPkt() Received Pong from : %s",
				mFns->bdPrintId(&srcId).c_str());
#endif
		if (be_version)
		{
			msgin_pong(&srcId, &transId, &versionId);
		}
		else
		{
			msgin_pong(&srcId, &transId, NULL);
		}

		break;
	}
	case BITDHT_MSG_TYPE_FIND_NODE: /* a: id, transId, target */
	{
#ifdef DEBUG_NODE_MSGS 
		LOG.info("bdNode::recvPkt() Req Find Node from : %s Looking for: %s",
				mFns->bdPrintId(&srcId).c_str(),
				mFns->bdPrintNodeId(&target_info_hash).c_str());
#endif
		msgin_find_node(&srcId, &transId, &target_info_hash);
		break;
	}
	case BITDHT_MSG_TYPE_REPLY_NODE: /* r: id, transId, nodes  */
	{
#ifdef DEBUG_NODE_MSGS 
		LOG.info("bdNode::recvPkt() Received Reply Node from : %s",
				mFns->bdPrintId(&srcId).c_str());
#endif
		msgin_reply_find_node(&srcId, &transId, nodes);
		break;
	}
	case BITDHT_MSG_TYPE_GET_HASH:    /* a: id, transId, info_hash */
	{
#ifdef DEBUG_NODE_MSGS 
		LOG.info("bdNode::recvPkt() Received SearchHash : %s for Hash: ",
				mFns->bdPrintId(&srcId).c_str(), mFns->bdPrintNodeId(&target_info_hash).c_str());
#endif
		msgin_get_hash(&srcId, &transId, &target_info_hash);
		break;
	}
	case BITDHT_MSG_TYPE_REPLY_HASH:  /* r: id, transId, token, values */
	{
#ifdef DEBUG_NODE_MSGS 
		LOG.info("bdNode::recvPkt() Received Reply Hash : %s",
				mFns->bdPrintId(&srcId).c_str());
#endif
		msgin_reply_hash(&srcId, &transId, &token, values);
		break;
	}
	case BITDHT_MSG_TYPE_REPLY_NEAR:  /* r: id, transId, token, nodes */
	{
#ifdef DEBUG_NODE_MSGS 
		LOG.info("bdNode::recvPkt() Received Reply Near : %s",
				mFns->bdPrintId(&srcId).c_str());
#endif
		msgin_reply_nearest(&srcId, &transId, &token, nodes);
		break;
	}
	case BITDHT_MSG_TYPE_POST_HASH:   /* a: id, transId, info_hash, port, token */
	{
#ifdef DEBUG_NODE_MSGS 
		LOG.info("bdNode::recvPkt() Post Hash from : %s to post: %s with port: %d",
				mFns->bdPrintId(&srcId).c_str(), mFns->bdPrintNodeId(&target_info_hash).c_str(), port);
#endif
		msgin_post_hash(&srcId, &transId, &target_info_hash, port, &token);
		break;
	}
	case BITDHT_MSG_TYPE_REPLY_POST:  /* r: id, transId */
	{
#ifdef DEBUG_NODE_MSGS 
		LOG.info("bdNode::recvPkt() Reply Post from: %s",
				mFns->bdPrintId(&srcId).c_str());
#endif
		msgin_reply_post(&srcId, &transId);
		break;
	}
	case BITDHT_MSG_TYPE_NEWCONN: {
//#ifdef DEBUG_NODE_MSGS
		LOG.info("bdNode::recvPkt() NewConn from: %s",
				mFns->bdPrintId(&srcId).c_str());
//#endif

		// it have not a dhtId!!!
		msgin_newconn(&srcId, &srcId, &transId);
		/*
		std::list<bdPeer> list;
		if (getIdFromQuery(&id, list)) {
			std::list<bdPeer>::iterator it;
			for(it = list.begin(); it != list.end(); it++) {
				msgin_newconn(&srcId, &it->mPeerId, &transId);
			}
		}
		*/
		break;
	}

	case BITDHT_MSG_TYPE_REPLY_NEWCONN: {
//#ifdef DEBUG_NODE_MSGS
		LOG.info("bdNode::recvPkt() Reply NewConn from: %s",
				mFns->bdPrintId(&srcId).c_str());
//#endif

		std::list<bdPeer> list;
		if (getIdFromQuery(&id, list)) {
			std::list<bdPeer>::iterator it;
			for(it = list.begin(); it != list.end(); it++) {
				msgin_reply_newconn(&srcId,  &it->mPeerId, &transId);
			}
		}
		break;
	}

	default:
	{
#ifdef DEBUG_NODE_MSGS 
		LOG.info("bdNode::recvPkt() ERROR");
#endif
		/* ERROR */
		break;
	}
	}

	be_free(node);
	return;
}

/* Input: id, token.
 * Response: pong(id, token)
 */

void bdNode::msgin_ping(bdId *id, bdToken *transId)
{
#ifdef DEBUG_NODE_MSGIN
	LOG.info("bdNode::msgin_ping() TransId: %s",
			bdPrintTransId(LOG << log4cpp::Priority::INFO, transId);
	LOG.info(" To: ";
	mFns->bdPrintId(LOG << log4cpp::Priority::INFO, id);
	LOG.info(std::endl;
#endif
	mCounterRecvPing++;
	mCounterPongs++;

	/* peer is alive */
	uint32_t peerflags = 0; /* no id typically, so cant get version */
	addPeer(id, peerflags);

	/* reply */
	msgout_pong(id, transId);
}

/* Input: id, token, (+optional version)
 * Response: store peer.
 */

void bdNode::msgin_pong(bdId *id, bdToken *transId, bdToken *versionId)
{
#ifdef DEBUG_NODE_MSGIN
	LOG.info("bdNode::msgin_pong() TransId: ";
	bdPrintTransId(transId);
	LOG.info(" Version: TODO!"; // << version;
	LOG.info(" To: ";
	mFns->bdPrintId(LOG << log4cpp::Priority::INFO, id);
	LOG.info(std::endl;
#else
	(void) transId;
#endif

	mCounterRecvPong++;
	/* recv pong, and peer is alive. add to DHT */
	//uint32_t vId = 0; // TODO XXX convertBdVersionToVID(versionId);

	/* calculate version match with peer */
	bool sameDhtEngine = false;
	bool sameAppl = false;
	bool sameVersion = false;

	if (versionId)
	{

#ifdef DEBUG_NODE_MSGIN
		LOG.info("bdNode::msgin_pong() Peer Version: ";
		for(int i = 0; i < versionId->len; i++)
		{
			LOG.info(versionId->data[i];
		}
		LOG.info(std::endl;
#endif

		/* check two bytes */
		if ((versionId->len >= 2) && (mDhtVersion.size() >= 2) &&
				(versionId->data[0] == mDhtVersion[0]) && (versionId->data[1] == mDhtVersion[1]))
		{
			sameDhtEngine = true;
		}

		/* check two bytes */
		if ((versionId->len >= 4) && (mDhtVersion.size() >= 4) &&
				(versionId->data[2] == mDhtVersion[2]) && (versionId->data[3] == mDhtVersion[3]))
		{
			sameAppl = true;
		}

		/* check two bytes */
		if ((versionId->len >= 6) && (mDhtVersion.size() >= 6) &&
				(versionId->data[4] == mDhtVersion[4]) && (versionId->data[5] == mDhtVersion[5]))
		{
			sameVersion = true;
		}
	}
	else
	{

#ifdef DEBUG_NODE_MSGIN
		LOG.info("bdNode::msgin_pong() No Version");
#endif
	}

	uint32_t peerflags = BITDHT_PEER_STATUS_RECV_PONG; /* should have id too */
	if (sameDhtEngine)
	{
		peerflags |= BITDHT_PEER_STATUS_DHT_ENGINE; 
	}
	if (sameAppl)
	{
		peerflags |= BITDHT_PEER_STATUS_DHT_APPL; 
	}
	if (sameVersion)
	{
		peerflags |= BITDHT_PEER_STATUS_DHT_VERSION; 
	}

	addPeer(id, peerflags);
}

/* Input: id, token, queryId */

void bdNode::msgin_find_node(bdId *id, bdToken *transId, bdNodeId *query)
{
#ifdef DEBUG_NODE_MSGIN
	LOG.info("bdNode::msgin_find_node() TransId: ";
	bdPrintTransId(LOG << log4cpp::Priority::INFO, transId);
	LOG.info(" From: ";
	mFns->bdPrintId(LOG << log4cpp::Priority::INFO, id);
	LOG.info(" Query: ";
	mFns->bdPrintNodeId(LOG << log4cpp::Priority::INFO, query);
	LOG.info(std::endl;
#endif

	mCounterRecvQueryNode++;

	/* store query... */
	queueQuery(id, query, transId, BD_QUERY_NEIGHBOURS);


	uint32_t peerflags = 0; /* no id, and no help! */
	addPeer(id, peerflags);
}

void bdNode::msgin_reply_find_node(bdId *id, bdToken *transId, std::list<bdId> &nodes)
{
	std::list<bdId>::iterator it;

#ifdef DEBUG_NODE_MSGS
	std::ostringstream debug, out;
	char buf[400];

	bdPrintTransId(out, transId);
	snprintf(buf, sizeof(buf), "bdNode::msgin_reply_find_node() TransId: %s From: %s Peers:",
			out.str().c_str(), mFns->bdPrintId(id).c_str());
	debug << out;
	for(it = nodes.begin(); it != nodes.end(); it++)
	{
		debug << " " << mFns->bdPrintId(&(*it));
	}

	LOG.info(debug.str().c_str());

#else
	(void) transId;
#endif
	mCounterRecvReplyFindNode++;

	/* add neighbours to the potential list */
	for(it = nodes.begin(); it != nodes.end(); it++)
	{
		checkPotentialPeer(&(*it));
	}

	/* received reply - so peer must be good */
	uint32_t peerflags = BITDHT_PEER_STATUS_RECV_NODES; /* no id ;( */
	addPeer(id, peerflags);
}

/********* THIS IS THE SECOND STAGE
 *
 */

void bdNode::msgin_get_hash(bdId *id, bdToken *transId, bdNodeId *info_hash)
{
#ifdef DEBUG_NODE_MSGIN
	LOG.info("bdNode::msgin_get_hash() TransId: ";
	bdPrintTransId(LOG << log4cpp::Priority::INFO, transId);
	LOG.info(" From: ";
	mFns->bdPrintId(LOG << log4cpp::Priority::INFO, id);
	LOG.info(" InfoHash: ";
	mFns->bdPrintNodeId(LOG << log4cpp::Priority::INFO, info_hash);
	LOG.info(std::endl;
#endif

	mCounterRecvQueryHash++;

	/* generate message, send to udp */
	queueQuery(id, info_hash, transId, BD_QUERY_HASH);
}

void bdNode::msgin_reply_hash(bdId *id, bdToken *transId, bdToken *token, std::list<std::string> &values)
{
	mCounterRecvReplyQueryHash++;

#ifdef DEBUG_NODE_MSGIN
	LOG.info("bdNode::msgin_reply_hash() TransId: ";
	bdPrintTransId(LOG << log4cpp::Priority::INFO, transId);
	LOG.info(" From: ";
	mFns->bdPrintId(LOG << log4cpp::Priority::INFO, id);
	LOG.info(" Token: ";
	bdPrintToken(LOG << log4cpp::Priority::INFO, token);

	LOG.info(" Peers: ";
	std::list<std::string>::iterator it;
	for(it = values.begin(); it != values.end(); it++)
	{
		LOG.info(" ";
		bdPrintCompactPeerId(LOG << log4cpp::Priority::INFO, *it);
	}
	LOG.info(std::endl;
#else
	(void) id;
	(void) transId;
	(void) token;
	(void) values;
#endif
}

void bdNode::msgin_reply_nearest(bdId *id, bdToken *transId, bdToken *token, std::list<bdId> &nodes)
{
	//mCounterRecvReplyNearestHash++;

#ifdef DEBUG_NODE_MSGIN
	LOG.info("bdNode::msgin_reply_nearest() TransId: ";
	bdPrintTransId(LOG << log4cpp::Priority::INFO, transId);
	LOG.info(" From: ";
	mFns->bdPrintId(LOG << log4cpp::Priority::INFO, id);
	LOG.info(" Token: ";
	bdPrintToken(LOG << log4cpp::Priority::INFO, token);
	LOG.info(" Nodes:";

	std::list<bdId>::iterator it;
	for(it = nodes.begin(); it != nodes.end(); it++)
	{
		LOG.info(" ";
		mFns->bdPrintId(LOG << log4cpp::Priority::INFO, &(*it));
	}
	LOG.info(std::endl;
#else
	(void) id;
	(void) transId;
	(void) token;
	(void) nodes;
#endif
}

void bdNode::msgin_post_hash(bdId *id,  bdToken *transId,  bdNodeId *info_hash,  uint32_t port, bdToken *token)
{
	//mCounterRecvPostHash++;

#ifdef DEBUG_NODE_MSGIN
	LOG.info("bdNode::msgin_post_hash() TransId: ";
	bdPrintTransId(LOG << log4cpp::Priority::INFO, transId);
	LOG.info(" From: ";
	mFns->bdPrintId(LOG << log4cpp::Priority::INFO, id);
	LOG.info(" Info_Hash: ";
	mFns->bdPrintNodeId(LOG << log4cpp::Priority::INFO, info_hash);
	LOG.info(" Port: " << port;
	LOG.info(" Token: ";
	bdPrintToken(LOG << log4cpp::Priority::INFO, token);
	LOG.info(std::endl;
#else
	(void) id;
	(void) transId;
	(void) info_hash;
	(void) port;
	(void) token;
#endif

}


void bdNode::msgin_reply_post(bdId *id, bdToken *transId)
{
	/* generate message, send to udp */
	//mCounterRecvReplyPostHash++;

#ifdef DEBUG_NODE_MSGIN
	LOG.info("bdNode::msgin_reply_post() TransId: ";
	bdPrintTransId(LOG << log4cpp::Priority::INFO, transId);
	LOG.info(" From: ";
	mFns->bdPrintId(LOG << log4cpp::Priority::INFO, id);
	LOG.info(std::endl;
#else
	(void) id;
	(void) transId;
#endif
}

void bdNode::msgin_newconn(bdId *tunnelId, bdId *dhtId, bdToken *transId)
{
	msgout_reply_newconn(tunnelId, dhtId, transId);
	mPacketCallback->onRecvCallback(tunnelId, BITDHT_MSG_TYPE_NEWCONN);
}

void bdNode::msgin_reply_newconn(bdId *tunnelId, bdId *dhtId, bdToken *transId)
{
	mPacketCallback->onRecvCallback(tunnelId, BITDHT_MSG_TYPE_REPLY_NEWCONN);
}

/****************** Other Functions ******************/

void bdNode::genNewToken(bdToken *token)
{
#ifdef DEBUG_NODE_ACTIONS 
	LOG.info("bdNode::genNewToken()");
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

std::atomic_ulong transIdCounter(0);
void bdNode::genNewTransId(bdToken *token)
{
	/* generate message, send to udp */
#ifdef DEBUG_NODE_ACTIONS 
	LOG.info("bdNode::genNewTransId()");
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

/* Store Remote Query for processing */
int bdNode::queueQuery(bdId *id, bdNodeId *query, bdToken *transId, uint32_t query_type)
{
#ifdef DEBUG_NODE_ACTIONS 
	LOG.info("bdnode::queueQuery()");
#endif

	mRemoteQueries.push_back(bdRemoteQuery(id, query, transId, query_type));	

	return 1;
}

/*************** Register Transaction Ids *************/

void bdNode::registerOutgoingMsg(bdId *id, bdToken *transId, uint32_t msgType)
{

#ifdef DEBUG_MSG_CHECKS
	LOG.info("bdNode::registerOutgoingMsg(%s, %d)",
			mFns->bdPrintId(id), msgType);
#else
	(void) id;
	(void) msgType;
#endif

#ifdef USE_HISTORY
	mHistory.addMsg(id, transId, msgType, false);
#else
	(void) transId;
#endif



	/****
#define BITDHT_MSG_TYPE_UNKNOWN         0
#define BITDHT_MSG_TYPE_PING            1
#define BITDHT_MSG_TYPE_PONG            2
#define BITDHT_MSG_TYPE_FIND_NODE       3
#define BITDHT_MSG_TYPE_REPLY_NODE      4
#define BITDHT_MSG_TYPE_GET_HASH        5
#define BITDHT_MSG_TYPE_REPLY_HASH      6
#define BITDHT_MSG_TYPE_REPLY_NEAR      7
#define BITDHT_MSG_TYPE_POST_HASH       8
#define BITDHT_MSG_TYPE_REPLY_POST      9
	 ***/

}



uint32_t bdNode::checkIncomingMsg(bdId *id, bdToken *transId, uint32_t msgType)
{

#ifdef DEBUG_MSG_CHECKS
	LOG.info("bdNode::checkIncomingMsg(%s, %d)",
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

void bdNode::cleanupTransIdRegister()
{
	return;
}



/*************** Internal Msg Storage *****************/

bdNodeNetMsg::bdNodeNetMsg(char *msg, int len, struct sockaddr_in *in_addr)
:data(NULL), mSize(len), addr(*in_addr)
{
	data = (char *) malloc(len);
	memcpy(data, msg, len);
	//print(LOG << log4cpp::Priority::INFO);
}

void bdNodeNetMsg::print(std::ostream &out)
{
	out << "bdNodeNetMsg::print(" << mSize << ") to "
			<< inet_ntoa(addr.sin_addr) << ":" << htons(addr.sin_port);
	out << std::endl;
}


bdNodeNetMsg::~bdNodeNetMsg()
{
	free(data);
}


