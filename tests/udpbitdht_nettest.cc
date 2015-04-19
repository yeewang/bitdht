/*
 * bitdht/udpbitdht_nettest.cc
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


#include "udp/udpbitdht.h"
#include "udp/udpstack.h"
#include "bitdht/bdstddht.h"

#include <unistd.h>
#include <string.h>
#include <stdlib.h>

/*******************************************************************
 * DHT test program.
 *
 *
 * Create a couple of each type.
 */

#define MAX_MESSAGE_LEN	10240

int args(char *name)
{
	std::cerr << "Usage: " << name;
	std::cerr << " -p <port> ";
	std::cerr << " -b </path/to/bootfile> ";
	std::cerr << " -u <uid> ";
	std::cerr << " -q <num_queries>";
	std::cerr << " -r  (do dht restarts) ";
	std::cerr << " -j  (do join test) ";
	std::cerr << std::endl;
	return 1;
}

#define DEF_PORT	7500

#define MIN_DEF_PORT	1001
#define MAX_DEF_PORT	16000

#define DEF_BOOTFILE	"bdboot.txt"

int main(int argc, char **argv)
{
	int c;
	int port = DEF_PORT;
	std::string bootfile = DEF_BOOTFILE;
	std::string uid;
	bool setUid = false;
	bool doRandomQueries = false;
	bool doRestart = false;
	bool doThreadJoin = false;
	int noQueries = 0;

	srand(time(NULL));

	while((c = getopt(argc, argv,"rjp:b:u:q:")) != -1)
	{
		switch (c)
		{
		case 'r':
			doRestart = true;
			break;
		case 'j':
			doThreadJoin = true;
			break;
		case 'p':
		{
			int tmp_port = atoi(optarg);
			if ((tmp_port > MIN_DEF_PORT) && (tmp_port < MAX_DEF_PORT))
			{
				port = tmp_port;
				LOG << log4cpp::Priority::INFO << "Port: " << port;
				LOG << log4cpp::Priority::INFO << std::endl;
			}
			else
			{
				LOG << log4cpp::Priority::INFO << "Invalid Port";
				LOG << log4cpp::Priority::INFO << std::endl;
				args(argv[0]);
				return 1;
			}

		}
		break;
		case 'b':
		{
			bootfile = optarg;
			LOG << log4cpp::Priority::INFO << "Bootfile: " << bootfile;
			LOG << log4cpp::Priority::INFO << std::endl;
		}
		break;
		case 'u':
		{
			setUid = true;
			uid = optarg;
			LOG << log4cpp::Priority::INFO << "UID: " << uid;
			LOG << log4cpp::Priority::INFO << std::endl;
		}
		break;
		case 'q':
		{
			doRandomQueries = true;
			noQueries = atoi(optarg);
			LOG << log4cpp::Priority::INFO << "Doing Random Queries";
			LOG << log4cpp::Priority::INFO << std::endl;
		}
		break;

		default:
		{
			args(argv[0]);
			return 1;
		}
		break;
		}
	}


	bdDhtFunctions *fns = new bdStdDht();

	bdNodeId id;

	/* start off with a random id! */
	bdStdRandomNodeId(&id);

	if (setUid)
	{
		int len = uid.size();
		if (len > 20)
		{
			len = 20;
		}

		for(int i = 0; i < len; i++)
		{
			id.data[i] = uid[i];
		}
	}

	LOG << log4cpp::Priority::INFO << "Using NodeId: ";
	fns->bdPrintNodeId(LOG << log4cpp::Priority::INFO, &id);
	LOG << log4cpp::Priority::INFO << std::endl;

	/* setup the udp port */
	struct sockaddr_in local;
	memset(&local, 0, sizeof(local));
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = 0;
	local.sin_port = htons(port);
	UdpStack *udpstack = new UdpStack(local);

	/* create bitdht component */
	std::string dhtVersion = "dbTEST";
	UdpBitDht *bitdht = new UdpBitDht(udpstack, &id, dhtVersion,  bootfile, fns);

	/* add in the stack */
	udpstack->addReceiver(bitdht);

	/* register callback display */
	bdDebugCallback *cb = new bdDebugCallback();
	bitdht->addCallback(cb);

	/* startup threads */
	//udpstack->start();
	bitdht->start();




	/* do a couple of random continuous searchs. */

	uint32_t mode = BITDHT_QFLAGS_DO_IDLE;

	int count = 0;
	int running = 1;

	LOG << log4cpp::Priority::INFO << "Starting Dht: ";
	LOG << log4cpp::Priority::INFO << std::endl;
	bitdht->startDht();


	if (doRandomQueries)
	{
		for(int i = 0; i < noQueries; i++)
		{
			bdNodeId rndId;
			bdStdRandomNodeId(&rndId);

			LOG << log4cpp::Priority::INFO << "BitDht Launching Random Search: ";
			bdStdPrintNodeId(LOG << log4cpp::Priority::INFO, &rndId);
			LOG << log4cpp::Priority::INFO << std::endl;

			bitdht->addFindNode(&rndId, mode);

		}
	}

	{

		bdNodeId rndId;
		bdStdRandomNodeId(&rndId);

		LOG << log4cpp::Priority::INFO << "BitDht Launching Random Search: ";
		bdStdPrintNodeId(LOG << log4cpp::Priority::INFO, &rndId);
		LOG << log4cpp::Priority::INFO << std::endl;

		bitdht->addFindNode(&rndId, mode);
	}

	while(1)
	{
		sleep(60);

		LOG << log4cpp::Priority::INFO << "BitDht State: ";
		LOG << log4cpp::Priority::INFO << bitdht->stateDht();
		LOG << log4cpp::Priority::INFO << std::endl;

		LOG << log4cpp::Priority::INFO << "Dht Network Size: ";
		LOG << log4cpp::Priority::INFO << bitdht->statsNetworkSize();
		LOG << log4cpp::Priority::INFO << std::endl;

		LOG << log4cpp::Priority::INFO << "BitDht Network Size: ";
		LOG << log4cpp::Priority::INFO << bitdht->statsBDVersionSize();
		LOG << log4cpp::Priority::INFO << std::endl;

		if (++count == 2)
		{
			/* switch to one-shot searchs */
			mode = 0;
		}

		if (doThreadJoin)
		{
			/* change address */
			if (count % 2 == 0)
			{

				LOG << log4cpp::Priority::INFO << "Resetting UdpStack: ";
				LOG << log4cpp::Priority::INFO << std::endl;

				udpstack->resetAddress(local);
			}
		}
		if (doRestart)
		{
			if (count % 2 == 1)
			{
				if (running)
				{

					LOG << log4cpp::Priority::INFO << "Stopping Dht: ";
					LOG << log4cpp::Priority::INFO << std::endl;

					bitdht->stopDht();
					running = 0;
				}
				else
				{
					LOG << log4cpp::Priority::INFO << "Starting Dht: ";
					LOG << log4cpp::Priority::INFO << std::endl;

					bitdht->startDht();
					running = 1;
				}
			}
		}
	}
	return 1;
}


