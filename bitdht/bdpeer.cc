/*
 * bitdht/bdpeer.cc
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
#include "util/bdnet.h"
#include "util/bdlog.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdio>

/**
 * #define BITDHT_DEBUG 1
**/

void bdSockAddrInit(struct sockaddr_in *addr)
{
	memset(addr, 0, sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
}

bdId::bdId() : type(GENERAL)
{
	/* blank everything */
	bdSockAddrInit(&addr);
	memset(&id.data, 0, BITDHT_KEY_LEN);
}

bdId::bdId(const bdId &bd_id) :
		type(GENERAL)
{
	/* this bit is to ensure the address is valid for windows / osx */
	bdSockAddrInit(&addr);
	addr.sin_addr.s_addr = bd_id.addr.sin_addr.s_addr;
	addr.sin_port = bd_id.addr.sin_port;

	for(int i = 0; i < BITDHT_KEY_LEN; i++)
	{
		id.data[i] = bd_id.id.data[i];
	}
}

bdId::bdId(const bdNodeId &in_id, const struct sockaddr_in &in_addr) :
		type(GENERAL)
{
	/* this bit is to ensure the address is valid for windows / osx */
	bdSockAddrInit(&addr);
	addr.sin_addr.s_addr = in_addr.sin_addr.s_addr;
	addr.sin_port = in_addr.sin_port;

	for(int i = 0; i < BITDHT_KEY_LEN; i++)
	{
		id.data[i] = in_id.data[i];
	}
}

bdId::bdId(const bdNodeId &in_id, const struct sockaddr_in &in_addr, int atype) :
		type(atype)
{
	/* this bit is to ensure the address is valid for windows / osx */
	bdSockAddrInit(&addr);
	addr.sin_addr.s_addr = in_addr.sin_addr.s_addr;
	addr.sin_port = in_addr.sin_port;

	for(int i = 0; i < BITDHT_KEY_LEN; i++)
	{
		id.data[i] = in_id.data[i];
	}
};

void bdZeroNodeId(bdNodeId *id)
{
	uint32_t *a_data = (uint32_t *) id->data;
	for(int i = 0; i < BITDHT_KEY_INTLEN; i++)
	{
		a_data[i] = 0;
	}
	return;
}


int operator<(const bdNodeId &a, const bdNodeId &b)
{
#if 0
	LOG << log4cpp::Priority::INFO <<  "operator<(");
	bdPrintNodeId(LOG << log4cpp::Priority::INFO, &a);
	LOG << log4cpp::Priority::INFO <<  ",";
	bdPrintNodeId(LOG << log4cpp::Priority::INFO, &b);
	LOG << log4cpp::Priority::INFO <<  ")" << std::endl;
#endif
	
	uint8_t *a_data = (uint8_t *) a.data;
	uint8_t *b_data = (uint8_t *) b.data;
	for(int i = 0; i < BITDHT_KEY_LEN; i++)	
	{
		if (*a_data < *b_data)
		{
			//LOG.info("Return 1, at i = %d\n", i);
			return 1;
		}
		else if (*a_data > *b_data)
		{
			//LOG.info("Return 0, at i = %d\n", i);
			return 0;
		}
		a_data++;
		b_data++;
	}
	//LOG.info("Return 0, at i = KEYLEN\n");
	return 0;
}

#if 0
int operator<(const struct sockaddr_in &a, const struct sockaddr_in &b)
{
	/* else NodeIds the same - check id addresses */
	if (a.sin_addr.s_addr < b.sin_addr.s_addr)
		return 1;
	if (b.sin_addr.s_addr > a.sin_addr.s_addr)
		return 0;

	if (a.sin_port < b.sin_port)
		return 1;

	return 0;
}
#endif

