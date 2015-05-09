	
AM_CFLAGS = -Wall -lpthread 
AM_CXXFLAGS = -Wall  -lpthread -std=c++11

if DEBUG
AM_CFLAGS += -g3 -O0 -D BE_DEBUG
AM_CXXFLAGS += -g3 -O0 -D BE_DEBUG
else
AM_CFLAGS += -O2
AM_CXXFLAGS += -O2
endif

noinst_LIBRARIES = libbitdht.a

libbitdht_a_SOURCES = bitdht/bencode.c \
		bitdht/bdobj.cc \
		bitdht/bdmsgs.cc \
		bitdht/bdpeer.cc \
		bitdht/bdquery.cc \
		bitdht/bdhash.cc \
		bitdht/bdstore.cc \
		bitdht/bdnode.cc \
		bitdht/bdtunnelnode.cc \
		bitdht/bdmanager.cc \
		bitdht/bdtunnelmanager.cc \
		bitdht/bdstddht.cc \
		bitdht/bdhistory.cc \
		util/bdnet.cc \
		util/bdthreads.cc \
		util/bdlog.cc \
		udp/udplayer.cc \
		udp/udpstack.cc \
		udp/udpbitdht.cc \
		udp/udptunnel.cc
