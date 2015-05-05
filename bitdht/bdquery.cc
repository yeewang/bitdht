
/*
 * bitdht/bdquery.cc
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


#include "bitdht/bdquery.h"
#include "util/bdnet.h"
#include "util/bdlog.h"

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <sstream>

/**
 * #define DEBUG_QUERY 1
**/


#define EXPECTED_REPLY 20
#define QUERY_IDLE_RETRY_PEER_PERIOD 300 // 5min =  (mFns->bdNodesPerBucket() * 30)


/************************************************************
 * bdQuery logic:
 *  1) as replies come in ... maintain list of M closest peers to ID.
 *  2) select non-queried peer from list, and query.
 *  3) halt when we have asked all M closest peers about the ID.
 * 
 * Flags can be set to disguise the target of the search.
 * This involves 
 */

bdQuery::bdQuery(const bdNodeId *id, std::list<bdId> &startList, uint32_t queryFlags, 
		bdDhtFunctions *fns)
{
	/* */
	mId = *id;
	mFns = fns;

	time_t now = time(NULL);
	std::list<bdId>::iterator it;
	for(it = startList.begin(); it != startList.end(); it++)
	{
		bdPeer peer;
		peer.mLastSendTime = 0;
		peer.mLastRecvTime = 0;
		peer.mFoundTime = now;
		peer.mPeerId = *it;

		bdMetric dist;

		mFns->bdDistance(&mId, &(peer.mPeerId.id), &dist);

		mClosest.insert(std::pair<bdMetric, bdPeer>(dist, peer));


	}

	mState = BITDHT_QUERY_QUERYING;
	mQueryFlags = queryFlags;
	mQueryTS = now;
	mSearchTime = 0;

	mQueryIdlePeerRetryPeriod = QUERY_IDLE_RETRY_PEER_PERIOD;

	/* setup the limit of the search
	 * by default it is setup to 000000 = exact match
	 */
	bdZeroNodeId(&mLimit);
}

bool bdQuery::result(std::list<bdId> &answer)
{
	/* get all the matches to our query */
        std::multimap<bdMetric, bdPeer>::iterator sit, eit;
	sit = mClosest.begin();
	eit = mClosest.upper_bound(mLimit);
	int i = 0;
	for(; sit != eit; sit++, i++)
	{
		answer.push_back(sit->second.mPeerId);
	}
	return (i > 0);
}

bool bdQuery::matchResult(std::list<bdPeer> &idList)
{
#ifdef DEBUG_QUERY
	LOG.info("bdQuery::fullResult()");
#endif

	int i = 0;
	std::multimap<bdMetric, bdPeer>::iterator it;
	for(it = mClosest.begin(); it != mClosest.end(); it++) {
		if (it->second.mPeerId.id == mId /*&& it->second.mPeerFlags != 0 && (it->second.mPeerId.type & typeMask) != 0*/) {
			i++;
			idList.push_back(it->second);
		}
	}
	for(it = mPotentialClosest.begin(); it != mPotentialClosest.end(); it++, i++) {
		if (it->second.mPeerId.id == mId /*&& it->second.mPeerFlags != 0 && (it->second.mPeerId.type & typeMask) != 0*/) {
			i++;
			idList.push_back(it->second);
		}
	}
	return (i > 0);
}