int operator<(const bdId &a, const bdId &b)
{
	if (a.id < b.id)
		return 1;
	if (b.id < a.id)
		return 0;

	/* else NodeIds the same - check id addresses */
	if (a.addr.sin_addr.s_addr < b.addr.sin_addr.s_addr)
		return 1;
	if (b.addr.sin_addr.s_addr > a.addr.sin_addr.s_addr)
		return 0;

	if (a.addr.sin_port < b.addr.sin_port)
		return 1;

	return 0;
}


int operator==(const bdNodeId &a, const bdNodeId &b)
{
	uint8_t *a_data = (uint8_t *) a.data;
	uint8_t *b_data = (uint8_t *) b.data;
	for(int i = 0; i < BITDHT_KEY_LEN; i++)	
	{
		if (*a_data < *b_data)
		{
			return 0;
		}
		else if (*a_data > *b_data)
		{
			return 0;
		}
		a_data++;
		b_data++;
	}
	return 1;
}

int operator==(const bdId &a, const bdId &b)
{
	if (!(a.id == b.id))
		return 0;

	if ((a.addr.sin_addr.s_addr == b.addr.sin_addr.s_addr) &&
	    (a.addr.sin_port == b.addr.sin_port))
	{
		return 1;
	}
	return 0;
}


#if 0
void bdRandomId(bdId *id)
{
	bdRandomNodeId(&(id->id));

	id->addr.sin_addr.s_addr = rand();
	id->addr.sin_port = rand();

	return;
}

void bdRandomNodeId(bdNodeId *id)
{
	uint32_t *a_data = (uint32_t *) id->data;
	for(int i = 0; i < BITDHT_KEY_INTLEN; i++)
	{
		a_data[i] = rand();
	}
	return;
}


/* fills in dbNodeId r, with XOR of a and b */
int bdDistance(const bdNodeId *a, const bdNodeId *b, bdMetric *r)
{
	uint8_t *a_data = (uint8_t *) a->data;
	uint8_t *b_data = (uint8_t *) b->data;
	uint8_t *ans = (uint8_t *) r->data;
	for(int i = 0; i < BITDHT_KEY_LEN; i++)	
	{
		*(ans++) = *(a_data++) ^ *(b_data++);
	}
	return 1;
}

void bdRandomMidId(const bdNodeId *target, const bdNodeId *other, bdNodeId *midId)
{
	bdMetric dist;
	
	/* get distance between a & c */
	bdDistance(target, other, &dist);

	/* generate Random Id */
	bdRandomNodeId(midId);

	/* zero bits of Random Id until under 1/2 of distance 
	 * done in bytes for ease... matches one extra byte than distance = 0 
	 * -> hence wierd order of operations
	 */
	bool done = false;
	for(int i = 0; i < BITDHT_KEY_LEN; i++)
	{
		midId->data[i] = target->data[i];

		if (dist.data[i] != 0)
			break;
	}
}

std::string bdConvertToPrintable(std::string input)
{
	std::ostringstream out;
        for(uint32_t i = 0; i < input.length(); i++)
        {
                /* sensible chars */
                if ((input[i] > 31) && (input[i] < 127))
                {
                        out << input[i];
                }
                else
                {
			out << "[0x" << std::hex << (uint32_t) input[i] << "]";
			out << std::dec;
                }
        }
	return out.str();
}

void bdPrintNodeId(std::ostream &out, const bdNodeId *a)
{
	for(int i = 0; i < BITDHT_KEY_LEN; i++)	
	{
		out << std::setw(2) << std::setfill('0') << std::hex << (uint32_t) (a->data)[i];
	}
	out << std::dec;

	return;
}


void bdPrintId(std::ostream &out, const bdId *a)
{
	bdPrintNodeId(out, &(a->id));
	out << " ip:" << inet_ntoa(a->addr.sin_addr);
	out << ":" << ntohs(a->addr.sin_port);
	return;
}

/* returns 0-160 depending on bucket */
int bdBucketDistance(const bdNodeId *a, const bdNodeId *b)
{
	bdMetric m;
	bdDistance(a, b, &m);
	return bdBucketDistance(&m);
}

