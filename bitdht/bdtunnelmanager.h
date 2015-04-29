#ifndef BITDHT_TUNNEL_MANAGER_H
#define BITDHT_TUNNEL_MANAGER_H

/*
 * bitdht/bdtunnelmanager.h
 */

#include "bdtunnelnode.h"

#define BITDHT_TUN_MGR_STATE_OFF		0
#define BITDHT_TUN_MGR_STATE_STARTUP	1
#define BITDHT_TUN_MGR_STATE_NEWCONN 	2
#define BITDHT_TUN_MGR_STATE_FAILED		4

#define MAX_STARTUP_TIME 10
#define MAX_REFRESH_TIME 10

class bdTunnelPeer
{
public:
	enum STATUS {CONNECTING, CONNECTED, DISCONNECTED};

	bdId mId;
	STATUS mStatus;
	time_t mLastQuery;
	time_t mLastFound;
	struct sockaddr_in mTunnelAddr;
};

class bdTunnelManager: public bdTunnelNode
{
public:
	bdTunnelManager(bdDhtFunctions *fns);

	void iteration();

	/***** Functions to Call down to bdTunnelManager ****/
	/* Request DHT Peer Lookup */
	/* Request Keyword Lookup */
	virtual void connectNode(const bdId *id);
	virtual void disconnectNode(const bdId *id);

	/***** Add / Remove Callback Clients *****/
	virtual void addCallback(BitDhtCallback *cb);
	virtual void removeCallback(BitDhtCallback *cb);

	/* stats and Dht state */
	virtual int startTunnel();
	virtual int stopTunnel();
	virtual int stateDht(); /* STOPPED, STARTING, ACTIVE, FAILED */

	/******************* Internals *************************/

	bool getPriorityPeer(bdId *id);

private:
	int		status();
	int		checkStatus();
	int 	checkPingStatus();
	int 	SearchOutOfDate();

	std::map<bdId, bdTunnelPeer> 		mTunnelPeers;
	std::list<BitDhtCallback *> 		mCallbacks;

	uint32_t mMode;
	time_t   mModeTS;

	bdDhtFunctions *mFns;
};

#endif