int bdQuery::nextQuery(bdId &id, bdNodeId &targetNodeId)
{
	if ((mState != BITDHT_QUERY_QUERYING) && !(mQueryFlags & BITDHT_QFLAGS_DO_IDLE))
	{
#ifdef DEBUG_QUERY 
        	LOG.info("NextQuery() Query is done\n");
#endif
		return 0;
	}

	/* search through through list, find closest not queried */
	time_t now = time(NULL);

	/* update IdlePeerRetry */
	if ((now - mQueryTS) / 2 > mQueryIdlePeerRetryPeriod)
	{
		mQueryIdlePeerRetryPeriod = (now-mQueryTS) / 2;
	}

	bool notFinished = false;
	std::multimap<bdMetric, bdPeer>::iterator it;
	for(it = mClosest.begin(); it != mClosest.end(); it++)
	{
		bool queryPeer = false;

		/* if never queried */
		if (it->second.mLastSendTime == 0)
		{
#ifdef DEBUG_QUERY 
        		LOG.info("NextQuery() Found non-sent peer. queryPeer = true : ");
			mFns->bdPrintId(LOG << log4cpp::Priority::INFO, &(it->second.mPeerId));
			LOG << log4cpp::Priority::INFO << std::endl;
#endif
			queryPeer = true;
		}

		/* re-request every so often */
		if ((!queryPeer) && (mQueryFlags & BITDHT_QFLAGS_DO_IDLE) && 
			(now - it->second.mLastSendTime > mQueryIdlePeerRetryPeriod))
		{
#ifdef DEBUG_QUERY 
        		LOG.info("NextQuery() Found out-of-date. queryPeer = true : ");
			mFns->bdPrintId(LOG << log4cpp::Priority::INFO, &(it->second.mPeerId));
			LOG << log4cpp::Priority::INFO << std::endl;
#endif
			queryPeer = true;
		}
		
		/* expecting every peer to be up-to-date is too hard...
		 * enough just to have received lists from each 
		 * - replacement policy will still work.
		 */	
		if (it->second.mLastRecvTime == 0)
		{
#ifdef DEBUG_QUERY 
        		LOG.info("NextQuery() Never Received: notFinished = true: ");
			mFns->bdPrintId(LOG << log4cpp::Priority::INFO, &(it->second.mPeerId));
			LOG << log4cpp::Priority::INFO << std::endl;
#endif
			notFinished = true;
		}

		if (queryPeer)
		{
			id = it->second.mPeerId;
			it->second.mLastSendTime = now;

			if (mQueryFlags & BITDHT_QFLAGS_DISGUISE)
			{
				/* calc Id mid point between Target and Peer */
				bdNodeId midRndId;
				mFns->bdRandomMidId(&mId, &(id.id), &midRndId);

				targetNodeId = midRndId;
			}
			else
			{
				targetNodeId = mId;
			}
#ifdef DEBUG_QUERY 
        		LOG.info("NextQuery() Querying Peer: ");
			mFns->bdPrintId(LOG << log4cpp::Priority::INFO, &id);
			LOG << log4cpp::Priority::INFO << std::endl;
#endif
			return 1;
		}
	}

	/* allow query to run for a minimal amount of time
	 * This is important as startup - when we might not have any peers.	
	 * Probably should be handled elsewhere.
	 */
	time_t age = now - mQueryTS;

	if (age < BITDHT_MIN_QUERY_AGE)
	{
#ifdef DEBUG_QUERY 
       		LOG.info("NextQuery() under Min Time: Query not finished / No Query\n");
#endif
		return 0;
	}

	if (age > BITDHT_MAX_QUERY_AGE)
	{
#ifdef DEBUG_QUERY 
       		LOG.info("NextQuery() under Min Time: Query not finished / No Query\n");
#endif
		/* fall through and stop */
	}
	else if ((mClosest.size() < mFns->bdNodesPerBucket()) || (notFinished))
	{
#ifdef DEBUG_QUERY 
       		LOG.info("NextQuery() notFinished | !size(): Query not finished / No Query\n");
#endif
		/* not full yet... */
		return 0;
	}

#ifdef DEBUG_QUERY 
	LOG.info("NextQuery() Finished\n");
#endif

	/* if we get here - query finished */
	if (mState == BITDHT_QUERY_QUERYING)
	{
		/* store query time */
		mSearchTime = now - mQueryTS;
	}

	/* check if we found the node */
	if (mClosest.size() > 0)
	{
		if ((mClosest.begin()->second).mPeerId.id == mId)
		{
			mState = BITDHT_QUERY_SUCCESS;
		}
		else if ((mPotentialClosest.begin()->second).mPeerId.id == mId)
		{
			mState = BITDHT_QUERY_PEER_UNREACHABLE;
		}
		else
		{
			mState = BITDHT_QUERY_FOUND_CLOSEST;
		}
	}
	else
	{
		mState = BITDHT_QUERY_FAILURE;
	}
	return 0;
}