/* returns 0-160 depending on bucket */
int bdBucketDistance(const bdMetric *m)
{
	for(int i = 0; i < BITDHT_KEY_BITLEN; i++)
	{
		int bit = BITDHT_KEY_BITLEN - i - 1;
		int byte = i / 8;
		int bbit = 7 - (i % 8);
		unsigned char comp = (1 << bbit);

#ifdef BITDHT_DEBUG
		LOG.info("bdBucketDistance: bit:%d  byte:%d bbit:%d comp:%x, data:%x\n", bit, byte, bbit, comp, m->data[byte]);
#endif

		if (comp & m->data[byte])
		{
			return bit;
		}
	}
	return 0;
}

#endif



bdBucket::bdBucket()
{
	return;
}

bdSpace::bdSpace(bdNodeId *ownId, bdDhtFunctions *fns)
	:mOwnId(*ownId), mFns(fns)
{
	/* make some space for data */
	buckets.resize(mFns->bdNumBuckets());
	return;
}

	/* empty the buckets */
int     bdSpace::clear()
{
	std::vector<bdBucket>::iterator it;
        /* iterate through the buckets, and sort by distance */
        for(it = buckets.begin(); it != buckets.end(); it++)
	{
		it->entries.clear();
	}
	return 1;
}


int bdSpace::find_nearest_nodes(
		const bdNodeId *id,
		int number,
		std::list<bdId> /*excluding*/,
		std::multimap<bdMetric, bdId> &nearest)
{
	std::multimap<bdMetric, bdId> closest;
	std::multimap<bdMetric, bdId>::iterator mit;

	bdMetric dist;
	mFns->bdDistance(id, &(mOwnId), &dist);

#ifdef DEBUG_BD_SPACE
	int bucket = mFns->bdBucketDistance(&dist);

	LOG << log4cpp::Priority::INFO << "bdSpace::find_nearest_nodes(NodeId:";
	mFns->bdPrintNodeId(LOG << log4cpp::Priority::INFO, id);

	LOG << log4cpp::Priority::INFO << " Number: " << number;
	LOG << log4cpp::Priority::INFO << " Query Bucket #: " << bucket;
	LOG << log4cpp::Priority::INFO << std::endl;
#endif

	std::vector<bdBucket>::iterator it;
	std::list<bdPeer>::iterator eit;
	/* iterate through the buckets, and sort by distance */
	for(it = buckets.begin(); it != buckets.end(); it++)
	{
		for(eit = it->entries.begin(); eit != it->entries.end(); eit++) 
		{
			mFns->bdDistance(id, &(eit->mPeerId.id), &dist);
			closest.insert(std::pair<bdMetric, bdId>(dist, eit->mPeerId));

#if 0
			LOG << log4cpp::Priority::INFO << "Added NodeId: ";
			bdPrintNodeId(LOG << log4cpp::Priority::INFO, &(eit->mPeerId.id));
			LOG << log4cpp::Priority::INFO << " Metric: ";
			bdPrintNodeId(LOG << log4cpp::Priority::INFO, &(dist));
			LOG << log4cpp::Priority::INFO << std::endl;
#endif
		}
	}

	/* take the first number of nodes */
	int i = 0;
	for(mit = closest.begin(); (mit != closest.end()) && (i < number); mit++, i++)
	{
		mFns->bdDistance(&(mOwnId), &(mit->second.id), &dist);

#ifdef DEBUG_BD_SPACE
		int iBucket = mFns->bdBucketDistance(&(mit->first));

		LOG << log4cpp::Priority::INFO << "Closest " << i << ": ";
		mFns->bdPrintNodeId(LOG << log4cpp::Priority::INFO, &(mit->second.id));
		LOG << log4cpp::Priority::INFO << " Bucket:        " << iBucket;
		LOG << log4cpp::Priority::INFO << std::endl;
#endif


#if 0
		LOG << log4cpp::Priority::INFO << "\tNodeId: ";
		mFns->bdPrintNodeId(LOG << log4cpp::Priority::INFO, &(mit->second.id));
		LOG << log4cpp::Priority::INFO << std::endl;

		LOG << log4cpp::Priority::INFO << "\tOwn Id: ";
		mFns->bdPrintNodeId(LOG << log4cpp::Priority::INFO, &(mOwnId));
		LOG << log4cpp::Priority::INFO << std::endl;

		LOG << log4cpp::Priority::INFO << "     Us Metric: ";
		mFns->bdPrintNodeId(LOG << log4cpp::Priority::INFO, &dist);
		LOG << log4cpp::Priority::INFO << " Bucket:        " << oBucket;
		LOG << log4cpp::Priority::INFO << std::endl;

		LOG << log4cpp::Priority::INFO << "\tFindId: ";
		mFns->bdPrintNodeId(LOG << log4cpp::Priority::INFO, id);
		LOG << log4cpp::Priority::INFO << std::endl;

		LOG << log4cpp::Priority::INFO << "     Id Metric: ";
		mFns->bdPrintNodeId(LOG << log4cpp::Priority::INFO, &(mit->first));
		LOG << log4cpp::Priority::INFO << " Bucket:        " << iBucket;
		LOG << log4cpp::Priority::INFO << std::endl;
#endif

		nearest.insert(*mit);
	}

#ifdef DEBUG_BD_SPACE
	LOG << log4cpp::Priority::INFO << "#Nearest: " << (int) nearest.size();
	LOG << log4cpp::Priority::INFO << " #Closest: " << (int) closest.size();
	LOG << log4cpp::Priority::INFO << " #Requested: " << number;
	LOG << log4cpp::Priority::INFO << std::endl << std::endl;
#endif

	return 1;
}


