/*
 * bitdht/bdmanager.cc
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


/*******
 * Node Manager.
 ******/

/******************************************
 * 1) Maintains a list of ids to search for.
 * 2) Sets up initial search for own node.
 * 3) Checks on status of queries.
 * 4) Callback on successful searches.
 *
 * This is pretty specific to RS requirements.
 ****/

#include "bitdht/bdiface.h"
#include "bitdht/bdstddht.h"
#include "bitdht/bdtunnelmanager.h"
#include "bitdht/bdmsgs.h"
#include "bitdht/bencode.h"

#include <algorithm>
#include <sstream>
#include <iomanip>
#include <ctype.h>

#include "util/bdnet.h"
#include "util/bdlog.h"

/***
 * #define DEBUG_MGR 1
 * #define DEBUG_MGR_PKT 1
 ***/

#define DEBUG_MGR 1

bdTunnelManager::bdTunnelManager(bdDhtFunctions *fns)
: bdTunnelNode(fns)
{
	mMode = BITDHT_TUN_MGR_STATE_OFF;
	mFns = fns;
	mModeTS = 0 ;

#ifdef DEBUG_MGR
	LOG.info("bdTunnelManager::bdTunnelManager()");
#endif
}

int	bdTunnelManager::stopTunnel()
{
	time_t now = time(NULL);

	/* clean up node */
	shutdownNode();

	/* flag queries as inactive */
	/* check if exists already */
	std::map<bdId, bdTunnelPeer>::iterator it;
	for(it = mTunnelPeers.begin(); it != mTunnelPeers.end(); it++)
	{
		it->second.mStatus = bdTunnelPeer::DISCONNECTED;
	}

	/* set state flag */
	mMode = BITDHT_TUN_MGR_STATE_OFF;
	mModeTS = now;

	return 1;
}

int	bdTunnelManager::startTunnel()
{
	time_t now = time(NULL);

	/* set startup mode */
	restartNode();

	mMode = BITDHT_TUN_MGR_STATE_STARTUP;
	mModeTS = now;

	return 1;
}

/* STOPPED, STARTING, ACTIVE, FAILED */
int bdTunnelManager::stateDht()
{
	return mMode;
}

void bdTunnelManager::connectNode(const bdId *id)
{
#if 0 //def DEBUG_MGR
	LOG.info("bdTunnelManager::connectNode() " + mFns->bdPrintNodeId(&id->id));

	std::map<bdId, bdTunnelPeer>::iterator it0;
	for (it0 = mTunnelPeers.begin(); it0 != mTunnelPeers.end(); it0++) {
		std::ostringstream ss;
		bdStdPrintId(ss, &it0->first);
		LOG.info("bdTunnelManager::connectNode():%s ", ss.str().c_str());
	}
#endif
	/* check if exists already */
	std::map<bdId, bdTunnelPeer>::iterator it;
	it = mTunnelPeers.find(*id);
	if (it != mTunnelPeers.end()) {
#ifdef DEBUG_MGR
		std::ostringstream ss;
		bdStdPrintId(ss, id);
		LOG.info("bdTunnelManager::connectNode() Found existing (total %d):%s....", mTunnelPeers.size(), ss.str().c_str());
#endif
		return;
	}

	/* add to map */
	bdTunnelPeer peer;
	peer.mId = (*id);
	peer.mStatus = bdTunnelPeer::CONNECTING;
	peer.mTunnelAddr.sin_addr.s_addr = 0;
	peer.mTunnelAddr.sin_port = 0;
	mTunnelPeers[*id] = peer;

#ifdef DEBUG_MGR
	std::ostringstream ss;
	bdStdPrintId(ss, id);
	LOG.info("bdTunnelManager::connectNode() Added QueryPeer as READY (total %d)...:%s....", mTunnelPeers.size(), ss.str().c_str());
#endif
	return;
}

void bdTunnelManager::disconnectNode(const bdId *id)
{
#ifdef DEBUG_MGR
	LOG.info("bdTunnelManager::disconnectNode() %s",
			mFns->bdPrintNodeId(&id->id).c_str());
#endif
	std::map<bdId, bdTunnelPeer>::iterator it;
	it = mTunnelPeers.find(*id);
	if (it == mTunnelPeers.end())
	{
		return;
	}

	// TODO::

	/* remove from map */
	mTunnelPeers.erase(it);
	return;
}

void bdTunnelManager::iteration()
{
	time_t now = time(NULL);
	time_t modeAge = now - mModeTS;
	switch(mMode)
	{
	case BITDHT_TUN_MGR_STATE_OFF:
	{
#ifdef DEBUG_MGR
		LOG.info("bdTunnelManager::iteration(): OFF");
#endif
	}
	break;

	case BITDHT_TUN_MGR_STATE_STARTUP:
		/* 10 seconds startup .... then switch to ACTIVE */

		if (modeAge > MAX_STARTUP_TIME)
		{
#ifdef DEBUG_MGR
			LOG.info("bdTunnelManager::iteration(): UNACTIVE -> STARTUP(%d)", mTunnelPeers.size());
#endif
			if (!mTunnelPeers.empty()) {
				mMode = BITDHT_TUN_MGR_STATE_NEWCONN;
			}
			mModeTS = now;
		}
		break;

	case BITDHT_TUN_MGR_STATE_NEWCONN:
		if (modeAge > MAX_REFRESH_TIME)
		{
#ifdef DEBUG_MGR
			LOG.info("bdTunnelManager::iteration(): STARTUP -> NEWCONN(%d)", mTunnelPeers.size());
#endif
			mMode = BITDHT_TUN_MGR_STATE_STARTUP;
			mModeTS = now;

			// make udp call
			bdId id;
			std::map<bdId, bdTunnelPeer>::iterator it;
			for (it = mTunnelPeers.begin(); it != mTunnelPeers.end(); it++) {
				addTunnel(&(it->first));
			}
		}
		break;

	default:
	case BITDHT_TUN_MGR_STATE_FAILED:
	{
		LOG.info("bdTunnelManager::iteration(): FAILED ==> STARTUP");
#ifdef DEBUG_MGR
#endif
		stopTunnel();
		startTunnel();
	}
	break;
	}

	if (mMode == BITDHT_TUN_MGR_STATE_OFF)
	{
		bdTunnelNode::iterationOff();
	}
	else
	{
		/* tick parent */
		bdTunnelNode::iteration();
	}
}

int bdTunnelManager::status()
{
	checkStatus();

	return 1;
}

int bdTunnelManager::checkStatus()
{
#ifdef DEBUG_MGR
	LOG.info("bdTunnelManager::checkStatus()");
#endif
	return 1;
}

/***** Add / Remove Callback Clients *****/
void bdTunnelManager::addCallback(BitDhtCallback *cb)
{
#ifdef DEBUG_MGR
	LOG.info("bdTunnelManager::addCallback()");
#endif
	/* search list */
	std::list<BitDhtCallback *>::iterator it;
	it = std::find(mCallbacks.begin(), mCallbacks.end(), cb);
	if (it == mCallbacks.end())
	{
		/* add it */
		mCallbacks.push_back(cb);
	}
}

void bdTunnelManager::removeCallback(BitDhtCallback *cb)
{

#ifdef DEBUG_MGR
	LOG.info("bdTunnelManager::removeCallback()");
#endif
	/* search list */
	std::list<BitDhtCallback *>::iterator it;
	it = std::find(mCallbacks.begin(), mCallbacks.end(), cb);
	if (it == mCallbacks.end())
	{
		/* not found! */
		return;
	}
	it = mCallbacks.erase(it);
}