int bdQuery::addPeer(const bdId *id, uint32_t mode)
{
	bdMetric dist;
	time_t ts = time(NULL);

	mFns->bdDistance(&mId, &(id->id), &dist);

#ifdef DEBUG_QUERY 
        LOG.info("bdQuery::addPeer(");
	mFns->bdPrintId(LOG << log4cpp::Priority::INFO, id);
        LOG.info(", %u)\n", mode);
#endif

	std::multimap<bdMetric, bdPeer>::iterator it, sit, eit;
	sit = mClosest.lower_bound(dist);
	eit = mClosest.upper_bound(dist);
	int i = 0;
	int actualCloser = 0;
	int toDrop = 0;
	for(it = mClosest.begin(); it != sit; it++, i++, actualCloser++)
	{
		time_t sendts = ts - it->second.mLastSendTime;
		bool hasSent = (it->second.mLastSendTime != 0);
		bool hasReply = (it->second.mLastRecvTime >= it->second.mLastSendTime);
		if ((hasSent) && (!hasReply) && (sendts > EXPECTED_REPLY))
		{
			i--; /* dont count this one */
			toDrop++;
		}
	}
		// Counts where we are.
#ifdef DEBUG_QUERY 
        LOG.info("Searching.... %di = %d - %d peers closer than this one\n", i, actualCloser, toDrop);
#endif

	if (i > mFns->bdNodesPerBucket() - 1)
	{
#ifdef DEBUG_QUERY 
        	LOG.info("Distance to far... dropping\n");
#endif
		/* drop it */
		return 0;
	}

	for(it = sit; it != eit; it++, i++)
	{
		/* full id check */
		if (it->second.mPeerId == *id)
		{
#ifdef DEBUG_QUERY 
        		LOG.info("Peer Already here!\n");
#endif
			if (mode & BITDHT_PEER_STATUS_RECV_NODES)
			{
				/* only update recvTime if sendTime > checkTime.... (then its our query) */
#ifdef DEBUG_QUERY 
				LOG.info("Updating LastRecvTime\n");
#endif
				it->second.mLastRecvTime = ts;
			}
			return 1;
		}
	}

#ifdef DEBUG_QUERY 
        LOG.info("Peer not in Query\n");
#endif
	/* firstly drop unresponded (bit ugly - but hard structure to extract from) */
	int j;
	for(j = 0; j < toDrop; j++)
	{
#ifdef DEBUG_QUERY 
        	LOG.info("Dropping Peer that dont reply\n");
#endif

        for(it = mClosest.begin(); it != mClosest.end(); ++it)
		{
			time_t sendts = ts - it->second.mLastSendTime;
			bool hasSent = (it->second.mLastSendTime != 0);
			bool hasReply = (it->second.mLastRecvTime >= it->second.mLastSendTime);

			if ((hasSent) && (!hasReply) && (sendts > EXPECTED_REPLY))
			{
#ifdef DEBUG_QUERY 
        			LOG.info("Dropped: ");
				mFns->bdPrintId(LOG << log4cpp::Priority::INFO, &(it->second.mPeerId));
        			LOG.info("\n");
#endif
				mClosest.erase(it);
				break ;
			}
		}
	}

	/* trim it back */
	while(mClosest.size() > (uint32_t) (mFns->bdNodesPerBucket() - 1))
	{
		std::multimap<bdMetric, bdPeer>::iterator it;
		it = mClosest.end();
		if (!mClosest.empty())
		{
			it--;
#ifdef DEBUG_QUERY 
			LOG.info("Removing Furthest Peer: ");
			mFns->bdPrintId(LOG << log4cpp::Priority::INFO, &(it->second.mPeerId));
			LOG.info("\n");
#endif

			mClosest.erase(it);
		}
	}

#ifdef DEBUG_QUERY 
        LOG.info("bdQuery::addPeer(): Closer Peer!: ");
	mFns->bdPrintId(LOG << log4cpp::Priority::INFO, id);
        LOG.info("\n");
#endif

	/* add it in */
	bdPeer peer;
	peer.mPeerId = *id;
	peer.mLastSendTime = 0;
	peer.mLastRecvTime = 0;
	peer.mFoundTime = ts;

	if (mode & BITDHT_PEER_STATUS_RECV_NODES)
	{
		peer.mLastRecvTime = ts;
	}

	mClosest.insert(std::pair<bdMetric, bdPeer>(dist, peer));
	return 1;
}

/* we also want to track unreachable node ... this allows us
 * to detect if peer are online - but uncontactible by dht.
 * 
 * simple list of closest.
 */