int	bdSpace::out_of_date_peer(bdId &id)
{
	/* 
	 * 
	 */

	std::map<bdMetric, bdId> closest;
	std::map<bdMetric, bdId>::iterator mit;

	std::vector<bdBucket>::iterator it;
	std::list<bdPeer>::iterator eit;
	time_t ts = time(NULL);

	/* iterate through the buckets, and sort by distance */
	for(it = buckets.begin(); it != buckets.end(); it++)
	{
		for(eit = it->entries.begin(); eit != it->entries.end(); eit++) 
		{
			/* timeout on last send time! */
			if (ts - eit->mLastSendTime > BITDHT_MAX_SEND_PERIOD )
			{
				id = eit->mPeerId;
				eit->mLastSendTime = ts;
				return 1;
			}
		}
	}
	return 0;
}

/* Called to add or update peer.
 * sorts bucket lists by lastRecvTime.
 * updates requested node.
 */

/* peer flags
 * order is important!
 * higher bits = more priority.
 * BITDHT_PEER_STATUS_RECVPONG
 * BITDHT_PEER_STATUS_RECVNODES
 * BITDHT_PEER_STATUS_RECVHASHES
 * BITDHT_PEER_STATUS_DHT_ENGINE  (dbXXxx)
 * BITDHT_PEER_STATUS_DHT_APPL    (XXRSxx)
 * BITDHT_PEER_STATUS_DHT_VERSION (XXxx50)
 * 
 */

