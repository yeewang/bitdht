
CPPFLAGS += -g -Wall -D BE_DEBUG -lpthread -D DEBUG_MGR
LDFLAGS += -g -Wall -D BE_DEBUG -lpthread -D DEBUG_MGR
CFLAGS += -g -Wall -D BE_DEBUG -D DEBUG_MGR

noinst_LIBRARIES = libbitdht.a

libbitdht_a_SOURCES = bitdht/bencode.c \
		bitdht/bdobj.cc \
		bitdht/bdmsgs.cc \
		bitdht/bdpeer.cc \
		bitdht/bdquery.cc \
		bitdht/bdhash.cc \
		bitdht/bdstore.cc \
		bitdht/bdnode.cc \
		bitdht/bdmanager.cc \
		bitdht/bdstddht.cc \
		bitdht/bdhistory.cc \
		util/bdnet.cc \
		util/bdthreads.cc \
		udp/udplayer.cc \
		udp/udpstack.cc \
		udp/udpbitdht.cc 
		
