/*
 * bitdht/bdmidids_test.cc
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
#include "bitdht/bdstddht.h"
#include <iostream>

int main(int argc, char **argv)
{

	/* Do this multiple times */
	int i, j;

	std::clog << "Test Mid Peer Intersection....." << std::endl;
	for(i = 0; i < 10; i++)
	{
		bdNodeId targetId;
		bdNodeId peerId;

		bdStdRandomNodeId(&targetId);
		bdStdRandomNodeId(&peerId);
	
		std::clog << "-------------------------------------------------" << std::endl;

		for(j = 0; j < 10; j++)
		{

			bdNodeId midId;

			bdStdRandomMidId(&targetId, &peerId, &midId);

			bdMetric TPmetric;
			bdMetric TMmetric;
			bdMetric PMmetric;

			bdStdDistance(&targetId, &peerId, &TPmetric);
			bdStdDistance(&targetId, &midId,  &TMmetric);
			bdStdDistance(&peerId, &midId,    &PMmetric);

			int TPdist = bdStdBucketDistance(&TPmetric);
			int TMdist = bdStdBucketDistance(&TMmetric);
			int PMdist = bdStdBucketDistance(&PMmetric);

			std::clog << "Target: ";
			bdStdPrintNodeId(std::clog,&targetId);
			std::clog << " Peer: ";
			bdStdPrintNodeId(std::clog,&peerId);
			std::clog << std::endl;

			std::clog << "\tTarget ^ Peer: ";
			bdStdPrintNodeId(std::clog,&TPmetric);
			std::clog << " Bucket: " << TPdist;
			std::clog << std::endl;

			std::clog << "\tTarget ^ Mid: ";
			bdStdPrintNodeId(std::clog,&TMmetric);
			std::clog << " Bucket: " << TMdist;
			std::clog << std::endl;

			std::clog << "\tPeer ^ Mid: ";
			bdStdPrintNodeId(std::clog,&PMmetric);
			std::clog << " Bucket: " << PMdist;
			std::clog << std::endl;

			/* now save mid to peer... and repeat */
			peerId = midId;
		}
	}

	return 1;
}