int bdSpace::add_peer(const bdId *id, uint32_t peerflags)
{
	/* find the peer */
	bool add = false;
	time_t ts = time(NULL);
	
#ifdef DEBUG_BD_SPACE
	LOG.info("bdSpace::add_peer()\n");
#endif

	/* calculate metric */
	bdMetric met;
	mFns->bdDistance(&(mOwnId), &(id->id), &met);
	int bucket = mFns->bdBucketDistance(&met);

#ifdef DEBUG_BD_SPACE
	LOG.info("peer:");
	mFns->bdPrintId(LOG << log4cpp::Priority::INFO, id);	
	LOG.info(" bucket: %d", bucket);
	LOG.info("\n");
#endif

	/* select correct bucket */
	bdBucket &buck =  buckets[bucket];


	std::list<bdPeer>::iterator it;

	/* calculate the score for this new peer */
	uint32_t minScore = peerflags;

	/* loop through ids, to find it */
	for(it = buck.entries.begin(); it != buck.entries.end(); it++)
	{
		if (*id == it->mPeerId)
		// should check addr too!
		{
			bdPeer peer = *it;
			it = buck.entries.erase(it);

			peer.mLastRecvTime = ts;
			peer.mPeerFlags |= peerflags; /* must be cumulative ... so can do online, replynodes, etc */

			buck.entries.push_back(peer);

#ifdef DEBUG_BD_SPACE
			LOG.info("Peer already in bucket: moving to back of the list");
#endif

			return 1;
		}
		
		/* find lowest score */
		if (it->mPeerFlags < minScore)
		{
			minScore = it->mPeerFlags;
		}
	}

	/* not in the list! */

	if (buck.entries.size() < mFns->bdNodesPerBucket())
	{
#ifdef DEBUG_BD_SPACE
		LOG << log4cpp::Priority::INFO << "Bucket not full: allowing add" << std::endl;
#endif
		add = true;
	}
	else 
	{
		/* check head of list */
		bdPeer &peer = buck.entries.front();
		if (peer.mLastRecvTime - ts >  BITDHT_MAX_RECV_PERIOD)
		{
#ifdef DEBUG_BD_SPACE
			LOG << log4cpp::Priority::INFO << "Dropping Out-of-Date peer in bucket" << std::endl;
#endif
			buck.entries.pop_front();
			add = true;
		}
		else if (peerflags > minScore)
		{
			/* find one to drop */
			for(it = buck.entries.begin(); it != buck.entries.end(); it++)
			{
				if (it->mPeerFlags == minScore)
				{
					/* delete low priority peer */
					it = buck.entries.erase(it);
					add = true;
					break;
				}
			}

#ifdef DEBUG_BD_SPACE
			LOG << log4cpp::Priority::INFO << "Inserting due to Priority: minScore: " << minScore
				<< " new Peer Score: " << peerscore <<  << std::endl;
#endif
		}
		else
		{
#ifdef DEBUG_BD_SPACE
			LOG << log4cpp::Priority::INFO << "No Out-Of-Date peers in bucket... dropping new entry" << std::endl;
#endif
		}
	}

	if (add)
	{
		bdPeer newPeer;

		newPeer.mPeerId = *id;
		newPeer.mLastRecvTime = ts;
		newPeer.mLastSendTime = ts; //????
		newPeer.mPeerFlags = peerflags;

		buck.entries.push_back(newPeer);

#ifdef DEBUG_BD_SPACE
#endif
		/* useful debug */
		LOG.info("bdSpace::add_peer() Added Bucket[%d] Entry: %s",
				bucket, mFns->bdPrintId(id).c_str());
	}
	return add;
}

/* print tables.
 */