int bdQuery::addPotentialPeer(const bdId *id, uint32_t mode)
{
	bdMetric dist;
	time_t ts = time(NULL);

	mFns->bdDistance(&mId, &(id->id), &dist);

#ifdef DEBUG_QUERY 
        LOG.info("bdQuery::addPotentialPeer(");
	mFns->bdPrintId(LOG << log4cpp::Priority::INFO, id);
        LOG.info(", %u)\n", mode);
#endif

	/* first we check if this is a worthy potential peer....
	 * if it is already in mClosest -> false. old peer.
	 * if it is > mClosest.rbegin() -> false. too far way.
	 */
	int retval = 1;

	std::multimap<bdMetric, bdPeer>::iterator it, sit, eit;
	sit = mClosest.lower_bound(dist);
	eit = mClosest.upper_bound(dist);

	for(it = sit; it != eit; it++)
	{
		if (it->second.mPeerId == *id)
		{
			/* already there */
			retval = 0;
#ifdef DEBUG_QUERY 
			LOG.info("Peer already in mClosest\n");
#endif
		}
		//empty loop.
	}

	/* check if outside range, & bucket is full  */
	if ((sit == mClosest.end()) && (mClosest.size() >= mFns->bdNodesPerBucket()))
	{
#ifdef DEBUG_QUERY 
		LOG.info("Peer to far away for Potential\n");
#endif
		retval = 0; /* too far way */
	}

	/* return if false; */
	if (!retval)
	{
#ifdef DEBUG_QUERY 
		LOG.info("Flagging as Not a Potential Peer!\n");
#endif
		return retval;
	}

	/* finally if a worthy & new peer -> add into potential closest
	 * and repeat existance tests with PotentialPeers
	 */

	sit = mPotentialClosest.lower_bound(dist);
	eit = mPotentialClosest.upper_bound(dist);
	int i = 0;
	for(it = mPotentialClosest.begin(); it != sit; it++, i++)
	{
		//empty loop.
	}

	if (i > mFns->bdNodesPerBucket() - 1)
	{
#ifdef DEBUG_QUERY 
        	LOG.info("Distance to far... dropping\n");
        	LOG.info("Flagging as Potential Peer!\n");
#endif
		/* outside the list - so we won't add to mPotentialClosest 
		 * but inside mClosest still - so should still try it 
		 */
		retval = 1;
		return retval;
	}

	for(it = sit; it != eit; it++, i++)
	{
		if (it->second.mPeerId == *id)
		{
			/* this means its already been pinged */
#ifdef DEBUG_QUERY 
        		LOG.info("Peer Already here in mPotentialClosest!\n");
#endif
			if (mode & BITDHT_PEER_STATUS_RECV_NODES)
			{
#ifdef DEBUG_QUERY 
        			LOG.info("Updating LastRecvTime\n");
#endif
				it->second.mLastRecvTime = ts;
			}
#ifdef DEBUG_QUERY 
        		LOG.info("Flagging as Not a Potential Peer!\n");
#endif
			retval = 0;
			return retval;
		}
	}

#ifdef DEBUG_QUERY 
        LOG.info("Peer not in Query\n");
#endif


	/* trim it back */
	while(mPotentialClosest.size() > (uint32_t) (mFns->bdNodesPerBucket() - 1))
	{
		std::multimap<bdMetric, bdPeer>::iterator it;
		it = mPotentialClosest.end();

		if(!mPotentialClosest.empty())
		{
			--it;
#ifdef DEBUG_QUERY 
			LOG.info("Removing Furthest Peer: ");
			mFns->bdPrintId(LOG << log4cpp::Priority::INFO, &(it->second.mPeerId));
			LOG.info("\n");
#endif
			mPotentialClosest.erase(it);
		}
	}

#ifdef DEBUG_QUERY 
        LOG.info("bdQuery::addPotentialPeer(): Closer Peer!: ");
	mFns->bdPrintId(LOG << log4cpp::Priority::INFO, id);
        LOG.info("\n");
#endif

	/* add it in */
	bdPeer peer;
	peer.mPeerId = *id;
	peer.mLastSendTime = 0;
	peer.mLastRecvTime = ts;
	peer.mFoundTime = ts;
	mPotentialClosest.insert(std::pair<bdMetric, bdPeer>(dist, peer));

#ifdef DEBUG_QUERY 
	LOG.info("Flagging as Potential Peer!\n");
#endif
	retval = 1;
	return retval;
}

/* print query.
 */

