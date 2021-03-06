#ifndef BITDHT_MSGS_H
#define BITDHT_MSGS_H

/*
 * bitdht/bdmsgs.h
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


#include <stdio.h>
#include <inttypes.h>
#include <list>
#include "bitdht/bencode.h"
#include "bitdht/bdobj.h"
#include "bitdht/bdpeer.h"

enum BITDHT_MSG_TYPE {
	BITDHT_MSG_TYPE_UNKNOWN =        0,
	BITDHT_MSG_TYPE_PING =           1,
	BITDHT_MSG_TYPE_PONG =           2,
	BITDHT_MSG_TYPE_FIND_NODE =      3,
	BITDHT_MSG_TYPE_REPLY_NODE =     4,
	BITDHT_MSG_TYPE_GET_HASH =       5,
	BITDHT_MSG_TYPE_REPLY_HASH =     6,
	BITDHT_MSG_TYPE_REPLY_NEAR =     7,
	BITDHT_MSG_TYPE_POST_HASH =      8,
	BITDHT_MSG_TYPE_REPLY_POST =     9,
	BITDHT_MSG_TYPE_NEWCONN =        10,
	BITDHT_MSG_TYPE_REPLY_NEWCONN =  11,
	BITDHT_MSG_TYPE_BROADCAST_CONN = 12,
	BITDHT_MSG_TYPE_ASK_CONN =     	 13,
	BITDHT_MSG_TYPE_REPLY_CONN =     14,
};

#define BITDHT_COMPACTNODEID_LEN 	26
#define BITDHT_COMPACTPEERID_LEN 	6

#define BE_Y_UNKNOWN    0
#define BE_Y_R          1
#define BE_Y_Q          2


/****** Known BD Version Strings ******/

#define BITDHT_VID_RS1	1
#define BITDHT_VID_UT	2


uint32_t bitdht_ecrypt(char *msg, int size, int avail, uint32_t token);
bool bitdht_decrypt(char *msg, int size, uint32_t *token);
int bitdht_create_ping_msg(bdToken *tid, bdNodeId *id, char *msg, int avail);
int bitdht_response_ping_msg(bdToken *tid, bdNodeId *id, bdToken *vid, char *msg, int avail); 
int bitdht_find_node_msg(bdToken *tid, bdNodeId *id, bdNodeId *target, char *msg, int avail);
int bitdht_resp_node_msg(bdToken *tid, bdNodeId *id, std::list<bdId> &nodes,
                                        char *msg, int avail);
int bitdht_get_peers_msg(bdToken *tid, bdNodeId *id, bdNodeId *info_hash,
                                        char *msg, int avail);
int bitdht_peers_reply_hash_msg(bdToken *tid, bdNodeId *id,
                bdToken *token, std::list<std::string> &values,
                                        char *msg, int avail);
int bitdht_peers_reply_closest_msg(bdToken *tid, bdNodeId *id,
                                bdToken *token, std::list<bdId> &nodes,
                                        char *msg, int avail);
int bitdht_announce_peers_msg(bdToken *tid, bdNodeId *id, bdNodeId *info_hash, 
			uint32_t port, bdToken *token, char *msg, int avail);
int bitdht_reply_announce_msg(bdToken *tid, bdNodeId *id,         
                                        char *msg, int avail);
int bitdht_ask_myip_msg(bdToken *tid, bdNodeId *id, char *msg, int avail);
int bitdht_reply_myip_msg(bdToken *tid, bdNodeId *id, bdId *peerId,
		char *msg, bool started, int avail);
int bitdht_broadcast_conn_msg(bdToken *tid, bdNodeId *id, bdNodeId *nodeId, bdNodeId *peerId,
		char *msg, int avail);
int bitdht_ask_conn_msg(bdToken *tid, bdNodeId *id, bdNodeId *nodeId, bdId *peerId,
		char *msg, int avail);
int bitdht_reply_conn_msg(bdToken *tid, bdNodeId *id, bool started,
		char *msg, int avail);

//int response_peers_message()
//int response_closestnodes_message()

be_node *beMsgGetDictNode(be_node *node, const char *key);
int beMsgMatchString(be_node *n, const char *str, int len);
uint32_t beMsgGetY(be_node *n);
uint32_t beMsgType(be_node *n);


uint32_t convertBdVersionToVID(bdVersion *version);

be_node *makeCompactBdIdString(bdId &id);
be_node *makeCompactPeerIds(std::list<std::string> &values);
be_node *makeCompactIdListString(std::list<bdId> &nodes);

int beMsgGetToken(be_node *n, bdToken &token);
int beMsgGetNodeId(be_node *n, bdNodeId &nodeId);
int beMsgGetBdId(be_node *n, bdId &bdId);
int beMsgGetListBdIds(be_node *n, std::list<bdId> &nodes);

int beMsgGetListStrings(be_node *n, std::list<std::string> &values);
int beMsgGetUInt32(be_node *n, uint32_t *port);

/* Low Level conversion functions */
int decodeCompactPeerId(struct sockaddr_in *addr, char *enc, int len);
std::string encodeCompactPeerId(struct sockaddr_in *addr);

int decodeCompactNodeId(bdId *id, char *enc, int len);
std::string encodeCompactNodeId(bdId *id);



#endif