int     bdSpace::printDHT()
{
	std::map<bdMetric, bdId> closest;
	std::map<bdMetric, bdId>::iterator mit;

	std::vector<bdBucket>::iterator it;
	std::list<bdPeer>::iterator eit;

	/* iterate through the buckets, and sort by distance */
	int i = 0;

#ifdef BITDHT_DEBUG
	LOG.info("bdSpace::printDHT()\n");
	for(it = buckets.begin(); it != buckets.end(); it++, i++)
	{
		if (it->entries.size() > 0)
		{
			LOG.info("Bucket %d ----------------------------\n", i);
		}

		for(eit = it->entries.begin(); eit != it->entries.end(); eit++) 
		{
			bdMetric dist;
			mFns->bdDistance(&(mOwnId), &(eit->mPeerId.id), &dist);

			LOG.info(" Metric: ");
			mFns->bdPrintNodeId(LOG << log4cpp::Priority::INFO, &(dist));
			LOG.info(" Id: ");
			mFns->bdPrintId(LOG << log4cpp::Priority::INFO, &(eit->mPeerId));
			LOG.info(" PeerFlags: %08x", eit->mPeerFlags);
			LOG.info("\n");
		}
	}
#endif

	LOG.info("--------------------------------------");
	LOG.info("DHT Table Summary --------------------");
	LOG.info("--------------------------------------");

	/* little summary */
	unsigned long long sum = 0;
	unsigned long long no_peers = 0;
	uint32_t count = 0;
	bool doPrint = false;
	bool doAvg = false;

	i = 0;
	for(it = buckets.begin(); it != buckets.end(); it++, i++)
	{
		std::ostringstream debug;
		int size = it->entries.size();
		int shift = BITDHT_KEY_BITLEN - i;
		bool toBig = false;

		if (shift > BITDHT_ULLONG_BITS - mFns->bdBucketBitSize() - 1)
		{
			toBig = true;
			shift = BITDHT_ULLONG_BITS - mFns->bdBucketBitSize() - 1;
		}
		unsigned long long no_nets = ((unsigned long long) 1 << shift);

		/* use doPrint so it acts as a single switch */
		if (size && !doAvg && !doPrint)
		{	
			doAvg = true;
		}

		if (size && !doPrint)
		{	
			doPrint = true;
		}

		if (size == 0)
		{
			/* reset counters - if empty slot - to discount outliers in average */
			sum = 0;
			no_peers = 0;
			count = 0;
		}

		if (doPrint)
		{
			if (size) {
				char buf[80];
				snprintf(buf, sizeof(buf), "Bucket %d: %d peers: ", i, size);
				debug << buf;
			}
#ifdef BITDHT_DEBUG
			else {
				char buf[80];
				snprintf(buf, sizeof(buf), "Bucket %d: %d peers: ", i, size);
				debug << buf;
			}
#endif
		}
		if (toBig)
		{
			if (size)
			{
				if (doPrint) {
					char buf[80];
					snprintf(buf, sizeof(buf), "Estimated NetSize >> %llu", no_nets);
					debug << buf;
				}
			}
			else
			{
#ifdef BITDHT_DEBUG
				if (doPrint) {
					char buf[80];
					snprintf(buf, sizeof(buf), " Bucket = Net / >> %llu", no_nets);
					debug << buf;
				}
#endif
			}
		}
		else
		{
			no_peers = no_nets * size;
			if (size)
			{	
				if (doPrint) {
					char buf[80];
					snprintf(buf, sizeof(buf), "Estimated NetSize = %llu", no_peers);
					debug << buf;
				}
			}
			else
			{

#ifdef BITDHT_DEBUG
				if (doPrint) {
					char buf[80];
					snprintf(buf, sizeof(buf), " Bucket = Net / %llu\n", no_nets);
					debug << buf;
				}
#endif
			}
		}
		if (doPrint && doAvg && !toBig)
		{
			if (size == mFns->bdNodesPerBucket())
			{
				/* last average */
				doAvg = false;
			}
			if (no_peers != 0)
			{
				sum += no_peers;
				count++;	
#ifdef BITDHT_DEBUG
				char buf[80];
				snprintf(buf, sizeof(buf), "Est: %d: %llu => %llu / %d\n",
					i, no_peers, sum, count);
				debug << buf;
#endif
			}
		}

		LOG.info(debug.str());
	}
	if (count == 0)
	{
		LOG.info("Zero Network Size (count = 0)");
	}
	else
	{
		LOG.info("Estimated Network Size = (%llu / %d) = %llu", sum, count, sum / count);
	}

	return 1;
}


