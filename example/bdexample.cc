

#include <unistd.h>
#include "bitdht/bdiface.h"
#include "bitdht/bdstddht.h"
#include "bdhandler.h"

int main(int argc, char **argv)
{

	/* startup dht : with a random id! */
	bdNodeId ownId;
	bdStdRandomNodeId(&ownId);

	uint16_t port = 6775;
	std::string appId = "TD01";
	std::string bootstrapfile = "bdboot.txt";

	BitDhtHandler dht(&ownId, port, appId, bootstrapfile);

	/* run your program */
	while(1)
	{
		//dht.mUdpBitDht->mBitDhtManager->getOwnId(&ownId);

		bdNodeId searchId;
		bdStdNodeId(&searchId, "32f54e697351ff4aec29cdbaabf2fbe3467cc267");

		dht.FindNode(&searchId);

		sleep(180);

		dht.DropNode(&searchId);

		LOG << log4cpp::Priority::INFO << "is Active: "<< dht.getActive() << std::endl;
	}

	return 1;
}