int bdQuery::printQuery()
{
	std::ostringstream debug;
	char debugBuf[80];

#ifdef DEBUG_QUERY 
	LOG.info("bdQuery::printQuery()");
#endif
	
	time_t ts = time(NULL);
	debug << ("Query for: " + mFns->bdPrintNodeId(&mId));

	snprintf(debugBuf, sizeof(debugBuf), " Query State: %d", mState);
	debug << debugBuf;

	snprintf(debugBuf, sizeof(debugBuf), " Query Age %ld secs", ts-mQueryTS);
	debug << debugBuf;
	if (mState >= BITDHT_QUERY_FAILURE)
	{
		snprintf(debugBuf, sizeof(debugBuf), " Search Time: %d secs", mSearchTime);
		debug << debugBuf;
	}
	LOG.info(debug.str().c_str());
	debug.str("");

#ifdef DEBUG_QUERY
	LOG.info("Closest Available Peers:");
	std::multimap<bdMetric, bdPeer>::iterator it;
	for(it = mClosest.begin(); it != mClosest.end(); it++)
	{

		snprintf(debugBuf, sizeof(debugBuf), "Id:  %s",
				mFns->bdPrintId&(it->second.mPeerId).c_str());
		debug << debugBuf;
		snprintf(debugBuf, sizeof(debugBuf), "  Bucket: %d ", mFns->bdBucketDistance(&(it->first)));
		debug << debugBuf;
		snprintf(debugBuf, sizeof(debugBuf), " Found: %ld ago", ts-it->second.mFoundTime);
		debug << debugBuf;
		snprintf(debugBuf, sizeof(debugBuf), " LastSent: %ld ago", ts-it->second.mLastSendTime);
		debug << debugBuf;
		snprintf(debugBuf, sizeof(debugBuf), " LastRecv: %ld ago", ts-it->second.mLastRecvTime);
		LOG.info(debug.str());
		debug.str("");
	}

	LOG.info("\nClosest Potential Peers:");
	for(it = mPotentialClosest.begin(); it != mPotentialClosest.end(); it++)
	{
		snprintf(debugBuf, sizeof(debugBuf), "Id:  %s",
				mFns->bdPrintId(&(it->second.mPeerId).c_str());
		debug << debugBuf;
		snprintf(debugBuf, sizeof(debugBuf), "  Bucket: %d ", mFns->bdBucketDistance(&(it->first)));
		debug << debugBuf;
		snprintf(debugBuf, sizeof(debugBuf), " Found: %ld ago", ts-it->second.mFoundTime);
		debug << debugBuf;
		snprintf(debugBuf, sizeof(debugBuf), " LastSent: %ld ago", ts-it->second.mLastSendTime);
		debug << debugBuf;
		snprintf(debugBuf, sizeof(debugBuf), " LastRecv: %ld ago", ts-it->second.mLastRecvTime);
		debug << debugBuf;
		LOG.info(debug.str());
		debug.str("");
	}
#else
	// shortened version.
	LOG.info("Closest Available Peer: ");
	std::multimap<bdMetric, bdPeer>::iterator it = mClosest.begin(); 
	if (it != mClosest.end())
	{
		snprintf(debugBuf, sizeof(debugBuf), "%s", mFns->bdPrintId(&(it->second.mPeerId)).c_str());
		debug << debugBuf;
		snprintf(debugBuf, sizeof(debugBuf), "  Bucket: %d ", mFns->bdBucketDistance(&(it->first)));
		debug << debugBuf;
		snprintf(debugBuf, sizeof(debugBuf), " Found: %ld ago", ts-it->second.mFoundTime);
		debug << debugBuf;
		snprintf(debugBuf, sizeof(debugBuf), " LastSent: %ld ago", ts-it->second.mLastSendTime);
		debug << debugBuf;
		snprintf(debugBuf, sizeof(debugBuf), " LastRecv: %ld ago", ts-it->second.mLastRecvTime);
		debug << debugBuf;
	}
	LOG.info(debug.str().c_str());
	debug.str("");

	LOG.info("Closest Potential Peer: ");
	it = mPotentialClosest.begin(); 
	if (it != mPotentialClosest.end())
	{
		snprintf(debugBuf, sizeof(debugBuf), "%s", mFns->bdPrintId(&(it->second.mPeerId)).c_str());
		debug << debugBuf;
		snprintf(debugBuf, sizeof(debugBuf), "  Bucket: %d ", mFns->bdBucketDistance(&(it->first)));
		debug << debugBuf;
		snprintf(debugBuf, sizeof(debugBuf), " Found: %ld ago", ts-it->second.mFoundTime);
		debug << debugBuf;
		snprintf(debugBuf, sizeof(debugBuf), " LastSent: %ld ago", ts-it->second.mLastSendTime);
		debug << debugBuf;
		snprintf(debugBuf, sizeof(debugBuf), " LastRecv: %ld ago", ts-it->second.mLastRecvTime);
		debug << debugBuf;
	}
	LOG.info(debug.str().c_str());
	debug.str("");
#endif

	return 1;
}

/********************************* Remote Query **************************************/
bdRemoteQuery::bdRemoteQuery(bdId *id, bdNodeId *query, bdToken *transId, uint32_t query_type)
	:mId(*id), mQuery(*query), mTransId(*transId), mQueryType(query_type)
{
	mQueryTS = time(NULL);
}