uint32_t  bdSpace::calcNetworkSize()
{
	std::vector<bdBucket>::iterator it;

	/* little summary */
	unsigned long long sum = 0;
	unsigned long long no_peers = 0;
	uint32_t count = 0;
	bool doPrint = false;
	bool doAvg = false;

	int i = 0;
	for(it = buckets.begin(); it != buckets.end(); it++, i++)
	{
		int size = it->entries.size();
		int shift = BITDHT_KEY_BITLEN - i;
		bool toBig = false;

		if (shift > BITDHT_ULLONG_BITS - mFns->bdBucketBitSize() - 1)
		{
			toBig = true;
			shift = BITDHT_ULLONG_BITS - mFns->bdBucketBitSize() - 1;
		}
		unsigned long long no_nets = ((unsigned long long) 1 << shift);

		/* use doPrint so it acts as a single switch */
		if (size && !doAvg && !doPrint)
		{	
			doAvg = true;
		}

		if (size && !doPrint)
		{	
			doPrint = true;
		}

		if (size == 0)
		{
			/* reset counters - if empty slot - to discount outliers in average */
			sum = 0;
			no_peers = 0;
			count = 0;
		}

		if (!toBig)
		{
			no_peers = no_nets * size;
		}

		if (doPrint && doAvg && !toBig)
		{
			if (size == mFns->bdNodesPerBucket())
			{
				/* last average */
				doAvg = false;
			}
			if (no_peers != 0)
			{
				sum += no_peers;
				count++;	
			}
		}
	}


	uint32_t NetSize = 0;
	if (count != 0)
	{
		NetSize = sum / count;
	}

	//LOG << log4cpp::Priority::INFO << "bdSpace::calcNetworkSize() : " << NetSize;
	//LOG << log4cpp::Priority::INFO << std::endl;

	return NetSize;
}

uint32_t  bdSpace::calcNetworkSizeWithFlag(uint32_t withFlag)
{
	std::vector<bdBucket>::iterator it;

	/* little summary */
	unsigned long long sum = 0;
	unsigned long long no_peers = 0;
	uint32_t count = 0;
	uint32_t totalcount = 0;
	bool doPrint = false;
	bool doAvg = false;

	int i = 0;
	for(it = buckets.begin(); it != buckets.end(); it++, i++)
	{
		int size = 0;
		std::list<bdPeer>::iterator lit;
		for(lit = it->entries.begin(); lit != it->entries.end(); lit++)
		{
			if (withFlag & lit->mPeerFlags)
			{
				size++;
			}
		}

		totalcount += size;

		int shift = BITDHT_KEY_BITLEN - i;
		bool toBig = false;

		if (shift > BITDHT_ULLONG_BITS - mFns->bdBucketBitSize() - 1)
		{
			toBig = true;
			shift = BITDHT_ULLONG_BITS - mFns->bdBucketBitSize() - 1;
		}
		unsigned long long no_nets = ((unsigned long long) 1 << shift);


		/* use doPrint so it acts as a single switch */
		if (size && !doAvg && !doPrint)
		{	
			doAvg = true;
		}

		if (size && !doPrint)
		{	
			doPrint = true;
		}

		if (size == 0)
		{
			/* reset counters - if empty slot - to discount outliers in average */
			sum = 0;
			no_peers = 0;
			count = 0;
		}

		if (!toBig)
		{
			no_peers = no_nets * size;
		}

		if (doPrint && doAvg && !toBig)
		{
			if (size == mFns->bdNodesPerBucket())
			{
				/* last average */
				doAvg = false;
			}
			if (no_peers != 0)
			{
				sum += no_peers;
				count++;	
			}
		}
	}


	uint32_t NetSize = 0;
	if (count != 0)
	{
		NetSize = sum / count;
	}

	//LOG << log4cpp::Priority::INFO << "bdSpace::calcNetworkSize() : " << NetSize;
	//LOG << log4cpp::Priority::INFO << std::endl;

	return NetSize;
}

uint32_t  bdSpace::calcSpaceSize()
{
	std::vector<bdBucket>::iterator it;

	/* little summary */
	uint32_t totalcount = 0;
	for(it = buckets.begin(); it != buckets.end(); it++)
	{
		totalcount += it->entries.size();
	}
	return totalcount;
}
